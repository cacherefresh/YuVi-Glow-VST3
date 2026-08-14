#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LibsonareTempoDetector.h"

#include <cmath>

YuViGlowAudioProcessor::YuViGlowAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      midiDeviceManager ([this] (const juce::MidiMessage& message) { processIncomingMidi (message); })
{
    formatManager.registerBasicFormats();
    for (auto& v : cuePointSample)
        v.store (-1);
    for (auto& v : momentaryRowEnabled)
        v.store (true); // pads 9-16 are hold-to-play out of the box (plan/issues/22)

    tempoDetector = std::make_unique<LibsonareTempoDetector>();
}

YuViGlowAudioProcessor::~YuViGlowAudioProcessor()
{
    disconnectMidiInput();
}

juce::AudioProcessorValueTreeState::ParameterLayout YuViGlowAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Two separate stages, deliberately — see the signal-chain comment in
    // PluginProcessor.h. "inputGain" keeps its original parameter ID so
    // existing saved host state and device presets still resolve; only its
    // display name changes now that it's explicitly the input trim rather
    // than the only gain in the plugin.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterOutput", 1 },
        "Master Output",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f));

    return { params.begin(), params.end() };
}

void YuViGlowAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    smoothedInputGain.reset (sampleRate, 0.02);
    smoothedInputGain.setCurrentAndTargetValue (*apvts.getRawParameterValue ("inputGain"));
    smoothedMasterOutput.reset (sampleRate, 0.02);
    smoothedMasterOutput.setCurrentAndTargetValue (*apvts.getRawParameterValue ("masterOutput"));
}

void YuViGlowAudioProcessor::releaseResources() {}

bool YuViGlowAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    const auto outSet = layouts.getMainOutputChannelSet();
    if (outSet != mono && outSet != stereo)
        return false;

    return layouts.getMainInputChannelSet() == outSet;
}

void YuViGlowAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (stopRequested.exchange (false))
    {
        playing.store (false);
        playbackPosition.store (-1.0);
        activePlaybackPadIndex.store (-1);
    }

    if (triggerRequested.exchange (false))
    {
        playbackPosition.store ((double) pendingStartSample.load());
        playing.store (true);
    }

    // Stage 1 — input trim (knob 0) on the host audio only. Serato's deck
    // audio is what's in the buffer at this point; the plugin's own playback
    // is added afterwards so the trim never touches it.
    const float rawInputGain = apvts.getRawParameterValue ("inputGain")->load();
    smoothedInputGain.setTargetValue (gainLocked.load() ? 1.0f : rawInputGain);

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = smoothedInputGain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= gain;
    }

    // Stage 2 — cue-point playback at the varispeed rate set by fader 1.
    if (playing.load())
    {
        const juce::ScopedLock sl (bufferLock);
        const int bufferLength = sampleBuffer.getNumSamples();
        const int sampleChannels = sampleBuffer.getNumChannels();

        if (bufferLength > 1)
        {
            // 1.0 = unmodified. Always positive over the ±8% range, so the
            // read position only ever moves forward.
            const double rate = 1.0 + (double) pitchAdjustPercent.load() / 100.0;
            // Stop one sample short of the end: linear interpolation reads
            // pos and pos+1, so the last whole sample has no partner.
            const double lastReadable = (double) (bufferLength - 1);

            // Channel pointer arrays hoisted out of the sample loop — this
            // runs per output sample on the audio thread, and getReadPointer/
            // getWritePointer per sample per channel is pure overhead.
            auto* const* dst = buffer.getArrayOfWritePointers();
            const auto* const* src = sampleBuffer.getArrayOfReadPointers();

            double pos = playbackPosition.load();
            int written = 0;

            while (written < numSamples && pos >= 0.0 && pos < lastReadable)
            {
                const int index = (int) pos;
                const float frac = (float) (pos - (double) index);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float* s = src[juce::jmin (ch, sampleChannels - 1)];
                    dst[ch][written] += s[index] + frac * (s[index + 1] - s[index]);
                }

                pos += rate;
                ++written;
            }

            if (pos >= lastReadable || pos < 0.0)
            {
                playing.store (false);
                playbackPosition.store (-1.0);
                activePlaybackPadIndex.store (-1);
            }
            else
            {
                playbackPosition.store (pos);
            }
        }
    }

    // Stage 3 — master output (fader 0) on the summed result. This is the
    // only level control the plugin's output has once it reaches a 2-channel
    // DJ mixer whose channels are both already taken by decks.
    const float rawMasterOutput = apvts.getRawParameterValue ("masterOutput")->load();
    smoothedMasterOutput.setTargetValue (faderLocked[(size_t) masterOutputFaderIndex].load() ? 1.0f : rawMasterOutput);

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = smoothedMasterOutput.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= gain;
    }
}

juce::AudioProcessorEditor* YuViGlowAudioProcessor::createEditor()
{
    return new YuViGlowAudioProcessorEditor (*this);
}

void YuViGlowAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("YuViGlowState");
    state.appendChild (apvts.copyState(), nullptr);
    state.setProperty ("triggerNote", triggerNoteNumber.load(), nullptr);
    state.setProperty ("triggerChannel", triggerMidiChannel.load(), nullptr);
    state.setProperty ("gainCc", gainCcNumber.load(), nullptr);
    state.setProperty ("gainChannel", gainMidiChannel.load(), nullptr);
    state.setProperty ("gainLocked", gainLocked.load(), nullptr);
    state.setProperty ("midiDeviceName", getCurrentMidiInputDisplayName(), nullptr);
    state.setProperty ("tapTempoNote", tapTempoNoteNumber.load(), nullptr);
    state.setProperty ("tapTempoChannel", tapTempoMidiChannel.load(), nullptr);

    for (int i = 0; i < numPads; ++i)
        state.setProperty ("pad" + juce::String (i), padBank.getAssignment (i), nullptr);
    for (int i = 0; i < numFaders; ++i)
        state.setProperty ("fader" + juce::String (i), faderBank.getAssignment (i), nullptr);
    for (int i = 0; i < numKnobs; ++i)
        state.setProperty ("knob" + juce::String (i), knobBank.getAssignment (i), nullptr);

    // Lock state and the per-row momentary toggles are user preferences, so
    // they ride along with host state. Cue points deliberately do not — they
    // belong to whichever track is loaded, not to the session (plan/issues/22).
    for (int i = 0; i < numFaders; ++i)
        state.setProperty ("faderLock" + juce::String (i), faderLocked[(size_t) i].load(), nullptr);
    for (int i = 0; i < numMomentaryRows; ++i)
        state.setProperty ("momentaryRow" + juce::String (i), momentaryRowEnabled[(size_t) i].load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void YuViGlowAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;

    triggerNoteNumber.store ((int) state.getProperty ("triggerNote", -1));
    triggerMidiChannel.store ((int) state.getProperty ("triggerChannel", -1));
    gainCcNumber.store ((int) state.getProperty ("gainCc", -1));
    gainMidiChannel.store ((int) state.getProperty ("gainChannel", -1));

    const auto deviceName = state.getProperty ("midiDeviceName").toString();

    auto paramsTree = state.getChildWithName (apvts.state.getType());
    if (paramsTree.isValid())
        apvts.replaceState (paramsTree);

    setGainLocked ((bool) state.getProperty ("gainLocked", false));
    tapTempoNoteNumber.store ((int) state.getProperty ("tapTempoNote", -1));
    tapTempoMidiChannel.store ((int) state.getProperty ("tapTempoChannel", -1));

    for (int i = 0; i < numPads; ++i)
        padBank.setAssignment (i, (int) state.getProperty ("pad" + juce::String (i), -1));
    for (int i = 0; i < numFaders; ++i)
        faderBank.setAssignment (i, (int) state.getProperty ("fader" + juce::String (i), -1));
    for (int i = 0; i < numKnobs; ++i)
        knobBank.setAssignment (i, (int) state.getProperty ("knob" + juce::String (i), -1));

    for (int i = 0; i < numFaders; ++i)
        setFaderLocked (i, (bool) state.getProperty ("faderLock" + juce::String (i), false));
    for (int i = 0; i < numMomentaryRows; ++i)
        momentaryRowEnabled[(size_t) i].store ((bool) state.getProperty ("momentaryRow" + juce::String (i), true));

    if (deviceName.isNotEmpty())
    {
        for (auto& device : getClassifiedMidiInputs())
        {
            if (device.displayName == deviceName)
            {
                setMidiInputDevice (device);
                break;
            }
        }
    }
}

void YuViGlowAudioProcessor::loadAudioFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
    {
        const juce::ScopedLock sl (bufferLock);
        loadErrorMessage = "Couldn't load \"" + file.getFileName() + "\" — unsupported or corrupt file";
        return;
    }

    const int numChannels = static_cast<int> (reader->numChannels);
    const int numSourceSamples = static_cast<int> (reader->lengthInSamples);

    juce::AudioBuffer<float> sourceBuffer (numChannels, numSourceSamples);
    reader->read (&sourceBuffer, 0, numSourceSamples, 0, true, true);

    const double sourceRate = reader->sampleRate;
    const double targetRate = getSampleRate() > 0.0 ? getSampleRate() : sourceRate;
    const double ratio = targetRate > 0.0 ? sourceRate / targetRate : 1.0;

    juce::AudioBuffer<float> newBuffer;

    if (std::abs (ratio - 1.0) < 0.0001)
    {
        newBuffer = std::move (sourceBuffer);
    }
    else
    {
        const int numTargetSamples = static_cast<int> (std::ceil (numSourceSamples / ratio));
        newBuffer.setSize (numChannels, numTargetSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.reset();
            interpolator.process (ratio, sourceBuffer.getReadPointer (ch), newBuffer.getWritePointer (ch), numTargetSamples);
        }
    }

    {
        const juce::ScopedLock sl (bufferLock);
        sampleBuffer = std::move (newBuffer);
        loadedFileName = file.getFileName();
        loadErrorMessage.clear();
    }

    stopPlayback();
    resetCuePointsForLoadedFile();

    // Auto-detect runs off the audio/message thread — analysis can take a
    // real amount of time and must never block either (AGENTS.md's
    // audio-thread-discipline rule). Copies the buffer under lock (fast)
    // rather than holding the lock through the analysis itself (slow).
    tempoAnalysisPool.addJob ([this]
    {
        juce::AudioBuffer<float> bufferCopy;
        double sr = 44100.0;

        {
            const juce::ScopedLock sl (bufferLock);
            if (sampleBuffer.getNumSamples() <= 0)
                return;
            bufferCopy = sampleBuffer;
            sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        }

        if (tempoDetector == nullptr)
            return;

        const auto beats = tempoDetector->detectBeatTimestamps (bufferCopy, sr);
        if (beats.size() < 2)
            return;

        // Average across the whole detection rather than trusting any single
        // inter-beat gap — real detections jitter. Since plan/issues/22 retired
        // beat-aligned loop regions, the BPM number itself is now the entire
        // product of auto-detect, so it's worth deriving properly. (It also
        // never used to reach the UI at all: the old code only wrote loop
        // regions and left currentBpm at 0, so the BPM box stayed blank after
        // an auto-detect.)
        const double span = beats.back() - beats.front();
        const double averageInterval = span / (double) (beats.size() - 1);
        if (averageInterval <= 0.0)
            return;

        const double bpm = 60.0 / averageInterval;
        if (bpm >= 20.0 && bpm <= 400.0)
            currentBpm.store (bpm);
    });
}

juce::String YuViGlowAudioProcessor::getLoadedFileName() const
{
    const juce::ScopedLock sl (bufferLock);
    return loadedFileName;
}

juce::String YuViGlowAudioProcessor::getLoadError() const
{
    const juce::ScopedLock sl (bufferLock);
    return loadErrorMessage;
}

bool YuViGlowAudioProcessor::isFileLoaded() const
{
    const juce::ScopedLock sl (bufferLock);
    return sampleBuffer.getNumSamples() > 0;
}

void YuViGlowAudioProcessor::startPlaybackFromSample (int startSample, int owningPadIndex)
{
    if (! isFileLoaded())
        return;

    pendingStartSample.store (juce::jmax (0, startSample));
    activePlaybackPadIndex.store (owningPadIndex);
    stopRequested.store (false);   // cancel any stop the audio thread hasn't consumed yet
    triggerRequested.store (true);
}

void YuViGlowAudioProcessor::triggerPlayback()
{
    // No owning pad: STOP is the only thing that ends this, which is what
    // the on-screen trigger and the legacy learned trigger note both want.
    startPlaybackFromSample (0, -1);
}

void YuViGlowAudioProcessor::stopPlayback()
{
    stopRequested.store (true);
    activePlaybackPadIndex.store (-1);
}

int YuViGlowAudioProcessor::getPlaybackPositionSamples() const
{
    const double pos = playbackPosition.load();
    return pos < 0.0 ? -1 : (int) pos;
}

int YuViGlowAudioProcessor::getCuePointSample (int cueIndex) const
{
    if (cueIndex < 0 || cueIndex >= numCuePoints)
        return -1;
    return cuePointSample[(size_t) cueIndex].load();
}

bool YuViGlowAudioProcessor::isCuePointSet (int cueIndex) const
{
    return getCuePointSample (cueIndex) >= 0;
}

void YuViGlowAudioProcessor::resetCuePointsForLoadedFile()
{
    for (auto& c : cuePointSample)
        c.store (-1);

    int lengthSamples = 0;
    {
        const juce::ScopedLock sl (bufferLock);
        lengthSamples = sampleBuffer.getNumSamples();
    }

    if (lengthSamples <= 0)
        return;

    // sampleBuffer is already resampled to the device rate by loadAudioFile,
    // so the host sample rate is the right one to convert seconds with.
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    const int lastSample = juce::jmax (0, lengthSamples - 1);

    cuePointSample[0].store (0);
    // Clamped rather than left pointing past the end, so a track shorter
    // than 8 seconds still gets a usable cue 2 instead of a dead pad.
    cuePointSample[1].store (juce::jmin ((int) std::llround (autoCueTwoSeconds * sr), lastSample));
}

bool YuViGlowAudioProcessor::isMomentaryRowEnabled (int rowOffset) const
{
    if (rowOffset < 0 || rowOffset >= numMomentaryRows)
        return false;
    return momentaryRowEnabled[(size_t) rowOffset].load();
}

void YuViGlowAudioProcessor::setMomentaryRowEnabled (int rowOffset, bool shouldBeMomentary)
{
    if (rowOffset < 0 || rowOffset >= numMomentaryRows)
        return;
    momentaryRowEnabled[(size_t) rowOffset].store (shouldBeMomentary);
}

bool YuViGlowAudioProcessor::isPadMomentary (int padIndex) const
{
    if (padIndex < firstMomentaryPadIndex || padIndex >= numPads)
        return false;
    return isMomentaryRowEnabled ((padIndex - firstMomentaryPadIndex) / padsPerRow);
}

void YuViGlowAudioProcessor::triggerCuePad (int padIndex)
{
    if (padIndex < 0 || padIndex >= numPads || ! isFileLoaded())
        return;

    const int cueIndex = cueIndexForPad (padIndex);
    const int cueSample = cuePointSample[(size_t) cueIndex].load();

    if (cueSample < 0)
    {
        // Empty cue: mark it wherever the playhead currently is and leave
        // playback running — the pad is armed for next time rather than
        // doing anything audible now. With nothing playing there's no
        // position to capture, so this is a no-op (see plan/issues/22 for why
        // that's preferred over silently aliasing it to the track start).
        const int position = getPlaybackPositionSamples();
        if (position >= 0)
            cuePointSample[(size_t) cueIndex].store (position);

        return;
    }

    startPlaybackFromSample (cueSample, padIndex);
}

void YuViGlowAudioProcessor::releaseCuePad (int padIndex)
{
    if (! isPadMomentary (padIndex))
        return;

    // Only stop playback this pad actually started — releasing pad 9 must
    // not cut a track that pad 1 latched.
    if (activePlaybackPadIndex.load() == padIndex)
        stopPlayback();
}

void YuViGlowAudioProcessor::setManualBpm (double bpm)
{
    if (bpm < 20.0 || bpm > 400.0)
        return;

    currentBpm.store (bpm);
}

void YuViGlowAudioProcessor::registerTapTempo()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double last = lastTapTimestampMs.exchange (now);

    if (last <= 0.0)
        return;

    const double interval = now - last;
    if (interval < 200.0 || interval > 2000.0) // outside ~30-300 BPM: treat as a fresh tap sequence, not a tempo
        return;

    double averageMs = 0.0;
    {
        const juce::ScopedLock sl (tapTempoLock);
        tapIntervalsMs.push_back (interval);
        if (tapIntervalsMs.size() > 4)
            tapIntervalsMs.erase (tapIntervalsMs.begin());

        double sum = 0.0;
        for (double v : tapIntervalsMs)
            sum += v;
        averageMs = sum / (double) tapIntervalsMs.size();
    }

    setManualBpm (60000.0 / averageMs);
}

void YuViGlowAudioProcessor::armLearnTapTempo() { learningTapTempo.store (true); }

juce::String YuViGlowAudioProcessor::getTapTempoDescription() const
{
    const int note = tapTempoNoteNumber.load();
    if (note < 0)
        return "Not learned";
    return "Note " + juce::String (note) + " (ch " + juce::String (tapTempoMidiChannel.load()) + ")";
}

void YuViGlowAudioProcessor::setGainLocked (bool shouldLock)
{
    gainLocked.store (shouldLock);

    if (! shouldLock)
        return;

    if (auto* param = apvts.getParameter ("inputGain"))
        param->setValueNotifyingHost (0.5f); // parameter range is fixed [0, 2.0] — 0.5 normalized = 1.0x = unity

    // Snap the on-screen knob to its midpoint too, so it stops displaying a
    // position the audio isn't actually using (plan/issues/22).
    knobValues[(size_t) inputGainKnobIndex].store (0.5f);
}

void YuViGlowAudioProcessor::setFaderLocked (int index, bool shouldLock)
{
    if (index < 0 || index >= numFaders)
        return;

    faderLocked[(size_t) index].store (shouldLock);

    if (! shouldLock)
        return;

    // Every lock snaps its fader's displayed position to the midpoint; what
    // "neutral" then means depends on the fader's role.
    faderValues[(size_t) index].store (0.5f);

    if (index == masterOutputFaderIndex)
    {
        if (auto* param = apvts.getParameter ("masterOutput"))
            param->setValueNotifyingHost (0.5f); // range [0, 2.0] — 0.5 normalized = unity
    }
    else if (index == pitchAdjustFaderIndex)
    {
        pitchAdjustPercent.store (0.0f);
    }
}

bool YuViGlowAudioProcessor::isFaderLocked (int index) const
{
    if (index < 0 || index >= numFaders)
        return false;
    return faderLocked[(size_t) index].load();
}

void YuViGlowAudioProcessor::refreshPitchFromFader()
{
    if (faderLocked[(size_t) pitchAdjustFaderIndex].load())
    {
        pitchAdjustPercent.store (0.0f);
        return;
    }

    const float normalized = faderValues[(size_t) pitchAdjustFaderIndex].load();
    float percent = (normalized - 0.5f) * 2.0f * pitchAdjustRangePercent;

    // Centre detent — see pitchAdjustDetentPercent in the header.
    if (std::abs (percent) < pitchAdjustDetentPercent)
        percent = 0.0f;

    pitchAdjustPercent.store (juce::jlimit (-pitchAdjustRangePercent, pitchAdjustRangePercent, percent));
}

void YuViGlowAudioProcessor::applyBestGuessMappingIfUnlearned()
{
    constexpr int bestGuessTriggerNote = 36; // near-universal "pad 1" note across MPC-style controllers
    constexpr int bestGuessGainCc = 7;       // GM Volume — common factory default for fader 1

    if (triggerNoteNumber.load() < 0)
    {
        triggerNoteNumber.store (bestGuessTriggerNote);
        triggerMidiChannel.store (-1);
    }

    if (gainCcNumber.load() < 0)
    {
        gainCcNumber.store (bestGuessGainCc);
        gainMidiChannel.store (-1);
    }
}

void YuViGlowAudioProcessor::armLearnTriggerPad() { learningTrigger.store (true); }
void YuViGlowAudioProcessor::armLearnGainFader() { learningGain.store (true); }

juce::String YuViGlowAudioProcessor::getTriggerDescription() const
{
    const int note = triggerNoteNumber.load();
    if (note < 0)
        return "Not learned";
    return "Note " + juce::String (note) + " (ch " + juce::String (triggerMidiChannel.load()) + ")";
}

juce::String YuViGlowAudioProcessor::getGainFaderDescription() const
{
    const int cc = gainCcNumber.load();
    if (cc < 0)
        return "Not learned";
    return "CC " + juce::String (cc) + " (ch " + juce::String (gainMidiChannel.load()) + ")";
}

float YuViGlowAudioProcessor::getPadTouchAmount (int padIndex) const
{
    if (padIndex < 0 || padIndex >= numPads)
        return 0.0f;

    if (! padHeld[(size_t) padIndex].load())
        return 0.0f;

    const int velocity = padVelocities[(size_t) padIndex].load();
    if (velocity <= 0)
        return 0.0f;

    // Sustained, not time-decayed — full purple for as long as the pad is
    // physically held down, matching a real pad's sustain rather than a
    // fixed-length flash.
    return (float) (velocity / 127.0);
}

float YuViGlowAudioProcessor::getPadAfterglowAmount (int padIndex) const
{
    if (padIndex < 0 || padIndex >= numPads)
        return 0.0f;

    if (padHeld[(size_t) padIndex].load())
        return 0.0f; // teal only begins once the pad is released

    const int velocity = padVelocities[(size_t) padIndex].load();
    if (velocity <= 0)
        return 0.0f;

    constexpr double fadeMs = 1800.0;
    const double elapsed = juce::Time::getMillisecondCounterHiRes() - padReleaseTimestampMs[(size_t) padIndex].load();
    const double fade = juce::jlimit (0.0, 1.0, 1.0 - (elapsed / fadeMs));

    return (float) (fade * (velocity / 127.0));
}

void YuViGlowAudioProcessor::setEditingMappings (bool shouldEdit)
{
    editingMappings.store (shouldEdit);
    padBank.armSlot (-1);
    faderBank.armSlot (-1);
    knobBank.armSlot (-1);
}

float YuViGlowAudioProcessor::getFaderValue (int index) const
{
    if (index < 0 || index >= numFaders)
        return 0.0f;
    return faderValues[(size_t) index].load();
}

float YuViGlowAudioProcessor::getKnobValue (int index) const
{
    if (index < 0 || index >= numKnobs)
        return 0.0f;
    return knobValues[(size_t) index].load();
}

void YuViGlowAudioProcessor::resetAllMappings()
{
    padBank.reset();
    for (auto& v : padVelocities)
        v.store (0);
    for (auto& v : padHeld)
        v.store (false);
    for (auto& v : padReleaseTimestampMs)
        v.store (0.0);

    faderBank.reset();
    for (auto& v : faderValues)
        v.store (0.0f);

    knobBank.reset();
    for (auto& v : knobValues)
        v.store (0.0f);
}

juce::File YuViGlowAudioProcessor::getPresetFileForCurrentDevice() const
{
    auto baseDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("YuViGlow")
                       .getChildFile ("presets");

    juce::String fileName;
    switch (midiDeviceManager.getCurrentCategory())
    {
        case DetectedDeviceCategory::code49: fileName = "code49"; break;
        case DetectedDeviceCategory::mpd226: fileName = "mpd226"; break;
        case DetectedDeviceCategory::custom: fileName = "custom-" + juce::File::createLegalFileName (getCurrentMidiInputDisplayName()); break;
    }

    return baseDir.getChildFile (fileName + ".xml");
}

bool YuViGlowAudioProcessor::saveMappingPresetForCurrentDevice()
{
    if (getCurrentMidiInputDisplayName().isEmpty())
        return false;

    juce::ValueTree preset ("YuViGlowMappingPreset");
    preset.setProperty ("triggerNote", triggerNoteNumber.load(), nullptr);
    preset.setProperty ("triggerChannel", triggerMidiChannel.load(), nullptr);
    preset.setProperty ("gainCc", gainCcNumber.load(), nullptr);
    preset.setProperty ("gainChannel", gainMidiChannel.load(), nullptr);
    preset.setProperty ("tapTempoNote", tapTempoNoteNumber.load(), nullptr);
    preset.setProperty ("tapTempoChannel", tapTempoMidiChannel.load(), nullptr);

    for (int i = 0; i < numPads; ++i)
        preset.setProperty ("pad" + juce::String (i), padBank.getAssignment (i), nullptr);
    for (int i = 0; i < numFaders; ++i)
        preset.setProperty ("fader" + juce::String (i), faderBank.getAssignment (i), nullptr);
    for (int i = 0; i < numKnobs; ++i)
        preset.setProperty ("knob" + juce::String (i), knobBank.getAssignment (i), nullptr);

    auto file = getPresetFileForCurrentDevice();
    file.getParentDirectory().createDirectory();

    if (auto xml = preset.createXml())
        return xml->writeTo (file);

    return false;
}

bool YuViGlowAudioProcessor::loadMappingPresetForCurrentDevice()
{
    if (getCurrentMidiInputDisplayName().isEmpty())
        return false;

    auto file = getPresetFileForCurrentDevice();
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr)
        return false;

    auto preset = juce::ValueTree::fromXml (*xml);
    if (! preset.isValid())
        return false;

    triggerNoteNumber.store ((int) preset.getProperty ("triggerNote", -1));
    triggerMidiChannel.store ((int) preset.getProperty ("triggerChannel", -1));
    gainCcNumber.store ((int) preset.getProperty ("gainCc", -1));
    gainMidiChannel.store ((int) preset.getProperty ("gainChannel", -1));
    tapTempoNoteNumber.store ((int) preset.getProperty ("tapTempoNote", -1));
    tapTempoMidiChannel.store ((int) preset.getProperty ("tapTempoChannel", -1));

    for (int i = 0; i < numPads; ++i)
        padBank.setAssignment (i, (int) preset.getProperty ("pad" + juce::String (i), -1));
    for (int i = 0; i < numFaders; ++i)
        faderBank.setAssignment (i, (int) preset.getProperty ("fader" + juce::String (i), -1));
    for (int i = 0; i < numKnobs; ++i)
        knobBank.setAssignment (i, (int) preset.getProperty ("knob" + juce::String (i), -1));

    return true;
}

void YuViGlowAudioProcessor::processIncomingMidi (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        const int note = message.getNoteNumber();
        const int padIndex = padBank.handleIncomingIdentifier (note, editingMappings.load());

        if (padIndex >= 0)
        {
            padVelocities[(size_t) padIndex].store (message.getVelocity());
            padHeld[(size_t) padIndex].store (true);

            // Every pad is a cue pad now (plan/issues/22). Latch vs. hold is
            // decided on *release*, not here — pressing pad 1 and pressing
            // pad 9 do exactly the same thing; only pad 9 also stops when
            // it's let go.
            if (! editingMappings.load())
                triggerCuePad (padIndex);
        }

        if (learningTrigger.exchange (false))
        {
            triggerNoteNumber.store (note);
            triggerMidiChannel.store (message.getChannel());
            return;
        }

        if (note == triggerNoteNumber.load()
            && (triggerMidiChannel.load() <= 0 || message.getChannel() == triggerMidiChannel.load()))
        {
            triggerPlayback();
        }

        if (learningTapTempo.exchange (false))
        {
            tapTempoNoteNumber.store (note);
            tapTempoMidiChannel.store (message.getChannel());
            return;
        }

        if (note == tapTempoNoteNumber.load()
            && (tapTempoMidiChannel.load() <= 0 || message.getChannel() == tapTempoMidiChannel.load()))
        {
            registerTapTempo();
        }
    }
    else if (message.isNoteOff())
    {
        // Read-only lookup (findSlotForIdentifier, not handleIncomingIdentifier)
        // — a release should never trigger a mapping-editor auto-fill.
        const int padIndex = padBank.findSlotForIdentifier (message.getNoteNumber());
        if (padIndex >= 0)
        {
            padHeld[(size_t) padIndex].store (false);
            padReleaseTimestampMs[(size_t) padIndex].store (juce::Time::getMillisecondCounterHiRes());

            if (! editingMappings.load())
                releaseCuePad (padIndex);
        }
    }
    else if (message.isController())
    {
        const int cc = message.getControllerNumber();
        const float ccNormalized = message.getControllerValue() / 127.0f;
        const bool editing = editingMappings.load();

        // Faders and knobs both just send a generic CC number — nothing in
        // the MIDI protocol tells them apart. If a CC is brand new to both
        // banks, only let ONE of them claim it per message (fader gets first
        // look); otherwise a single physical control's first touch could
        // auto-fill a slot in both banks at once (real bug, found via
        // hardware testing 2026-08-08 — see plan/issues/11 for the writeup).
        const bool ccWasUnknownToEitherBank = ! faderBank.isIdentifierAssigned (cc) && ! knobBank.isIdentifierAssigned (cc);

        const int faderSlot = faderBank.handleIncomingIdentifier (cc, editing);
        if (faderSlot >= 0 && ! faderLocked[(size_t) faderSlot].load())
        {
            faderValues[(size_t) faderSlot].store (ccNormalized);

            // Fixed fader roles (plan/issues/22). A locked fader is skipped
            // entirely by the guard above, so a bumped physical fader can't
            // quietly undo its own lock.
            if (faderSlot == masterOutputFaderIndex)
            {
                if (auto* param = apvts.getParameter ("masterOutput"))
                    param->setValueNotifyingHost (ccNormalized);
            }
            else if (faderSlot == pitchAdjustFaderIndex)
            {
                refreshPitchFromFader();
            }
        }

        const bool faderJustClaimedFreshCc = ccWasUnknownToEitherBank && faderSlot >= 0;

        if (! faderJustClaimedFreshCc)
        {
            const int knobSlot = knobBank.handleIncomingIdentifier (cc, editing);
            if (knobSlot >= 0)
            {
                knobValues[(size_t) knobSlot].store (ccNormalized);

                // Knob 0 is the input trim. The legacy learned gain CC below
                // still works too, so device presets saved before this became
                // a fixed knob role keep functioning.
                if (knobSlot == inputGainKnobIndex && ! gainLocked.load())
                    if (auto* param = apvts.getParameter ("inputGain"))
                        param->setValueNotifyingHost (ccNormalized);
            }
        }

        if (learningGain.exchange (false))
        {
            gainCcNumber.store (cc);
            gainMidiChannel.store (message.getChannel());
            return;
        }

        if (! gainLocked.load()
            && cc == gainCcNumber.load()
            && (gainMidiChannel.load() <= 0 || message.getChannel() == gainMidiChannel.load()))
        {
            if (auto* param = apvts.getParameter ("inputGain"))
                param->setValueNotifyingHost (ccNormalized);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YuViGlowAudioProcessor();
}
