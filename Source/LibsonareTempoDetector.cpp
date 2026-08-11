#include "LibsonareTempoDetector.h"

#include <sonare/sonare_c_types_enums.h>
#include <sonare/sonare_c_types_functions.h>

// Using libsonare's C API (sonare_detect_beats), not the sonare::quick::
// C++ wrapper its docs site describes — that wrapper doesn't exist in the
// actual v1.6.0 source as fetched; the C API is real, verified against the
// checked-out headers, and is the more stable surface anyway (it's what the
// language bindings themselves are built on). See plan/16 for the
// verification trail.
std::vector<double> LibsonareTempoDetector::detectBeatTimestamps (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (buffer.getNumSamples() <= 0)
        return {};

    // sonare_detect_beats expects mono float samples — sum down if the
    // loaded file is stereo (or more).
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    std::vector<float> mono ((size_t) numSamples, 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = buffer.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            mono[(size_t) i] += src[i];
    }

    if (numChannels > 1)
        for (auto& s : mono)
            s /= (float) numChannels;

    float* outTimes = nullptr;
    size_t outCount = 0;

    const SonareError err = sonare_detect_beats (mono.data(), mono.size(), (int) sampleRate, &outTimes, &outCount);

    std::vector<double> beats;
    if (err == SONARE_OK && outTimes != nullptr)
    {
        beats.reserve (outCount);
        for (size_t i = 0; i < outCount; ++i)
            beats.push_back ((double) outTimes[i]);
    }

    if (outTimes != nullptr)
        sonare_free_floats (outTimes);

    return beats;
}
