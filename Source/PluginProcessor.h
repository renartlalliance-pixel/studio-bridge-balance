#pragma once

#include <atomic>
#include <juce_audio_utils/juce_audio_utils.h>

class StudioBridgeBalanceAudioProcessor final : public juce::AudioProcessor
{
public:
    StudioBridgeBalanceAudioProcessor();
    ~StudioBridgeBalanceAudioProcessor() override;

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

    bool loadAudioFile(const juce::File& file, juce::String& errorMessage);
    void setAudioPreviewPlaying(bool shouldPlay);
    bool renderAudioFileToWav(const juce::File& inputFile,
                              const juce::File& outputFile,
                              juce::String& errorMessage) const;

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
    juce::AudioFormatManager audioFormatManager;
    juce::TimeSliceThread audioReadAheadThread{"Studio Bridge Balance audio reader"};
    juce::AudioTransportSource audioTransportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> audioFileSource;
    std::atomic<int> audioFileChannels{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioBridgeBalanceAudioProcessor)
};
