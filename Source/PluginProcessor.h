#pragma once

#include <JuceHeader.h>
#include "MidiDeviceManager.h"
#include "MidiLearnBank.h"
#include "TempoDetector.h"
#include <array>

/**
    MVP scope only — see plan/issues/12-mvp-v0.md.

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
    // Empty if the last load attempt succeeded (or nothing's been tried yet).
    // Set when formatManager can't decode the chosen file at all — e.g. no
    // MP3 support compiled in — so a bad pick fails loudly in the UI instead
    // of just silently leaving "No file loaded" up with no explanation.
    juce::String getLoadError() const;
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
    // nothing. See plan/issues/11-open-questions-assumptions.md item 10 for the
    // history of what was tried before landing here.
    void setEditingMappings (bool shouldEdit);
    bool isEditingMappings() const { return editingMappings.load(); }

    // Pad touch display (not while editing mappings): purple is sustained at
    // full intensity for as long as the pad is physically held down (velocity-
    // scaled, no time-based fade); teal only starts once the pad is released,
    // then fades over ~1800ms. Requires tracking note-off, not just note-on.
    static constexpr int numPads = 16;

    // --- Cue points + pad roles (plan/issues/22) -------------------------
    //
    // The 4x4 grid splits in half by function, not by hardware:
    //   pads 0-7  (bottom two rows) — latch: press once, play from that
    //             pad's cue point through to the end of the file;
    //   pads 8-15 (top two rows)    — momentary twins of the *same* cue
    //             points: pad N+8 shares pad N's cue but plays only while
    //             physically held, stopping the instant it's released.
    // So padIndex % numCuePoints is the cue index for either half, and
    // latch-vs-hold is the only difference between the two halves.
    //
    // Each top row's momentary behavior is user-toggleable (the two
    // checkboxes left of rows 3 and 4); unchecked, that row latches like the
    // bottom two. Both default to on.
    static constexpr int numCuePoints = 8;
    static constexpr int padsPerRow = 4;
    static constexpr int firstMomentaryPadIndex = numCuePoints; // pads 8-15
    static constexpr int numMomentaryRows = 2;

    // Cue 0 (track start) and cue 1 (8s in) are both set automatically on
    // load. Cues 2-7 start empty and capture the live playhead the first
    // time their pad is pressed *while playing* — pressing an empty cue with
    // nothing playing is a deliberate no-op, since there's no playhead to
    // mark and aliasing it to "start" would make it indistinguishable from
    // cue 0. See plan/issues/22.
    static constexpr double autoCueTwoSeconds = 8.0;
    int getCuePointSample (int cueIndex) const;
    bool isCuePointSet (int cueIndex) const;
    static int cueIndexForPad (int padIndex) { return padIndex % numCuePoints; }

    // rowOffset 0 = pads 8-11 (third row), 1 = pads 12-15 (top row).
    bool isMomentaryRowEnabled (int rowOffset) const;
    void setMomentaryRowEnabled (int rowOffset, bool shouldBeMomentary);
    bool isPadMomentary (int padIndex) const;

    // Live playhead in samples, or -1 when stopped — what cue capture reads.
    int getPlaybackPositionSamples() const;

    float getPadTouchAmount (int padIndex) const;      // purple: velocity-scaled, sustained while held, 0 once released
    float getPadAfterglowAmount (int padIndex) const;  // teal: 0 while held, fades from full over ~1800ms after release
    bool isPadAssigned (int padIndex) const { return padBank.isSlotAssigned (padIndex); }
    void armLearnPadSlot (int slotIndex) { padBank.armSlot (slotIndex); } // click-to-target; only meaningful while editing
    int getLearningPadSlot() const { return padBank.getLearningSlot(); }

    static constexpr int numFaders = 4;
    static constexpr int numKnobs = 4;

    // Fixed control roles — plan/issues/22's signal chain:
    //
    //   host audio in ─→ [knob 0: input trim] ─┐
    //                                          ├─→ [fader 0: master out] ─→ out
    //   cue playback  ─→ [fader 1: pitch]  ────┘
    //
    // Modelled on a mixer channel deliberately: on a 2-channel DJ mixer both
    // channels are already taken by the decks, so this plugin's own output
    // has no physical fader anywhere in the chain and has to carry its own
    // master level separately from input trim. These are *slot* indices, not
    // CC numbers — which physical control lands in each slot is still
    // MIDI-learn's job, so nothing here is tied to one controller model.
    static constexpr int inputGainKnobIndex = 0;
    static constexpr int masterOutputFaderIndex = 0;
    static constexpr int pitchAdjustFaderIndex = 1;

    // --- Pitch Adjust (fader 1) ------------------------------------------
    // Varispeed, turntable-style: the read position advances at
    // 1.0 + percent/100 samples per output sample with linear interpolation,
    // so pitch moves with tempo rather than being held. Standard DJ ±8%.
    static constexpr float pitchAdjustRangePercent = 8.0f;
    // A physical fader can't land on exactly 0.00% — CC 64 of 0..127 works
    // out to +0.06% — so anything this close to centre reads as true zero.
    static constexpr float pitchAdjustDetentPercent = 0.1f;
    float getPitchAdjustPercent() const { return pitchAdjustPercent.load(); }

    // --- Per-fader locks ---------------------------------------------------
    // Locking a fader snaps it back to neutral and holds it there: fader 0 to
    // unity, fader 1 to 0.00%, faders 2-3 to their midpoint. Incoming MIDI
    // for a locked fader is ignored, so a bumped physical fader can't quietly
    // undo the lock — the same guarantee setGainLocked() has always made.
    void setFaderLocked (int index, bool shouldLock);
    bool isFaderLocked (int index) const;
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

    // --- Tempo (plan/issues/16, revised by plan/issues/22) ------------------
    //
    // BPM still comes from all three original sources — libsonare
    // auto-detect on load, manual entry, and tap tempo — and is still
    // displayed and available. What plan/issues/22 retired is the *binding*
    // of a detected beat grid onto pads 0-3 as loop regions: those pads are
    // cue pads now, one rule per pad, so nothing silently changes meaning
    // depending on whether a BPM happens to have been established yet.
    void setManualBpm (double bpm);
    double getCurrentBpm() const { return currentBpm.load(); }

    void registerTapTempo();
    void armLearnTapTempo();
    bool isLearningTapTempo() const { return learningTapTempo.load(); }
    juce::String getTapTempoDescription() const;

private:
    void processIncomingMidi (const juce::MidiMessage& message);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::File getPresetFileForCurrentDevice() const;

    // Pad press/release, cue semantics — see plan/issues/22.
    void triggerCuePad (int padIndex);
    void releaseCuePad (int padIndex);
    void startPlaybackFromSample (int startSample, int owningPadIndex);
    void resetCuePointsForLoadedFile();
    // Recomputes pitchAdjustPercent from fader 1's live position (or forces
    // 0.00% while that fader is locked).
    void refreshPitchFromFader();

    juce::AudioFormatManager formatManager;

    juce::CriticalSection bufferLock;
    juce::AudioBuffer<float> sampleBuffer;
    juce::String loadedFileName;
    juce::String loadErrorMessage; // empty = no error; set when formatManager can't read a chosen file

    // Fractional so varispeed can advance it by a non-integer rate; -1.0 is
    // the stopped sentinel, matching what the int version used to mean.
    std::atomic<double> playbackPosition { -1.0 };
    std::atomic<bool> playing { false };
    std::atomic<bool> gainLocked { false };
    std::atomic<bool> triggerRequested { false };
    std::atomic<bool> stopRequested { false };
    std::atomic<int> pendingStartSample { 0 };
    // Which pad's press started what's currently playing, so a momentary
    // pad's release only stops playback it actually owns — releasing pad 9
    // must not cut a track that pad 1 started.
    std::atomic<int> activePlaybackPadIndex { -1 };

    std::array<std::atomic<int>, (size_t) numCuePoints> cuePointSample; // -1 = unset
    std::array<std::atomic<bool>, (size_t) numMomentaryRows> momentaryRowEnabled;

    std::atomic<float> pitchAdjustPercent { 0.0f };
    std::array<std::atomic<bool>, (size_t) numFaders> faderLocked {};

    std::atomic<double> currentBpm { 0.0 };
    std::atomic<bool> learningTapTempo { false };
    std::atomic<int> tapTempoNoteNumber { -1 };
    std::atomic<int> tapTempoMidiChannel { -1 };
    std::atomic<double> lastTapTimestampMs { 0.0 };
    juce::CriticalSection tapTempoLock;
    std::vector<double> tapIntervalsMs; // guarded by tapTempoLock

    juce::SmoothedValue<float> smoothedInputGain;
    juce::SmoothedValue<float> smoothedMasterOutput;

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
    std::array<std::atomic<bool>, (size_t) numPads> padHeld {};
    std::array<std::atomic<double>, (size_t) numPads> padReleaseTimestampMs {};

    MidiLearnBank faderBank { numFaders };
    std::array<std::atomic<float>, (size_t) numFaders> faderValues {};

    MidiLearnBank knobBank { numKnobs };
    std::array<std::atomic<float>, (size_t) numKnobs> knobValues {};

    // Declared last so it's destroyed *first* — its destructor waits for any
    // running analysis job, which must finish (or be safely abandoned)
    // before tempoDetector/sampleBuffer above it get torn down.
    std::unique_ptr<TempoDetector> tempoDetector;
    juce::ThreadPool tempoAnalysisPool { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuViGlowAudioProcessor)
};
