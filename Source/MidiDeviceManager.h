#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

// Detects, classifies, and connects to MIDI controllers (Code 49, MPD226, or
// anything else), forwarding every incoming message to a single callback.
// Extracted out of PluginProcessor as its own concern: "which physical MIDI
// ports are we listening to" is a distinct responsibility from "what do we
// do with the messages," which the processor still owns.
//
// Opens its own direct connection to the device rather than relying on
// host-forwarded MIDI — see AGENTS.md's "Plugins get hardware control by
// opening their own direct MIDI connection" ground rule; this class is
// where that actually happens.
class MidiDeviceManager : private juce::MidiInputCallback
{
public:
    enum class DeviceCategory { code49, mpd226, custom };

    // One hardware controller can expose several MIDI ports at once (the
    // Akai MPD226 shows up as four on Linux/ALSA: Port A, Port B, MIDI,
    // Remote) — all ports belonging to the same logical device are grouped
    // here so callers see one entry per physical controller, not one per port.
    struct DetectedDevice
    {
        std::vector<juce::MidiDeviceInfo> ports;
        DeviceCategory category;
        juce::String displayName;
    };

    using MessageCallback = std::function<void (const juce::MidiMessage&)>;

    explicit MidiDeviceManager (MessageCallback callbackIn) : callback (std::move (callbackIn)) {}
    ~MidiDeviceManager() override { disconnect(); }

    // Classifies whatever's currently plugged in: at most one Code 49 entry,
    // one MPD226 entry, and one "Custom" entry standing in for the first
    // unrecognized device found (see plan/issues/12-mvp-v0.md for why only one).
    std::vector<DetectedDevice> getClassifiedInputs() const;

    // Opens every port belonging to this device simultaneously, so a pad on
    // one port and a fader on another (or vice versa) are both heard — we
    // don't know or want to hardcode which of a device's ports carries what.
    void connect (const DetectedDevice& device);
    void disconnect();

    juce::String getCurrentDisplayName() const { return currentDisplayName; }
    DeviceCategory getCurrentCategory() const { return currentCategory; }

private:
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        if (callback)
            callback (message);
    }

    MessageCallback callback;
    std::vector<std::unique_ptr<juce::MidiInput>> midiInputs;
    juce::String currentDisplayName;
    DeviceCategory currentCategory { DeviceCategory::custom };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDeviceManager)
};
