#pragma once

#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

class WaveformWebBrowser final : public juce::WebBrowserComponent,
                                 public juce::FileDragAndDropTarget
{
public:
    explicit WaveformWebBrowser(const Options& options);

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    std::function<void(const juce::String&)> onButtonClick;
    std::function<void(const juce::File&)> onWavFileDropped;
};

class TascamLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TascamLookAndFeel();

    void drawRotarySlider(juce::Graphics& graphics,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosition,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class StudioBridgeBalanceAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit StudioBridgeBalanceAudioProcessorEditor(
        StudioBridgeBalanceAudioProcessor& processor);
    ~StudioBridgeBalanceAudioProcessorEditor() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadWavFile(const juce::File& file);
    void handlePlayPause(bool isPlaying);
    void exportProcessedAudio();
    void setBrowserStatus(const juce::String& message);
    void updateWaveform(const juce::File& file);

    StudioBridgeBalanceAudioProcessor& audioProcessor;
    std::unique_ptr<WaveformWebBrowser> webComponent;
    juce::AudioFormatManager waveformFormatManager;
    juce::File loadedAudioFile;

    juce::Slider targetGainSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider sensitivitySlider;
    juce::Slider refrainBoostSlider;

    juce::Label targetGainLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label sensitivityLabel;
    juce::Label refrainBoostLabel;
    juce::Label lcdDisplay;
    juce::Label audioFileStatusLabel;
    juce::ToggleButton activeButton;
    TascamLookAndFeel tascamLookAndFeel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        targetGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        sensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        refrainBoostAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        activeAttachment;

    float currentMeterValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        StudioBridgeBalanceAudioProcessorEditor)
};
