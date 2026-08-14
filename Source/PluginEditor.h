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

    juce::Label gainLabel { {}, "VST MASTER Gain" };
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::ToggleButton lockGainButton { "Lock @ 0dB (50%)" };

    juce::Label midiDeviceLabel { {}, "MIDI Input" };
    juce::ComboBox midiDeviceBox;
    juce::String lastDeviceSignature;
    bool hasDetectedDevicesOnce = false;

    juce::TextButton saveDefaultButton { "Save as Default for Device" };
    juce::TextButton loadDefaultButton { "Load Default for Device" };

    PadGridComponent padGrid;
    ControlPanelComponent controlPanel;

    // Tempo + beat-aligned loop pads (plan/issues/16). Pads 0-3 in the grid above
    // auto-populate as loop regions once a BPM is known from any of these.
    juce::Label bpmLabel { {}, "BPM" };
    juce::TextEditor bpmEditor;
    juce::TextButton tapTempoButton { "Tap Tempo" };

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
