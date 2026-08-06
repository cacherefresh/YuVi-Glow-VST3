#pragma once

#include <JuceHeader.h>
#include "MidiDeviceManager.h"
#include "MidiLearnBank.h"
#include <array>

/**
    MVP scope only — see plan/12-mvp-v0.md.

    Loads one audio file, plays it start-to-end when a learned trigger pad
    is pressed (or Stop is clicked), and applies a host-audio input gain
    controlled by a learned fader. Nothing here is hardcoded to one
    controller model — MIDI-learn binds whatever note/CC the selected
    device sends, so this works identically with the M-Audio Code 49's
    pads/fader or an Akai MPD226's pads/fader, or any other class-compliant
    MIDI controller. Control comes from a direct MIDI connection this
    processor opens itself (via MidiDeviceManager), independent of whatever
    MIDI the host (Serato) does or doesn't forward — see the "Serato only
    hosts effects" ground rule in AGENTS.md.

    Organized as: this file owns audio playback/gain and orchestrates three
    smaller, reusable pieces — MidiDeviceManager (which physical MIDI ports
    we're listening to) and three MidiLearnBank instances (pad/fader/knob
    assignment state, one bank per control type instead of triplicated
    logic) — see AGENTS.md's code-organization note.
*/
class YuViGlowAudioProcessor : public juce::AudioProcessor
{
public:
    YuViGlowAudioProcessor();
    ~YuViGlowAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // File / playback
    void loadAudioFile (const juce::File& file);
    juce::String getLoadedFileName() const;
    bool isFileLoaded() const;
    void triggerPlayback();
    void stopPlayback();
    bool isPlaying() const { return playing.load(); }

    // Forces the input-gain parameter to unity (1.0x = the fader's 50%
    // midpoint on its 0-2.0 range = 0dB, i.e. no boost/cut) and holds it
    // there — the audio-thread gain calc also clamps to unity directly while
    // locked (not just via the parameter), and incoming gain-fader MIDI is
    // ignored while locked, so a bumped physical fader can't un-lock it.
    void setGainLocked (bool shouldLock);
    bool isGainLocked() const { return gainLocked.load(); }

    // MIDI device selection — thin forwarding to MidiDeviceManager, kept on
    // the processor since that's what the UI already talks to.
    using DetectedDeviceCategory = MidiDeviceManager::DeviceCategory;
    using DetectedMidiDevice = MidiDeviceManager::DetectedDevice;

    std::vector<DetectedMidiDevice> getClassifiedMidiInputs() const { return midiDeviceManager.getClassifiedInputs(); }
    void setMidiInputDevice (const DetectedMidiDevice& device) { midiDeviceManager.connect (device); }
    void disconnectMidiInput() { midiDeviceManager.disconnect(); }
    juce::String getCurrentMidiInputDisplayName() const { return midiDeviceManager.getCurrentDisplayName(); }

    // Applies a best-guess trigger note (36, the near-universal "pad 1" default)
    // and gain CC (7, GM Volume) — only for bindings that haven't been explicitly
    // learned yet, and only meant to be called for DetectedDeviceCategory::custom,
    // since Code 49 / MPD226 defaults are deliberately not guessed (see AGENTS.md).
    void applyBestGuessMappingIfUnlearned();

    // MIDI learn
    void armLearnTriggerPad();
    void armLearnGainFader();
    bool isLearningTrigger() const { return learningTrigger.load(); }
    bool isLearningGain() const { return learningGain.load(); }
    juce::String getTriggerDescription() const;
    juce::String getGainFaderDescription() const;

    // --- Full-surface mapping editor: pads, knobs, faders ---------------
    //
    // A single global edit mode governs all three MidiLearnBank instances
    // together. While ON:
    //  - clicking any pad/knob/fader arms just that slot (yellow) as an
    //    explicit target — the next matching message steals its note/CC
    //    from wherever else it was bound and rebinds it here;
    //  - if nothing is explicitly armed, any note/CC that doesn't already
    //    match an assignment auto-fills the first unassigned (red) slot in
    //    that bank, so you can just physically touch every pad/knob/fader in
    //    turn with no clicking at all;
    //  - repeated messages from an already-bound note/CC (e.g. continuing to
    //    turn the same knob) are pure no-ops for assignment — they only
    //    update the live value. This is specifically what makes continuous
    //    controllers safe: earlier versions advanced on *every* CC message,
    //    so one knob turn (which sends dozens of messages) blew through all
    //    4 slots instantly. Advancing now only happens when a genuinely
    //    different note/CC number appears, which is a hardware guarantee per
    //    physical control, not an assumption about encoder direction/mode.
    // While OFF, everything behaves as a plain display: pads show their
    // touch-glow, knobs/faders show live position, and clicking does
    // nothing. See plan/11-open-questions-assumptions.md item 10 for the
    // history of what was tried before landing here.
    void setEditingMappings (bool shouldEdit);
    bool isEditingMappings() const { return editingMappings.load(); }

    static constexpr int numPads = 16;
    float getPadTouchAmount (int padIndex) const;      // fast purple flash, ~400ms (display only, not while editing)
    float getPadAfterglowAmount (int padIndex) const;  // delayed teal afterglow, slow fade (display only, not while editing)
    bool isPadAssigned (int padIndex) const { return padBank.isSlotAssigned (padIndex); }
    void armLearnPadSlot (int slotIndex) { padBank.armSlot (slotIndex); } // click-to-target; only meaningful while editing
    int getLearningPadSlot() const { return padBank.getLearningSlot(); }

    static constexpr int numFaders = 4;
    static constexpr int numKnobs = 4;
    float getFaderValue (int index) const; // 0..1, live CC position
    float getKnobValue (int index) const;  // 0..1, live CC position
    bool isFaderAssigned (int index) const { return faderBank.isSlotAssigned (index); }
    bool isKnobAssigned (int index) const { return knobBank.isSlotAssigned (index); }
    void armLearnFaderSlot (int index) { faderBank.armSlot (index); }
    void armLearnKnobSlot (int index) { knobBank.armSlot (index); }
    int getLearningFaderSlot() const { return faderBank.getLearningSlot(); }
    int getLearningKnobSlot() const { return knobBank.getLearningSlot(); }

    // Clears every pad/fader/knob assignment back to unlearned.
    void resetAllMappings();

    // Saves/loads the full mapping (trigger pad, gain fader, all pads,
    // faders, knobs) to a small file keyed by whichever device category is
    // currently connected (mpd226.xml / code49.xml / custom-<name>.xml)
    // under the app's data directory — so a capture only has to happen once
    // per physical controller, not once per app launch.
    bool saveMappingPresetForCurrentDevice();
    bool loadMappingPresetForCurrentDevice();

private:
    void processIncomingMidi (const juce::MidiMessage& message);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::File getPresetFileForCurrentDevice() const;

    juce::AudioFormatManager formatManager;

    juce::CriticalSection bufferLock;
    juce::AudioBuffer<float> sampleBuffer;
    juce::String loadedFileName;

    std::atomic<int> playbackPosition { -1 };
    std::atomic<bool> playing { false };
    std::atomic<bool> gainLocked { false };
    std::atomic<bool> triggerRequested { false };
    std::atomic<bool> stopRequested { false };

    juce::SmoothedValue<float> smoothedInputGain;

    MidiDeviceManager midiDeviceManager;

    std::atomic<bool> editingMappings { false };

    std::atomic<bool> learningTrigger { false };
    std::atomic<bool> learningGain { false };
    std::atomic<int> triggerNoteNumber { -1 };
    std::atomic<int> triggerMidiChannel { -1 };
    std::atomic<int> gainCcNumber { -1 };
    std::atomic<int> gainMidiChannel { -1 };

    MidiLearnBank padBank { numPads };
    std::array<std::atomic<int>, (size_t) numPads> padVelocities {};
    std::array<std::atomic<double>, (size_t) numPads> padHitTimestampMs {};

    MidiLearnBank faderBank { numFaders };
    std::array<std::atomic<float>, (size_t) numFaders> faderValues {};

    MidiLearnBank knobBank { numKnobs };
    std::array<std::atomic<float>, (size_t) numKnobs> knobValues {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuViGlowAudioProcessor)
};
