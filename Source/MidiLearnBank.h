#pragma once

#include <JuceHeader.h>
#include <vector>

// A bank of N independently MIDI-learnable slots, identified by a note
// number (pads) or CC number (knobs/faders) — the caller decides which.
// Extracted because PluginProcessor used to have this exact same
// assignment/arm/steal/auto-fill logic written out three times, once each
// for pads, faders, and knobs (see AGENTS.md's code-organization note).
//
// Behavior while editing (see handleIncomingIdentifier):
//  - an explicitly armed slot (armSlot()) binds the next *different*
//    identifier it sees, stealing it from any other slot that already
//    held it — repeated messages carrying the identifier already bound
//    there are no-ops, which is what makes continuous controllers safe
//    (a knob turn sends dozens of CC messages per gesture; only a
//    genuinely different CC number should ever cause a rebind);
//  - with nothing explicitly armed, an unrecognized identifier auto-fills
//    the first unassigned slot, so a whole bank can be captured by just
//    touching every control in turn with no clicking required.
// While not editing, incoming identifiers only look up an existing slot —
// nothing gets assigned.
class MidiLearnBank
{
public:
    explicit MidiLearnBank (int numSlotsIn)
        : numSlots (numSlotsIn), assignment ((size_t) numSlotsIn, -1)
    {
    }

    int getNumSlots() const { return numSlots; }

    void reset()
    {
        const juce::ScopedLock sl (lock);
        std::fill (assignment.begin(), assignment.end(), -1);
        learningSlot.store (-1);
    }

    void armSlot (int slotIndex)
    {
        if (slotIndex < 0 || slotIndex >= numSlots)
            return;
        learningSlot.store (slotIndex);
    }

    int getLearningSlot() const { return learningSlot.load(); }

    // For cross-bank disambiguation — see PluginProcessor::processIncomingMidi's
    // fader/knob handling, which uses this to stop a brand-new CC number from
    // being auto-claimed by two different banks at once (faders and knobs
    // both just send generic CC messages, so nothing else tells them apart).
    bool isIdentifierAssigned (int identifier) const
    {
        const juce::ScopedLock sl (lock);
        for (int i = 0; i < numSlots; ++i)
            if (assignment[(size_t) i] == identifier)
                return true;
        return false;
    }

    // Read-only lookup, no learn-mode side effects — for messages that
    // should never assign anything (e.g. note-off, which only needs to know
    // "which pad was this?" to clear its held state).
    int findSlotForIdentifier (int identifier) const
    {
        const juce::ScopedLock sl (lock);
        for (int i = 0; i < numSlots; ++i)
            if (assignment[(size_t) i] == identifier)
                return i;
        return -1;
    }

    bool isSlotAssigned (int slotIndex) const
    {
        if (slotIndex < 0 || slotIndex >= numSlots)
            return false;
        const juce::ScopedLock sl (lock);
        return assignment[(size_t) slotIndex] >= 0;
    }

    // Call for every incoming note/CC number. Returns the slot it now maps
    // to (whether just bound or already known), or -1 if it matches nothing.
    int handleIncomingIdentifier (int identifier, bool editing)
    {
        const juce::ScopedLock sl (lock);

        if (! editing)
        {
            for (int i = 0; i < numSlots; ++i)
                if (assignment[(size_t) i] == identifier)
                    return i;
            return -1;
        }

        const int armedSlot = learningSlot.load();

        if (armedSlot >= 0)
        {
            if (assignment[(size_t) armedSlot] != identifier)
            {
                for (int i = 0; i < numSlots; ++i)
                    if (i != armedSlot && assignment[(size_t) i] == identifier)
                        assignment[(size_t) i] = -1;

                assignment[(size_t) armedSlot] = identifier;
                learningSlot.store (-1); // bound — release the explicit arm
            }

            return armedSlot;
        }

        for (int i = 0; i < numSlots; ++i)
            if (assignment[(size_t) i] == identifier)
                return i;

        for (int i = 0; i < numSlots; ++i)
        {
            if (assignment[(size_t) i] < 0)
            {
                assignment[(size_t) i] = identifier;
                return i;
            }
        }

        return -1;
    }

    // For preset save/load and DAW-state save/restore.
    int getAssignment (int slotIndex) const
    {
        if (slotIndex < 0 || slotIndex >= numSlots)
            return -1;
        const juce::ScopedLock sl (lock);
        return assignment[(size_t) slotIndex];
    }

    void setAssignment (int slotIndex, int identifier)
    {
        if (slotIndex < 0 || slotIndex >= numSlots)
            return;
        const juce::ScopedLock sl (lock);
        assignment[(size_t) slotIndex] = identifier;
    }

private:
    int numSlots;
    mutable juce::CriticalSection lock;
    std::vector<int> assignment; // -1 = unassigned; guarded by lock
    std::atomic<int> learningSlot { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiLearnBank)
};
