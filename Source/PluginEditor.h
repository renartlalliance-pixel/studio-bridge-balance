#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

class StudioBridgeBalanceAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit StudioBridgeBalanceAudioProcessorEditor(
        StudioBridgeBalanceAudioProcessor& processor);
    ~StudioBridgeBalanceAudioProcessorEditor() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    StudioBridgeBalanceAudioProcessor& audioProcessor;

    juce::Label title;
    juce::Label targetGainLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label mixLabel;

    juce::Slider targetGainSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider mixSlider;

    juce::AudioProcessorValueTreeState::SliderAttachment targetGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        StudioBridgeBalanceAudioProcessorEditor)
};
