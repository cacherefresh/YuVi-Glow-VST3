#pragma once

#include <JuceHeader.h>
#include <vector>

// Abstraction over "give me an audio buffer, get back beat timestamps" so
// the actual detection library is swappable without touching anything else
// in the codebase — explicit requirement from plan/16-beat-detection-and-loop-pads.md:
// libsonare is the current choice, not a permanent one. If it becomes
// unmaintained or a better option appears, write one new class implementing
// this interface and change the single construction site in PluginProcessor.
class TempoDetector
{
public:
    virtual ~TempoDetector() = default;

    // Returns beat timestamps in seconds (empty on failure or silence).
    // Not real-time-safe — call off the audio thread, see AGENTS.md's
    // audio-thread-discipline rule.
    virtual std::vector<double> detectBeatTimestamps (const juce::AudioBuffer<float>& buffer, double sampleRate) = 0;
};
