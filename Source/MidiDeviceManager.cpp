#include "MidiDeviceManager.h"

std::vector<MidiDeviceManager::DetectedDevice> MidiDeviceManager::getClassifiedInputs() const
{
    std::vector<DetectedDevice> result;

    auto findOrAdd = [&result] (DeviceCategory category, const juce::String& displayName) -> DetectedDevice&
    {
        for (auto& d : result)
            if (d.category == category && d.displayName == displayName)
                return d;

        result.push_back ({ {}, category, displayName });
        return result.back();
    };

    juce::String customDeviceName;
    bool customNameChosen = false;

    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        if (device.name.containsIgnoreCase ("code 49") || device.name.containsIgnoreCase ("code49"))
        {
            findOrAdd (DeviceCategory::code49, "M-Audio Code 49").ports.push_back (device);
        }
        else if (device.name.containsIgnoreCase ("mpd226"))
        {
            findOrAdd (DeviceCategory::mpd226, "Akai MPD226").ports.push_back (device);
        }
        else
        {
            if (! customNameChosen)
            {
                customDeviceName = device.name;
                customNameChosen = true;
            }

            // Only fold in ports sharing the first unrecognized device's exact
            // name — a second, differently-named unrecognized device is simply
            // not shown, matching the "one third option" spec.
            if (device.name == customDeviceName)
                findOrAdd (DeviceCategory::custom, "Custom MIDI Device (" + customDeviceName + ")").ports.push_back (device);
        }
    }

    return result;
}

void MidiDeviceManager::connect (const DetectedDevice& device)
{
    disconnect();

    for (auto& port : device.ports)
    {
        if (auto input = juce::MidiInput::openDevice (port.identifier, this))
        {
            input->start();
            midiInputs.push_back (std::move (input));
        }
    }

    if (! midiInputs.empty())
    {
        currentDisplayName = device.displayName;
        currentCategory = device.category;
    }
}

void MidiDeviceManager::disconnect()
{
    for (auto& input : midiInputs)
        input->stop();

    midiInputs.clear();
    currentDisplayName.clear();
}
