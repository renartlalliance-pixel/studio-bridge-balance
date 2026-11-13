#pragma once

#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

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

    StudioBridgeBalanceAudioProcessor& audioProcessor;

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
