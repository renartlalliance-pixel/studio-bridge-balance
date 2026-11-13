#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

WaveformWebBrowser::WaveformWebBrowser(
    const juce::WebBrowserComponent::Options& options)
    : juce::WebBrowserComponent(options)
{
}

bool WaveformWebBrowser::isInterestedInFileDrag(
    const juce::StringArray& files)
{
    return std::any_of(files.begin(), files.end(), [](const juce::String& path)
    {
        return path.endsWithIgnoreCase(".wav");
    });
}

void WaveformWebBrowser::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& path : files)
    {
        const juce::File file(path);
        if (file.hasFileExtension(".wav"))
        {
            if (onWavFileDropped)
                onWavFileDropped(file);
            return;
        }
    }
}

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
    waveformFormatManager.registerBasicFormats();

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

    audioFileStatusLabel.setText("Drop a WAV file into the waveform",
                                 juce::dontSendNotification);
    audioFileStatusLabel.setJustificationType(juce::Justification::centredLeft);
    audioFileStatusLabel.setColour(juce::Label::textColourId,
                                   juce::Colour(0xffd4af37));
    addAndMakeVisible(audioFileStatusLabel);

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

    auto browserOptions = juce::WebBrowserComponent::Options()
        .withNativeIntegrationEnabled()
        .withEventListener(
            juce::Identifier{"juceButtonClick"},
            [this](juce::var event)
            {
                if (webComponent != nullptr && webComponent->onButtonClick)
                {
                    webComponent->onButtonClick(
                        event.getProperty("buttonId", juce::var{}).toString());
                }
            });
#if JUCE_WINDOWS
    browserOptions = browserOptions.withBackend(
        juce::WebBrowserComponent::Options::Backend::webview2);
#endif

    webComponent = std::make_unique<WaveformWebBrowser>(browserOptions);
    webComponent->onButtonClick = [this](const juce::String& buttonId)
    {
        if (buttonId == "play")
            handlePlayPause(true);
        else if (buttonId == "pause")
            handlePlayPause(false);
        else if (buttonId == "export")
            exportProcessedAudio();
    };
    webComponent->onWavFileDropped = [this](const juce::File& file)
    {
        loadWavFile(file);
    };
    addAndMakeVisible(*webComponent);

    const auto html = juce::String(
        R"html(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
* { box-sizing: border-box; }
body { margin: 0; padding: 12px; color: #eee; background: #101010; font: 13px Arial, sans-serif; }
#waveform { display: block; width: 100%; height: 118px; background: #071007; border: 1px solid #335533; }
#status { height: 22px; padding-top: 5px; color: #d4af37; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; }
.buttons { display: flex; gap: 8px; }
button { min-height: 34px; padding: 0 15px; color: #eee; background: #292929; border: 1px solid #806c32; font-weight: bold; cursor: pointer; }
button:hover { background: #403719; }
#export { margin-left: auto; color: #101010; background: #d4af37; }
</style>
</head>
<body>
<canvas id="waveform"></canvas>
<div id="status">Drop a WAV file here to view its waveform.</div>
<div class="buttons">
  <button id="play" type="button">PLAY</button>
  <button id="pause" type="button">PAUSE</button>
  <button id="export" type="button">GLISSER VERS LA TIMELINE</button>
</div>
<script>
(function () {
  var canvas = document.getElementById("waveform");
  var context = canvas.getContext("2d");
  var peaks = [];

  function drawWaveform() {
    var width = canvas.clientWidth;
    var height = canvas.clientHeight;
    var ratio = window.devicePixelRatio || 1;
    canvas.width = Math.max(1, Math.floor(width * ratio));
    canvas.height = Math.max(1, Math.floor(height * ratio));
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.fillStyle = "#071007";
    context.fillRect(0, 0, width, height);
    context.strokeStyle = "#00d040";
    context.lineWidth = 1;
    context.beginPath();
    context.moveTo(0, height / 2);
    context.lineTo(width, height / 2);
    context.stroke();
    if (peaks.length === 0)
      return;
    context.strokeStyle = "#55ff77";
    context.lineWidth = Math.max(1, width / peaks.length);
    for (var i = 0; i < peaks.length; ++i) {
      var x = i * width / peaks.length;
      var amplitude = Math.max(0, Math.min(1, peaks[i])) * height * 0.46;
      context.beginPath();
      context.moveTo(x, (height - amplitude) / 2);
      context.lineTo(x, (height + amplitude) / 2);
      context.stroke();
    }
  }

  window.updateWaveformData = function (arrayOfPeaks) {
    peaks = arrayOfPeaks || [];
    drawWaveform();
  };
  window.setWaveformStatus = function (message) {
    document.getElementById("status").textContent = message;
  };
  window.juceBridge = {
    play: function () { window.__JUCE__.backend.emitEvent("juceButtonClick", { buttonId: "play" }); },
    pause: function () { window.__JUCE__.backend.emitEvent("juceButtonClick", { buttonId: "pause" }); },
    exportProcessedAudio: function () { window.__JUCE__.backend.emitEvent("juceButtonClick", { buttonId: "export" }); }
  };
  document.getElementById("play").addEventListener("click", window.juceBridge.play);
  document.getElementById("pause").addEventListener("click", window.juceBridge.pause);
  document.getElementById("export").addEventListener("click", window.juceBridge.exportProcessedAudio);
  window.addEventListener("resize", drawWaveform);
  drawWaveform();
})();
</script>
</body>
</html>)html");
    webComponent->goToURL(
        "data:text/html;charset=utf-8,"
        + juce::URL::addEscapeChars(html, true));

    startTimerHz(30);
    setSize(760, 660);
}

StudioBridgeBalanceAudioProcessorEditor::~StudioBridgeBalanceAudioProcessorEditor()
{
    stopTimer();
    webComponent.reset();
    targetGainSlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    sensitivitySlider.setLookAndFeel(nullptr);
    refrainBoostSlider.setLookAndFeel(nullptr);
    activeButton.setLookAndFeel(nullptr);
}

void StudioBridgeBalanceAudioProcessorEditor::loadWavFile(
    const juce::File& file)
{
    juce::String errorMessage;
    if (!audioProcessor.loadAudioFile(file, errorMessage))
    {
        setBrowserStatus(errorMessage);
        return;
    }

    loadedAudioFile = file;
    updateWaveform(file);
}

void StudioBridgeBalanceAudioProcessorEditor::handlePlayPause(bool isPlaying)
{
    if (loadedAudioFile.existsAsFile())
    {
        audioProcessor.setAudioPreviewPlaying(isPlaying);
        setBrowserStatus(isPlaying ? "Playing " + loadedAudioFile.getFileName()
                                   : "Paused " + loadedAudioFile.getFileName());
    }
    else
    {
        setBrowserStatus("Drop a WAV file before starting playback.");
    }
}

void StudioBridgeBalanceAudioProcessorEditor::exportProcessedAudio()
{
    if (!loadedAudioFile.existsAsFile())
    {
        setBrowserStatus("Drop a WAV file before exporting.");
        return;
    }

    const auto temporaryDirectory =
        juce::File::getSpecialLocation(juce::File::tempDirectory);
    const auto outputFile = temporaryDirectory.getNonexistentChildFile(
        "StudioBridgeBalance-Processed",
        ".wav",
        false);
    juce::String errorMessage;
    if (!audioProcessor.renderAudioFileToWav(loadedAudioFile,
                                             outputFile,
                                             errorMessage))
    {
        setBrowserStatus(errorMessage);
        return;
    }

    juce::StringArray files{outputFile.getFullPathName()};
    const auto dragStarted =
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            files,
            true,
            this);
    setBrowserStatus(dragStarted
                         ? "Processed WAV ready to drag into your timeline."
                         : "The operating system could not start the file drag.");
}

void StudioBridgeBalanceAudioProcessorEditor::setBrowserStatus(
    const juce::String& message)
{
    audioFileStatusLabel.setText(message, juce::dontSendNotification);
    if (webComponent != nullptr)
    {
        webComponent->evaluateJavascript(
            "window.setWaveformStatus("
            + juce::JSON::toString(juce::var(message))
            + ");");
    }
}

void StudioBridgeBalanceAudioProcessorEditor::updateWaveform(
    const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        waveformFormatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
    {
        setBrowserStatus("The WAV file contains no readable audio.");
        return;
    }

    constexpr int peakCount = 512;
    constexpr int samplesPerChunk = 8192;
    const auto channelCount = static_cast<int>(
        juce::jmin(reader->numChannels, 2u));
    juce::AudioBuffer<float> chunk(channelCount, samplesPerChunk);
    juce::Array<float> peaks;
    peaks.ensureStorageAllocated(peakCount);

    for (auto peakIndex = 0; peakIndex < peakCount; ++peakIndex)
    {
        const auto startSample =
            reader->lengthInSamples * peakIndex / peakCount;
        const auto endSample =
            reader->lengthInSamples * (peakIndex + 1) / peakCount;
        auto position = startSample;
        auto peak = 0.0f;

        while (position < endSample)
        {
            const auto samplesToRead = static_cast<int>(juce::jmin(
                static_cast<juce::int64>(samplesPerChunk),
                endSample - position));
            if (!reader->read(&chunk,
                              0,
                              samplesToRead,
                              position,
                              true,
                              channelCount > 1))
            {
                setBrowserStatus("The WAV waveform could not be read.");
                return;
            }

            for (auto channel = 0; channel < channelCount; ++channel)
            {
                const auto* samples = chunk.getReadPointer(channel);
                for (auto sample = 0; sample < samplesToRead; ++sample)
                    peak = juce::jmax(peak, std::abs(samples[sample]));
            }
            position += samplesToRead;
        }

        peaks.add(peak);
    }

    juce::StringArray peakValues;
    for (const auto peak : peaks)
        peakValues.add(juce::String(peak, 4));
    webComponent->evaluateJavascript(
        "window.updateWaveformData(["
        + peakValues.joinIntoString(",")
        + "]);");
    setBrowserStatus("Loaded " + file.getFileName());
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
    auto area = getLocalBounds().reduced(60, 28);
    auto topArea = area.removeFromTop(72);
    lcdDisplay.setBounds(topArea.removeFromLeft(topArea.getWidth() - 100));
    activeButton.setBounds(topArea.reduced(10));

    area.removeFromTop(14);
    audioFileStatusLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    auto controlsArea = area.removeFromTop(210);
    const auto sliderWidth = controlsArea.getWidth() / 5;
    const std::array<std::pair<juce::Label*, juce::Slider*>, 5> controls{{
        {&targetGainLabel, &targetGainSlider},
        {&attackLabel, &attackSlider},
        {&releaseLabel, &releaseSlider},
        {&sensitivityLabel, &sensitivitySlider},
        {&refrainBoostLabel, &refrainBoostSlider}
    }};

    for (const auto& [label, slider] : controls)
    {
        auto controlArea = controlsArea.removeFromLeft(sliderWidth);
        label->setBounds(controlArea.removeFromTop(28));
        slider->setBounds(controlArea.reduced(4, 0));
    }

    area.removeFromTop(10);
    webComponent->setBounds(area);
}
