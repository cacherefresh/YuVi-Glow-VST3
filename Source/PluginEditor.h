#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PadGridComponent.h"
#include "ControlPanelComponent.h"

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
    juce::TextButton stopButton { "Stop" };
    juce::Label fileNameLabel;
    juce::Label statusLabel;

    juce::Label gainLabel { {}, "Input Gain" };
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::ToggleButton lockGainButton { "Lock @ 0dB (50%)" };

    juce::Label midiDeviceLabel { {}, "MIDI Input" };
    juce::ComboBox midiDeviceBox;
    juce::String lastDeviceSignature;
    bool hasDetectedDevicesOnce = false;

    juce::TextButton learnTriggerButton { "Learn Trigger Pad" };
    juce::Label triggerLearnedLabel;

    juce::TextButton learnGainButton { "Learn Gain Fader" };
    juce::Label gainLearnedLabel;

    juce::TextButton editMappingButton { "Edit MIDI Mapping" };
    juce::TextButton resetAllMappingsButton { "Reset All Mappings" };
    juce::TextButton saveDefaultButton { "Save as Default for Device" };
    juce::TextButton loadDefaultButton { "Load Default for Device" };
    juce::Label mappingStatusLabel;

    PadGridComponent padGrid;
    ControlPanelComponent controlPanel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuViGlowAudioProcessorEditor)
};
