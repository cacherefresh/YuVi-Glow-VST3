#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PadGridComponent.h"
#include "ControlPanelComponent.h"
#include "HeaderBarComponent.h"
#include "MidiControllerSettingsPanel.h"
#include "AppSettingsPanel.h"

class YuViGlowAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit YuViGlowAudioProcessorEditor (YuViGlowAudioProcessor&);
    ~YuViGlowAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateMidiDeviceDetection();
    void chooseFile();

    YuViGlowAudioProcessor& processor;

    juce::TextButton loadButton { "Load Audio File..." };
    juce::TextButton stopButton { "STOP" }; // always says STOP — a momentary master-off switch, never toggles label/state
    juce::Label fileNameLabel;
    juce::Label statusLabel;

    // Stage 1 of the signal chain (plan/issues/22): trim on the incoming host
    // audio, bound to knob 1 on the controller.
    juce::Label gainLabel { {}, "Input Gain (Knob 1)" };
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::ToggleButton lockGainButton { "Lock @ 0dB (50%)" };

    // Stage 3: master output on the summed result, bound to fader 1. Its lock
    // lives with the other fader locks in ControlPanelComponent rather than
    // being duplicated here.
    juce::Label masterOutputLabel { {}, "Master Output (Fader 1)" };
    juce::Slider masterOutputSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterOutputAttachment;

    juce::Label midiDeviceLabel { {}, "MIDI Input" };
    juce::ComboBox midiDeviceBox;
    juce::String lastDeviceSignature;
    bool hasDetectedDevicesOnce = false;

    juce::TextButton saveDefaultButton { "Save as Default for Device" };
    juce::TextButton loadDefaultButton { "Load Default for Device" };

    PadGridComponent padGrid;
    ControlPanelComponent controlPanel;

    // Tempo (plan/issues/16, revised by plan/issues/22 — the beat grid no longer
    // binds itself onto pads 0-3 as loop regions; those are cue pads now).
    juce::Label bpmLabel { {}, "BPM" };
    juce::TextEditor bpmEditor;
    juce::TextButton tapTempoButton { "Tap Tempo" };

    // Read-only varispeed readout for fader 2, sat beside Tap Tempo.
    juce::Label pitchAdjLabel { {}, "Pitch Adj" };
    juce::Label pitchAdjValueLabel;

    // Header bar + the two menus it opens (plan/issues/18). Controller-mapping
    // configuration (learn buttons, edit toggle, reset, learn tap tempo)
    // lives in midiSettingsPanel now, reachable only via the header bar's
    // icon — see MidiControllerSettingsPanel.h for what moved and why.
    HeaderBarComponent headerBar;
    MidiControllerSettingsPanel midiSettingsPanel;
    AppSettingsPanel appSettingsPanel;
    juce::TooltipWindow tooltipWindow { this }; // required for Button::setTooltip() to actually render anything

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuViGlowAudioProcessorEditor)
};
