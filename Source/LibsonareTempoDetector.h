#pragma once

#include "TempoDetector.h"

// The only header in this codebase that (transitively, via the .cpp) touches
// libsonare — see TempoDetector.h for why that isolation matters.
class LibsonareTempoDetector : public TempoDetector
{
public:
    std::vector<double> detectBeatTimestamps (const juce::AudioBuffer<float>& buffer, double sampleRate) override;
};
