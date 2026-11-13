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
    currentSampleRate = sampleRate;
    currentGain = 1.0f;
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

    const auto targetGainDb = apvts.getRawParameterValue("targetGain")->load();
    const auto attackSeconds = apvts.getRawParameterValue("attack")->load();
    const auto releaseSeconds = apvts.getRawParameterValue("release")->load();
    const auto mix = apvts.getRawParameterValue("mix")->load();
    const auto targetGain = juce::Decibels::decibelsToGain(targetGainDb);

    auto desiredGain = targetGain;
    if (sidechain.getNumChannels() > 0)
    {
        double mainEnergy = 0.0;
        double sidechainEnergy = 0.0;
        const auto sidechainSamples = juce::jmin(numSamples,
                                                  sidechain.getNumSamples());

        for (auto channel = 0; channel < numMainChannels; ++channel)
        {
            const auto* samples = mainInput.getReadPointer(channel);
            for (auto sample = 0; sample < numSamples; ++sample)
                mainEnergy += static_cast<double>(samples[sample]) * samples[sample];
        }

        for (auto channel = 0; channel < sidechain.getNumChannels(); ++channel)
        {
            const auto* samples = sidechain.getReadPointer(channel);
            for (auto sample = 0; sample < sidechainSamples; ++sample)
                sidechainEnergy += static_cast<double>(samples[sample]) * samples[sample];
        }

        const auto mainSampleCount =
            static_cast<double>(numMainChannels) * numSamples;
        const auto sidechainSampleCount =
            static_cast<double>(sidechain.getNumChannels()) * sidechainSamples;
        const auto mainRms = static_cast<float>(
            std::sqrt(mainEnergy / mainSampleCount));
        const auto sidechainRms = sidechainSampleCount > 0.0
            ? static_cast<float>(std::sqrt(sidechainEnergy / sidechainSampleCount))
            : 0.0f;
        const auto ratio = mainRms / (sidechainRms + 0.0001f);

        if (ratio < 1.0f)
            desiredGain *= ratio;
        else
            desiredGain = 1.0f;
    }

    const auto sampleRate = currentSampleRate > 0.0
        ? currentSampleRate
        : 44100.0;
    const auto attackCoefficient = std::exp(
        -1.0f / static_cast<float>(sampleRate * juce::jmax(attackSeconds, 0.001f)));
    const auto releaseCoefficient = std::exp(
        -1.0f / static_cast<float>(sampleRate * juce::jmax(releaseSeconds, 0.001f)));

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto coefficient = desiredGain < currentGain
            ? attackCoefficient
            : releaseCoefficient;
        currentGain = desiredGain + coefficient * (currentGain - desiredGain);

        for (auto channel = 0; channel < numMainChannels; ++channel)
        {
            const auto drySample = mainInput.getReadPointer(channel)[sample];
            mainOutput.getWritePointer(channel)[sample] =
                drySample * (1.0f - mix) + drySample * currentGain * mix;
        }
    }
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
        juce::AudioParameterFloatAttributes().withLabel("s")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release", 1},
        "Release",
        juce::NormalisableRange<float>(0.1f, 5.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StudioBridgeBalanceAudioProcessor();
}
