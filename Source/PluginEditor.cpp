#include "PluginEditor.h"

#include <array>
#include <utility>

StudioBridgeBalanceAudioProcessorEditor::StudioBridgeBalanceAudioProcessorEditor(
    StudioBridgeBalanceAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      targetGainAttachment(audioProcessor.apvts, "targetGain", targetGainSlider),
      attackAttachment(audioProcessor.apvts, "attack", attackSlider),
      releaseAttachment(audioProcessor.apvts, "release", releaseSlider),
      mixAttachment(audioProcessor.apvts, "mix", mixSlider)
{
    title.setText("Studio Bridge Balance", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::Font(22.0f, juce::Font::bold));
    addAndMakeVisible(title);

    const auto configureControl = [this](juce::Label& label,
                                         juce::Slider& slider,
                                         const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 22);
        addAndMakeVisible(slider);
    };

    configureControl(targetGainLabel, targetGainSlider, "Target Gain");
    configureControl(attackLabel, attackSlider, "Attack");
    configureControl(releaseLabel, releaseSlider, "Release");
    configureControl(mixLabel, mixSlider, "Mix");

    setSize(520, 340);
}

void StudioBridgeBalanceAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void StudioBridgeBalanceAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    title.setBounds(bounds.removeFromTop(44));

    const auto controlWidth = bounds.getWidth() / 4;
    const std::array<std::pair<juce::Label*, juce::Slider*>, 4> controls{{
        {&targetGainLabel, &targetGainSlider},
        {&attackLabel, &attackSlider},
        {&releaseLabel, &releaseSlider},
        {&mixLabel, &mixSlider}
    }};

    for (const auto& [label, slider] : controls)
    {
        auto control = bounds.removeFromLeft(controlWidth);
        label->setBounds(control.removeFromTop(28));
        slider->setBounds(control.reduced(8, 0));
    }
}
