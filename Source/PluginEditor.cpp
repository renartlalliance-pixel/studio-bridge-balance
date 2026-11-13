#include "PluginEditor.h"

#include <array>
#include <utility>

TascamLookAndFeel::TascamLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffd4af37));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff222222));
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxOutlineColourId,
              juce::Colours::transparentBlack);
}

void TascamLookAndFeel::drawRotarySlider(juce::Graphics& graphics,
                                         int x,
                                         int y,
                                         int width,
                                         int height,
                                         float sliderPosition,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    juce::ignoreUnused(slider);

    const auto bounds = juce::Rectangle<int>(x, y, width, height)
                            .toFloat()
                            .reduced(10.0f);
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin(bounds.getWidth() / 2.0f,
                                   bounds.getHeight() / 2.0f);
    const auto angle = rotaryStartAngle
        + sliderPosition * (rotaryEndAngle - rotaryStartAngle);
    const auto diameter = radius * 2.0f;

    graphics.setColour(juce::Colours::black.withAlpha(0.5f));
    graphics.fillEllipse(centre.x - radius + 3.0f,
                         centre.y - radius + 3.0f,
                         diameter,
                         diameter);

    juce::ColourGradient metalGradient(juce::Colour(0xff888888),
                                       centre.x - radius,
                                       centre.y - radius,
                                       juce::Colour(0xff222222),
                                       centre.x + radius,
                                       centre.y + radius,
                                       false);
    graphics.setGradientFill(metalGradient);
    graphics.fillEllipse(centre.x - radius,
                         centre.y - radius,
                         diameter,
                         diameter);

    graphics.setColour(juce::Colour(0xff111111));
    graphics.drawEllipse(centre.x - radius,
                         centre.y - radius,
                         diameter,
                         diameter,
                         2.0f);

    juce::Path pointer;
    const auto pointerThickness = 4.0f;
    pointer.addRectangle(-pointerThickness * 0.5f,
                         -radius,
                         pointerThickness,
                         radius * 0.8f);
    pointer.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    graphics.setColour(juce::Colour(0xffd4af37));
    graphics.fillPath(pointer);
}

StudioBridgeBalanceAudioProcessorEditor::StudioBridgeBalanceAudioProcessorEditor(
    StudioBridgeBalanceAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    const auto configureSlider = [this](juce::Slider& slider,
                                        juce::Label& label,
                                        const juce::String& name,
                                        const juce::String& suffix)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        slider.setTextValueSuffix(suffix);
        slider.setLookAndFeel(&tascamLookAndFeel);
        addAndMakeVisible(slider);
    };

    configureSlider(targetGainSlider, targetGainLabel, "TARGET GAIN", " dB");
    configureSlider(attackSlider, attackLabel, "ATTACK", " ms");
    configureSlider(releaseSlider, releaseLabel, "RELEASE", " ms");
    configureSlider(sensitivitySlider, sensitivityLabel, "SENSITIVITY", " dB");
    configureSlider(refrainBoostSlider, refrainBoostLabel, "REFRAIN BOOST", " dB");

    lcdDisplay.setText("STUDIO BRIDGE\nBALANCE",
                       juce::dontSendNotification);
    lcdDisplay.setJustificationType(juce::Justification::centredLeft);
    lcdDisplay.setFont(juce::Font(16.0f, juce::Font::bold));
    lcdDisplay.setColour(juce::Label::backgroundColourId,
                         juce::Colour(0xff0a1a0a));
    lcdDisplay.setColour(juce::Label::textColourId,
                         juce::Colour(0xff00ff00));
    addAndMakeVisible(lcdDisplay);

    activeButton.setButtonText("ACTIVE");
    activeButton.setLookAndFeel(&tascamLookAndFeel);
    activeButton.setColour(juce::ToggleButton::textColourId,
                           juce::Colours::white);
    activeButton.setColour(juce::ToggleButton::tickColourId,
                           juce::Colour(0xff00ff00));
    activeButton.setColour(juce::ToggleButton::tickDisabledColourId,
                           juce::Colour(0xffff0000));
    addAndMakeVisible(activeButton);

    targetGainAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "targetGain", targetGainSlider);
    attackAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "attack", attackSlider);
    releaseAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "release", releaseSlider);
    sensitivityAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "sensitivity", sensitivitySlider);
    refrainBoostAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "refrainBoost", refrainBoostSlider);
    activeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.apvts, "active", activeButton);

    startTimerHz(30);
    setSize(700, 450);
}

StudioBridgeBalanceAudioProcessorEditor::~StudioBridgeBalanceAudioProcessorEditor()
{
    stopTimer();
    targetGainSlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    sensitivitySlider.setLookAndFeel(nullptr);
    refrainBoostSlider.setLookAndFeel(nullptr);
    activeButton.setLookAndFeel(nullptr);
}

void StudioBridgeBalanceAudioProcessorEditor::timerCallback()
{
    const auto newValue = juce::jlimit(
        -24.0f, 0.0f, audioProcessor.gainReductionDb.load());
    currentMeterValue += (newValue - currentMeterValue) * 0.2f;

    if (audioProcessor.isRefrain.load())
        lcdDisplay.setText("REFRAIN\nBOOST ACTIF",
                           juce::dontSendNotification);
    else
        lcdDisplay.setText("COUPLET\nNORMAL",
                           juce::dontSendNotification);

    repaint(lcdDisplay.getBounds());
}

void StudioBridgeBalanceAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff1a1a1a));

    juce::ColourGradient woodGradient(juce::Colour(0xff5c4033),
                                      0.0f,
                                      0.0f,
                                      juce::Colour(0xff2b1d14),
                                      40.0f,
                                      0.0f,
                                      false);
    graphics.setGradientFill(woodGradient);
    graphics.fillRect(0, 0, 40, getHeight());
    graphics.fillRect(getWidth() - 40, 0, 40, getHeight());

    graphics.setColour(juce::Colour(0xff444444));
    graphics.drawLine(40.0f, 0.0f, 40.0f, static_cast<float>(getHeight()), 2.0f);
    graphics.drawLine(static_cast<float>(getWidth() - 40),
                      0.0f,
                      static_cast<float>(getWidth() - 40),
                      static_cast<float>(getHeight()),
                      2.0f);

    if (lcdDisplay.getBounds().getWidth() > 0)
    {
        auto lcdArea = lcdDisplay.getBounds().toFloat().reduced(10.0f);
        auto meterArea = lcdArea.removeFromRight(40.0f).reduced(0.0f, 20.0f);
        graphics.setColour(juce::Colour(0xff001100));
        graphics.fillRect(meterArea);

        const auto normalizedValue = juce::jmap(
            juce::jlimit(-24.0f, 0.0f, currentMeterValue),
            -24.0f,
            0.0f,
            0.0f,
            1.0f);
        auto activeArea =
            meterArea.removeFromBottom(meterArea.getHeight() * normalizedValue);
        graphics.setColour(currentMeterValue < -6.0f
                               ? juce::Colour(0xffff0000)
                               : juce::Colour(0xff00ff00));
        graphics.fillRect(activeArea);

        graphics.setColour(juce::Colour(0xff00ff00));
        graphics.drawRect(
            lcdDisplay.getBounds().toFloat()
                .reduced(10.0f)
                .removeFromRight(40.0f)
                .reduced(0.0f, 20.0f),
            1.0f);
    }
}

void StudioBridgeBalanceAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(60, 40);
    auto topArea = area.removeFromTop(80);
    lcdDisplay.setBounds(topArea.removeFromLeft(topArea.getWidth() - 100));
    activeButton.setBounds(topArea.reduced(10));

    area.removeFromTop(30);
    const auto sliderWidth = area.getWidth() / 5;
    const std::array<std::pair<juce::Label*, juce::Slider*>, 5> controls{{
        {&targetGainLabel, &targetGainSlider},
        {&attackLabel, &attackSlider},
        {&releaseLabel, &releaseSlider},
        {&sensitivityLabel, &sensitivitySlider},
        {&refrainBoostLabel, &refrainBoostSlider}
    }};

    for (const auto& [label, slider] : controls)
    {
        auto controlArea = area.removeFromLeft(sliderWidth);
        label->setBounds(controlArea.removeFromTop(28));
        slider->setBounds(controlArea.reduced(4, 0));
    }
}
