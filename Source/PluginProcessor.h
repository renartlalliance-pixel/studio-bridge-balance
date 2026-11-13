#pragma once

#include <atomic>
#include <juce_audio_utils/juce_audio_utils.h>

class StudioBridgeBalanceAudioProcessor final : public juce::AudioProcessor
{
public:
    StudioBridgeBalanceAudioProcessor();
    ~StudioBridgeBalanceAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> gainReductionDb{0.0f};
    std::atomic<bool> isRefrain{false};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    double currentSampleRate = 44100.0;
    float currentGain = 1.0f;
    float vocalEnvelope = 0.0f;
    float instrumentEnvelope = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioBridgeBalanceAudioProcessor)
};
