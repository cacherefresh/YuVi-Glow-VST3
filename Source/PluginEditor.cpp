#include "PluginEditor.h"

YuViGlowAudioProcessorEditor::YuViGlowAudioProcessorEditor (YuViGlowAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), padGrid (p), controlPanel (p),
      midiSettingsPanel (p)
{
    setSize (540, 770);

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { chooseFile(); };

    addAndMakeVisible (stopButton);
    stopButton.onClick = [this] { processor.stopPlayback(); };

    addAndMakeVisible (fileNameLabel);
    fileNameLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (gainLabel);
    addAndMakeVisible (gainSlider);
    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "inputGain", gainSlider);

    addAndMakeVisible (lockGainButton);
    lockGainButton.setToggleState (processor.isGainLocked(), juce::dontSendNotification);
    gainSlider.setEnabled (! processor.isGainLocked());
    lockGainButton.onClick = [this]
    {
        const bool locked = lockGainButton.getToggleState();
        processor.setGainLocked (locked);
        gainSlider.setEnabled (! locked);
    };

    addAndMakeVisible (midiDeviceLabel);
    addAndMakeVisible (midiDeviceBox);
    updateMidiDeviceDetection();
    midiDeviceBox.onChange = [this]
    {
        const auto devices = processor.getClassifiedMidiInputs();
        const int index = midiDeviceBox.getSelectedItemIndex();
        if (index < 0 || index >= (int) devices.size())
            return;

        const auto& chosen = devices[(size_t) index];
        processor.setMidiInputDevice (chosen);
        if (chosen.category == YuViGlowAudioProcessor::DetectedDeviceCategory::custom)
            processor.applyBestGuessMappingIfUnlearned();
    };

    addAndMakeVisible (saveDefaultButton);
    saveDefaultButton.onClick = [this] { processor.saveMappingPresetForCurrentDevice(); };

    addAndMakeVisible (loadDefaultButton);
    loadDefaultButton.onClick = [this] { processor.loadMappingPresetForCurrentDevice(); };

    addAndMakeVisible (bpmLabel);
    addAndMakeVisible (bpmEditor);
    bpmEditor.setInputRestrictions (6, "0123456789.");
    bpmEditor.setText (juce::String (processor.getCurrentBpm(), 1), juce::dontSendNotification);
    auto applyBpmFromEditor = [this]
    {
        const double bpm = bpmEditor.getText().getDoubleValue();
        if (bpm > 0.0)
            processor.setManualBpm (bpm);
    };
    bpmEditor.onReturnKey = applyBpmFromEditor;
    bpmEditor.onFocusLost = applyBpmFromEditor;

    addAndMakeVisible (tapTempoButton);
    tapTempoButton.onClick = [this] { processor.registerTapTempo(); };

    addAndMakeVisible (padGrid);
    addAndMakeVisible (controlPanel);

    addAndMakeVisible (headerBar);
    headerBar.onSettingsClicked = [this]
    {
        appSettingsPanel.setVisible (! appSettingsPanel.isVisible());
        if (appSettingsPanel.isVisible())
        {
            headerBar.setMidiSettingsActive (false);
            midiSettingsPanel.setVisible (false);
            appSettingsPanel.toFront (false);
            resized(); // MIDI-settings section may have just collapsed
        }
    };
    headerBar.onMidiSettingsClicked = [this]
    {
        // The icon already flipped its own toggle state before this fires
        // (setClickingTogglesState) — just mirror it onto the section.
        midiSettingsPanel.setVisible (headerBar.isMidiSettingsActive());
        if (midiSettingsPanel.isVisible())
            appSettingsPanel.setVisible (false);
        resized(); // reflow: expanding/collapsing this section shifts everything below it
    };

    addChildComponent (midiSettingsPanel); // starts collapsed — toggled inline by the header bar icon
    addChildComponent (appSettingsPanel);

    startTimerHz (15);
}

YuViGlowAudioProcessorEditor::~YuViGlowAudioProcessorEditor() = default;

void YuViGlowAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void YuViGlowAudioProcessorEditor::resized()
{
    auto fullBounds = getLocalBounds();
    constexpr int headerHeight = 32;
    headerBar.setBounds (fullBounds.removeFromTop (headerHeight));

    // The general Settings stub still floats below its icon (nothing in it
    // yet to overlap) — the MIDI-settings section below is laid out inline
    // instead, in the normal row flow, so it can never overlap anything.
    constexpr int appPanelWidth = 240;
    constexpr int appPanelHeight = 120;
    appSettingsPanel.setBounds (fullBounds.getRight() - appPanelWidth - 12, headerHeight + 4,
                                 appPanelWidth, appPanelHeight);

    auto area = fullBounds.reduced (12);
    auto row = [&area] (int h) { return area.removeFromTop (h); };

    auto fileRow = row (30);
    loadButton.setBounds (fileRow.removeFromLeft (160));
    fileRow.removeFromLeft (8);
    fileNameLabel.setBounds (fileRow);

    area.removeFromTop (8);

    auto statusRow = row (24);
    stopButton.setBounds (statusRow.removeFromLeft (100));
    statusRow.removeFromLeft (8);
    statusLabel.setBounds (statusRow);

    area.removeFromTop (16);

    auto gainRow = row (28);
    gainLabel.setBounds (gainRow.removeFromLeft (140));
    lockGainButton.setBounds (gainRow.removeFromRight (150));
    gainRow.removeFromRight (8);
    gainSlider.setBounds (gainRow);

    area.removeFromTop (16);

    auto midiRow = row (28);
    midiDeviceLabel.setBounds (midiRow.removeFromLeft (90));
    midiDeviceBox.setBounds (midiRow);

    area.removeFromTop (16);

    // Collapsible MIDI-controller-settings section (plan/issues/18): reserves zero
    // space when collapsed, so everything below (Save/Load Default, BPM,
    // the pad grid) shifts straight up to close the gap — no stray blank
    // row left behind either way.
    if (midiSettingsPanel.isVisible())
    {
        midiSettingsPanel.setBounds (row (MidiControllerSettingsPanel::contentHeight));
        area.removeFromTop (16);
    }
    else
    {
        midiSettingsPanel.setBounds ({});
    }

    auto presetButtonRow = row (28);
    saveDefaultButton.setBounds (presetButtonRow.removeFromLeft (200));
    presetButtonRow.removeFromLeft (8);
    loadDefaultButton.setBounds (presetButtonRow.removeFromLeft (200));

    area.removeFromTop (16);

    auto tempoRow = row (28);
    bpmLabel.setBounds (tempoRow.removeFromLeft (40));
    bpmEditor.setBounds (tempoRow.removeFromLeft (70));
    tempoRow.removeFromLeft (8);
    tapTempoButton.setBounds (tempoRow.removeFromLeft (110));

    area.removeFromTop (16);

    // Physical MPD226 layout: pads on the left, faders to their right, knobs
    // above the faders — ControlPanelComponent draws knobs-over-faders
    // internally, this just places it beside the pad grid.
    constexpr int controlPanelWidth = 180;
    const int gridSize = juce::jmin (area.getWidth() - 8 - controlPanelWidth, 220);
    auto bottomRow = area.removeFromTop (gridSize);
    padGrid.setBounds (bottomRow.removeFromLeft (gridSize));
    bottomRow.removeFromLeft (8);
    controlPanel.setBounds (bottomRow.withWidth (controlPanelWidth));
}

void YuViGlowAudioProcessorEditor::timerCallback()
{
    const auto loadError = processor.getLoadError();
    fileNameLabel.setText (processor.isFileLoaded() ? processor.getLoadedFileName()
                                                     : (loadError.isNotEmpty() ? loadError : "No file loaded"),
                            juce::dontSendNotification);
    statusLabel.setText (processor.isPlaying() ? juce::String (juce::CharPointer_UTF8 ("\xf0\x9f\x94\x8a Playing..."))  // U+1F50A speaker-with-sound-waves
                                                : juce::String ("Stopped"),
                          juce::dontSendNotification);

    if (! bpmEditor.hasKeyboardFocus (false))
    {
        const double bpm = processor.getCurrentBpm();
        bpmEditor.setText (bpm > 0.0 ? juce::String (bpm, 1) : juce::String(), juce::dontSendNotification);
    }

    updateMidiDeviceDetection();
}

void YuViGlowAudioProcessorEditor::updateMidiDeviceDetection()
{
    const auto detected = processor.getClassifiedMidiInputs();

    juce::String signature;
    for (auto& d : detected)
    {
        signature << d.displayName << "[";
        for (auto& port : d.ports)
            signature << port.identifier << ";";
        signature << "]";
    }

    if (hasDetectedDevicesOnce && signature == lastDeviceSignature)
        return;

    hasDetectedDevicesOnce = true;
    lastDeviceSignature = signature;

    const auto currentDisplayName = processor.getCurrentMidiInputDisplayName();
    bool currentStillPresent = false;
    for (auto& d : detected)
        if (currentDisplayName.isNotEmpty() && d.displayName == currentDisplayName)
            currentStillPresent = true;

    if (currentDisplayName.isNotEmpty() && ! currentStillPresent)
        processor.disconnectMidiInput(); // previously-connected device was unplugged

    midiDeviceBox.clear (juce::dontSendNotification);
    midiDeviceBox.setEnabled (! detected.empty());
    midiDeviceBox.setTextWhenNothingSelected (detected.empty() ? "<No MIDI Device Detected>" : "Select MIDI Device");

    int idx = 1;
    int matchId = -1;

    for (auto& d : detected)
    {
        midiDeviceBox.addItem (d.displayName, idx);
        if (currentStillPresent && d.displayName == currentDisplayName)
            matchId = idx;
        ++idx;
    }

    if (matchId > 0)
    {
        midiDeviceBox.setSelectedId (matchId, juce::dontSendNotification);
    }
    else if (detected.size() == 1)
    {
        // Exactly one device detected — unambiguous, connect automatically.
        midiDeviceBox.setSelectedId (1, juce::dontSendNotification);
        processor.setMidiInputDevice (detected.front());
        if (detected.front().category == YuViGlowAudioProcessor::DetectedDeviceCategory::custom)
            processor.applyBestGuessMappingIfUnlearned();
    }
}

void YuViGlowAudioProcessorEditor::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select an audio file", juce::File(), "*.wav;*.mp3");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File())
            processor.loadAudioFile (file);
    });
}
