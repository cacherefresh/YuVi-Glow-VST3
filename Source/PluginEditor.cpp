#include "PluginEditor.h"

YuViGlowAudioProcessorEditor::YuViGlowAudioProcessorEditor (YuViGlowAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), padGrid (p), controlPanel (p)
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

    addAndMakeVisible (learnTriggerButton);
    learnTriggerButton.onClick = [this] { processor.armLearnTriggerPad(); };

    addAndMakeVisible (triggerLearnedLabel);
    triggerLearnedLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (learnGainButton);
    learnGainButton.onClick = [this] { processor.armLearnGainFader(); };

    addAndMakeVisible (gainLearnedLabel);
    gainLearnedLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (editMappingButton);
    editMappingButton.setClickingTogglesState (true);
    editMappingButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange.withAlpha (0.7f));
    editMappingButton.onClick = [this]
    {
        processor.setEditingMappings (editMappingButton.getToggleState());
    };

    addAndMakeVisible (resetAllMappingsButton);
    resetAllMappingsButton.onClick = [this] { processor.resetAllMappings(); };

    addAndMakeVisible (saveDefaultButton);
    saveDefaultButton.onClick = [this] { processor.saveMappingPresetForCurrentDevice(); };

    addAndMakeVisible (loadDefaultButton);
    loadDefaultButton.onClick = [this] { processor.loadMappingPresetForCurrentDevice(); };

    addAndMakeVisible (mappingStatusLabel);
    mappingStatusLabel.setJustificationType (juce::Justification::centredLeft);

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

    addAndMakeVisible (learnTapTempoButton);
    learnTapTempoButton.onClick = [this] { processor.armLearnTapTempo(); };

    addAndMakeVisible (tempoStatusLabel);
    tempoStatusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (padGrid);
    addAndMakeVisible (controlPanel);

    startTimerHz (15);
}

YuViGlowAudioProcessorEditor::~YuViGlowAudioProcessorEditor() = default;

void YuViGlowAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void YuViGlowAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
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
    gainLabel.setBounds (gainRow.removeFromLeft (90));
    lockGainButton.setBounds (gainRow.removeFromRight (150));
    gainRow.removeFromRight (8);
    gainSlider.setBounds (gainRow);

    area.removeFromTop (16);

    auto midiRow = row (28);
    midiDeviceLabel.setBounds (midiRow.removeFromLeft (90));
    midiDeviceBox.setBounds (midiRow);

    area.removeFromTop (16);

    auto triggerRow = row (28);
    learnTriggerButton.setBounds (triggerRow.removeFromLeft (160));
    triggerRow.removeFromLeft (8);
    triggerLearnedLabel.setBounds (triggerRow);

    area.removeFromTop (8);

    auto gainLearnRow = row (28);
    learnGainButton.setBounds (gainLearnRow.removeFromLeft (160));
    gainLearnRow.removeFromLeft (8);
    gainLearnedLabel.setBounds (gainLearnRow);

    area.removeFromTop (16);

    auto mappingButtonRow = row (28);
    editMappingButton.setBounds (mappingButtonRow.removeFromLeft (150));
    mappingButtonRow.removeFromLeft (8);
    resetAllMappingsButton.setBounds (mappingButtonRow.removeFromLeft (150));

    area.removeFromTop (8);

    auto presetButtonRow = row (28);
    saveDefaultButton.setBounds (presetButtonRow.removeFromLeft (200));
    presetButtonRow.removeFromLeft (8);
    loadDefaultButton.setBounds (presetButtonRow.removeFromLeft (200));

    area.removeFromTop (8);

    mappingStatusLabel.setBounds (row (20));

    area.removeFromTop (16);

    auto tempoRow = row (28);
    bpmLabel.setBounds (tempoRow.removeFromLeft (40));
    bpmEditor.setBounds (tempoRow.removeFromLeft (70));
    tempoRow.removeFromLeft (8);
    tapTempoButton.setBounds (tempoRow.removeFromLeft (110));
    tempoRow.removeFromLeft (8);
    learnTapTempoButton.setBounds (tempoRow.removeFromLeft (150));

    area.removeFromTop (8);

    tempoStatusLabel.setBounds (row (20));

    area.removeFromTop (8);

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
    fileNameLabel.setText (processor.isFileLoaded() ? processor.getLoadedFileName() : "No file loaded",
                            juce::dontSendNotification);
    statusLabel.setText (processor.isPlaying() ? "Playing..." : "Stopped", juce::dontSendNotification);

    triggerLearnedLabel.setText (processor.isLearningTrigger() ? "Waiting for pad press..."
                                                                 : processor.getTriggerDescription(),
                                  juce::dontSendNotification);
    gainLearnedLabel.setText (processor.isLearningGain() ? "Waiting for fader move..."
                                                            : processor.getGainFaderDescription(),
                               juce::dontSendNotification);

    if (! processor.isEditingMappings())
    {
        mappingStatusLabel.setText ("Click \"Edit MIDI Mapping\" to (re)capture pads/knobs/faders",
                                     juce::dontSendNotification);
    }
    else
    {
        const int learningSlot = processor.getLearningPadSlot();
        const int learningKnob = processor.getLearningKnobSlot();
        const int learningFader = processor.getLearningFaderSlot();

        if (learningSlot >= 0)
        {
            mappingStatusLabel.setText ("Armed: pad " + juce::String (learningSlot + 1) + " of 16 — press it now",
                                         juce::dontSendNotification);
        }
        else if (learningKnob >= 0)
        {
            mappingStatusLabel.setText ("Armed: knob " + juce::String (learningKnob + 1) + " of 4 — turn it now",
                                         juce::dontSendNotification);
        }
        else if (learningFader >= 0)
        {
            mappingStatusLabel.setText ("Armed: fader " + juce::String (learningFader + 1) + " of 4 — move it now",
                                         juce::dontSendNotification);
        }
        else
        {
            mappingStatusLabel.setText ("Editing: red = unassigned, green = assigned. Click one, "
                                         "or just touch any unassigned pad/knob/fader to fill it in.",
                                         juce::dontSendNotification);
        }
    }

    if (! bpmEditor.hasKeyboardFocus (false))
    {
        const double bpm = processor.getCurrentBpm();
        bpmEditor.setText (bpm > 0.0 ? juce::String (bpm, 1) : juce::String(), juce::dontSendNotification);
    }

    tempoStatusLabel.setText (processor.isLearningTapTempo() ? "Waiting for tap tempo button press..."
                                                               : "Tap tempo control: " + processor.getTapTempoDescription(),
                               juce::dontSendNotification);

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
