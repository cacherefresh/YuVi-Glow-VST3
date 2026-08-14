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
    for (auto& v : padLoopStartSample)
        v.store (-1);
    for (auto& v : padLoopEndSample)
        v.store (-1);

    tempoDetector = std::make_unique<LibsonareTempoDetector>();
}

YuViGlowAudioProcessor::~YuViGlowAudioProcessor()
{
    disconnectMidiInput();
}

juce::AudioProcessorValueTreeState::ParameterLayout YuViGlowAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "VST MASTER Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f));

    return { params.begin(), params.end() };
}

void YuViGlowAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    smoothedInputGain.reset (sampleRate, 0.02);
    smoothedInputGain.setCurrentAndTargetValue (*apvts.getRawParameterValue ("inputGain"));
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
        playbackPosition.store (-1);
    }

    if (triggerRequested.exchange (false))
    {
        playbackPosition.store (0);
        playing.store (true);
    }

    const float rawInputGain = apvts.getRawParameterValue ("inputGain")->load();
    smoothedInputGain.setTargetValue (gainLocked.load() ? 1.0f : rawInputGain);

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = smoothedInputGain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= gain;
    }

    if (playing.load())
    {
        const juce::ScopedLock sl (bufferLock);
        const int bufferLength = sampleBuffer.getNumSamples();
        const int sampleChannels = sampleBuffer.getNumChannels();
        const bool looping = loopActive.load();
        // Non-looping playback plays to the end of the file; looping
        // playback is bounded by loopEndSample (clamped to the file length
        // in case the loop region was computed against a different file).
        const int playEnd = looping ? juce::jmin (loopEndSample.load(), bufferLength) : bufferLength;

        int pos = playbackPosition.load();
        int destOffset = 0;
        int remaining = numSamples;

        // A while-loop rather than a single copy so a loop region shorter
        // than one audio block (very high BPM, very short blocks) still
        // wraps correctly within a single processBlock call instead of
        // just truncating.
        while (remaining > 0 && bufferLength > 0 && pos >= 0 && pos < playEnd)
        {
            const int samplesToCopy = juce::jmin (remaining, playEnd - pos);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const int srcCh = juce::jmin (ch, sampleChannels - 1);
                const float* src = sampleBuffer.getReadPointer (srcCh, pos);
                float* dst = buffer.getWritePointer (ch) + destOffset;

                for (int i = 0; i < samplesToCopy; ++i)
                    dst[i] += src[i];
            }

            pos += samplesToCopy;
            destOffset += samplesToCopy;
            remaining -= samplesToCopy;

            if (pos >= playEnd)
            {
                if (looping)
                    pos = juce::jmax (0, loopStartSample.load());
                else
                    break;
            }
        }

        if (! looping && pos >= playEnd)
        {
            playing.store (false);
            playbackPosition.store (-1);
        }
        else
        {
            playbackPosition.store (pos);
        }
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
        return;

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
    }

    stopPlayback();

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
        if (! beats.empty())
            applyBeatGridToPads (beats);
    });
}

juce::String YuViGlowAudioProcessor::getLoadedFileName() const
{
    const juce::ScopedLock sl (bufferLock);
    return loadedFileName;
}

bool YuViGlowAudioProcessor::isFileLoaded() const
{
    const juce::ScopedLock sl (bufferLock);
    return sampleBuffer.getNumSamples() > 0;
}

void YuViGlowAudioProcessor::triggerPlayback()
{
    if (isFileLoaded())
    {
        loopActive.store (false);
        activeLoopPadIndex.store (-1);
        triggerRequested.store (true);
    }
}

void YuViGlowAudioProcessor::stopPlayback()
{
    stopRequested.store (true);
    loopActive.store (false);
    activeLoopPadIndex.store (-1);
}

bool YuViGlowAudioProcessor::hasPadLoopRegion (int padIndex) const
{
    if (padIndex < 0 || padIndex >= numPads)
        return false;
    return padLoopStartSample[(size_t) padIndex].load() >= 0
        && padLoopEndSample[(size_t) padIndex].load() > padLoopStartSample[(size_t) padIndex].load();
}

void YuViGlowAudioProcessor::triggerOrStopPadLoop (int padIndex)
{
    if (! hasPadLoopRegion (padIndex))
    {
        // No BPM loop region assigned to this pad — fall back to playing
        // the whole file from the start (restart-on-repress), same as the
        // old dedicated single-pad trigger. Means any pad bound via "Edit
        // MIDI Mapping" alone is immediately useful with zero BPM/tap-tempo
        // step required first — pads 0-3 only get the shorter beat-aligned
        // loop behavior below once a tempo source has actually populated
        // their loop region (see applyBeatGridToPads()).
        triggerPlayback();
        return;
    }

    if (activeLoopPadIndex.load() == padIndex)
    {
        // Same pad pressed again while its loop is playing — latched stop.
        stopPlayback();
        return;
    }

    const int start = padLoopStartSample[(size_t) padIndex].load();
    const int end = padLoopEndSample[(size_t) padIndex].load();

    activeLoopPadIndex.store (padIndex);
    loopStartSample.store (start);
    loopEndSample.store (end);
    loopActive.store (true);
    playbackPosition.store (start);
    playing.store (true);
    stopRequested.store (false);
    triggerRequested.store (false);
}

void YuViGlowAudioProcessor::applyBeatGridToPads (const std::vector<double>& beatTimestampsSeconds)
{
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;

    for (int pad = 0; pad < 4 && pad + 1 < (int) beatTimestampsSeconds.size(); ++pad)
    {
        const int startSample = (int) std::lround (beatTimestampsSeconds[(size_t) pad] * sr);
        const int endSample = (int) std::lround (beatTimestampsSeconds[(size_t) pad + 1] * sr);

        if (endSample > startSample)
        {
            padLoopStartSample[(size_t) pad].store (startSample);
            padLoopEndSample[(size_t) pad].store (endSample);
        }
    }
}

void YuViGlowAudioProcessor::setManualBpm (double bpm)
{
    if (bpm < 20.0 || bpm > 400.0)
        return;

    currentBpm.store (bpm);

    const double beatSeconds = 60.0 / bpm;
    std::vector<double> beats;
    for (int i = 0; i < 5; ++i)
        beats.push_back ((double) i * beatSeconds);

    applyBeatGridToPads (beats);
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

    if (shouldLock)
        if (auto* param = apvts.getParameter ("inputGain"))
            param->setValueNotifyingHost (0.5f); // parameter range is fixed [0, 2.0] — 0.5 normalized = 1.0x = unity
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

            if (! editingMappings.load())
                triggerOrStopPadLoop (padIndex);
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
        if (faderSlot >= 0)
            faderValues[(size_t) faderSlot].store (ccNormalized);

        const bool faderJustClaimedFreshCc = ccWasUnknownToEitherBank && faderSlot >= 0;

        if (! faderJustClaimedFreshCc)
        {
            const int knobSlot = knobBank.handleIncomingIdentifier (cc, editing);
            if (knobSlot >= 0)
                knobValues[(size_t) knobSlot].store (ccNormalized);
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
