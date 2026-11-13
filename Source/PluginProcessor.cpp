#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <cmath>

StudioBridgeBalanceAudioProcessor::StudioBridgeBalanceAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain",
                                    juce::AudioChannelSet::stereo(),
                                    false)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void StudioBridgeBalanceAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    currentGain = 1.0f;
    vocalEnvelope = 0.0f;
    instrumentEnvelope = 0.0f;
}

void StudioBridgeBalanceAudioProcessor::releaseResources()
{
}

bool StudioBridgeBalanceAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();
    const auto sidechain = layouts.getChannelSet(true, 1);
    const auto validMainLayout =
        (mainInput == juce::AudioChannelSet::mono()
         || mainInput == juce::AudioChannelSet::stereo())
        && mainInput == mainOutput;
    const auto validSidechainLayout =
        sidechain.isDisabled()
        || sidechain == juce::AudioChannelSet::mono()
        || sidechain == juce::AudioChannelSet::stereo();

    return validMainLayout && validSidechainLayout;
}

void StudioBridgeBalanceAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    auto mainInput = getBusBuffer(buffer, true, 0);
    auto mainOutput = getBusBuffer(buffer, false, 0);
    auto sidechain = getBusBuffer(buffer, true, 1);
    const auto numSamples = mainOutput.getNumSamples();
    const auto numMainChannels = juce::jmin(mainInput.getNumChannels(),
                                            mainOutput.getNumChannels());

    if (numSamples == 0 || numMainChannels == 0)
        return;

    const auto isActive = apvts.getRawParameterValue("active")->load() > 0.5f;
    if (!isActive)
    {
        for (auto channel = 0; channel < numMainChannels; ++channel)
            mainOutput.copyFrom(channel, 0, mainInput, channel, 0, numSamples);

        currentGain = 1.0f;
        gainReductionDb.store(0.0f);
        isRefrain.store(false);
        return;
    }

    const auto targetGainDb = apvts.getRawParameterValue("targetGain")->load();
    const auto attackMs = apvts.getRawParameterValue("attack")->load();
    const auto releaseMs = apvts.getRawParameterValue("release")->load();
    const auto sensitivityDb = apvts.getRawParameterValue("sensitivity")->load();
    const auto refrainBoostDb = apvts.getRawParameterValue("refrainBoost")->load();
    const auto targetGain = juce::Decibels::decibelsToGain(targetGainDb);
    const auto attackSeconds = juce::jmax(attackMs * 0.001f, 0.00001f);
    const auto releaseSeconds = juce::jmax(releaseMs * 0.001f, 0.00001f);
    const auto attackCoefficient = std::exp(
        -1.0 / (currentSampleRate * attackSeconds));
    const auto releaseCoefficient = std::exp(
        -1.0 / (currentSampleRate * releaseSeconds));

    double mainEnergy = 0.0;
    for (auto channel = 0; channel < numMainChannels; ++channel)
    {
        const auto* samples = mainInput.getReadPointer(channel);
        for (auto sample = 0; sample < numSamples; ++sample)
            mainEnergy += static_cast<double>(samples[sample]) * samples[sample];
    }

    const auto mainSampleCount =
        static_cast<double>(numMainChannels) * numSamples;
    const auto mainRms = static_cast<float>(
        std::sqrt(mainEnergy / mainSampleCount));

    double sidechainEnergy = 0.0;
    const auto sidechainSamples = juce::jmin(numSamples,
                                              sidechain.getNumSamples());
    for (auto channel = 0; channel < sidechain.getNumChannels(); ++channel)
    {
        const auto* samples = sidechain.getReadPointer(channel);
        for (auto sample = 0; sample < sidechainSamples; ++sample)
            sidechainEnergy += static_cast<double>(samples[sample]) * samples[sample];
    }

    const auto sidechainSampleCount =
        static_cast<double>(sidechain.getNumChannels()) * sidechainSamples;
    const auto sidechainRms = sidechainSampleCount > 0.0
        ? static_cast<float>(std::sqrt(sidechainEnergy / sidechainSampleCount))
        : 0.0f;
    const auto hasSidechainSignal = sidechainRms > 0.001f;
    auto desiredGain = targetGain;

    if (hasSidechainSignal)
    {
        const auto attackBlockCoefficient = std::pow(
            attackCoefficient, static_cast<double>(numSamples));
        const auto releaseBlockCoefficient = std::pow(
            releaseCoefficient, static_cast<double>(numSamples));
        const auto vocalCoefficient = mainRms > vocalEnvelope
            ? attackBlockCoefficient
            : releaseBlockCoefficient;
        const auto instrumentCoefficient = sidechainRms > instrumentEnvelope
            ? attackBlockCoefficient
            : releaseBlockCoefficient;

        vocalEnvelope = mainRms
            + static_cast<float>(vocalCoefficient) * (vocalEnvelope - mainRms);
        instrumentEnvelope = sidechainRms
            + static_cast<float>(instrumentCoefficient)
                * (instrumentEnvelope - sidechainRms);

        const auto instrumentDb =
            juce::Decibels::gainToDecibels(instrumentEnvelope, -60.0f);
        const auto refrainDetected = instrumentDb > sensitivityDb;
        isRefrain.store(refrainDetected);

        const auto ratio = vocalEnvelope / (instrumentEnvelope + 0.0001f);
        if (refrainDetected)
            desiredGain *= juce::Decibels::decibelsToGain(refrainBoostDb);

        if (ratio < 1.0f)
            desiredGain *= ratio;
        else
            desiredGain = 1.0f;
    }
    else
    {
        vocalEnvelope = mainRms;
        instrumentEnvelope = 0.0f;
        isRefrain.store(false);
    }

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto coefficient = desiredGain < currentGain
            ? attackCoefficient
            : releaseCoefficient;
        currentGain = desiredGain
            + static_cast<float>(coefficient) * (currentGain - desiredGain);

        for (auto channel = 0; channel < numMainChannels; ++channel)
            mainOutput.getWritePointer(channel)[sample] =
                mainInput.getReadPointer(channel)[sample] * currentGain;
    }

    const auto gainDb = juce::Decibels::gainToDecibels(currentGain, -24.0f);
    gainReductionDb.store(juce::jmin(gainDb, 0.0f));
}

juce::AudioProcessorEditor* StudioBridgeBalanceAudioProcessor::createEditor()
{
    return new StudioBridgeBalanceAudioProcessorEditor(*this);
}

bool StudioBridgeBalanceAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String StudioBridgeBalanceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool StudioBridgeBalanceAudioProcessor::acceptsMidi() const
{
    return false;
}

bool StudioBridgeBalanceAudioProcessor::producesMidi() const
{
    return false;
}

bool StudioBridgeBalanceAudioProcessor::isMidiEffect() const
{
    return false;
}

double StudioBridgeBalanceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int StudioBridgeBalanceAudioProcessor::getNumPrograms()
{
    return 1;
}

int StudioBridgeBalanceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void StudioBridgeBalanceAudioProcessor::setCurrentProgram(int)
{
}

const juce::String StudioBridgeBalanceAudioProcessor::getProgramName(int)
{
    return {};
}

void StudioBridgeBalanceAudioProcessor::changeProgramName(int,
                                                          const juce::String&)
{
}

void StudioBridgeBalanceAudioProcessor::getStateInformation(
    juce::MemoryBlock& destinationData)
{
    if (const auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destinationData);
}

void StudioBridgeBalanceAudioProcessor::setStateInformation(const void* data,
                                                            int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
StudioBridgeBalanceAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"targetGain", 1},
        "Target Gain",
        juce::NormalisableRange<float>(0.0f, 24.0f),
        6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attack", 1},
        "Attack",
        juce::NormalisableRange<float>(0.01f, 1.0f),
        0.1f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1},
        "Release",
        juce::NormalisableRange<float>(0.1f, 5.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"sensitivity", 1},
        "Sensitivity",
        juce::NormalisableRange<float>(-40.0f, 0.0f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"refrainBoost", 1},
        "Refrain Boost",
        juce::NormalisableRange<float>(0.0f, 6.0f),
        3.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"active", 1},
        "Active",
        true));
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StudioBridgeBalanceAudioProcessor();
}
