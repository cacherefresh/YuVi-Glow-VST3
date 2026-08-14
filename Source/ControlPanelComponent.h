#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ToggleCheckbox.h"
#include <array>

// Right-hand control column matching the MPD226's physical layout: 4 knobs
// on top, 4 vertical faders below. Normally shows live position: a purple
// arc gauge per knob, a teal fill bar per fader. While the processor's
// global mapping editor is active (YuViGlowAudioProcessor::isEditingMappings()),
// this switches to the same flat red/yellow/green assignment-state display
// as PadGridComponent, and each knob/fader becomes clickable to arm it for
// learning (YuViGlowAudioProcessor::armLearnKnobSlot()/armLearnFaderSlot()).
//
// A row of lock checkboxes sits under the faders (plan/issues/22): locking one
// snaps it to its neutral position — unity for the master output on fader 1,
// 0.00% for the pitch fader on fader 2, the midpoint for the two spares —
// and makes it ignore incoming MIDI so a bumped physical fader can't undo it.
class ControlPanelComponent : public juce::Component,
                               private juce::Timer
{
public:
    // Height of the lock-checkbox strip along the bottom.
    static constexpr int lockRowHeight = 22;

    explicit ControlPanelComponent (YuViGlowAudioProcessor& p) : processor (p)
    {
        static const char* lockTooltips[] = {
            "Lock master output at 0dB",
            "Lock pitch at 0.00%",
            "Lock fader 3 at centre",
            "Lock fader 4 at centre"
        };

        for (int i = 0; i < YuViGlowAudioProcessor::numFaders; ++i)
        {
            auto& button = faderLockButtons[(size_t) i];
            button.setToggleState (processor.isFaderLocked (i), juce::dontSendNotification);
            button.setTooltip (lockTooltips[i]);
            button.onClick = [this, i] { processor.setFaderLocked (i, faderLockButtons[(size_t) i].getToggleState()); };
            addAndMakeVisible (button);
        }

        startTimerHz (30);
    }

    void resized() override
    {
        auto lockRow = getLocalBounds().removeFromBottom (lockRowHeight);
        const int cellW = lockRow.getWidth() / YuViGlowAudioProcessor::numFaders;

        for (int i = 0; i < YuViGlowAudioProcessor::numFaders; ++i)
            faderLockButtons[(size_t) i].setBounds (lockRow.getX() + i * cellW + (cellW - yuviglow::checkbox::width) / 2,
                                                     lockRow.getY() + (lockRowHeight - yuviglow::checkbox::height) / 2,
                                                     yuviglow::checkbox::width, yuviglow::checkbox::height);
    }

    void paint (juce::Graphics& g) override
    {
        const auto purple = juce::Colour (0xff9b59ff);
        const auto teal = juce::Colour (0xff2dd4bf);
        const bool editing = processor.isEditingMappings();

        auto area = getLocalBounds().toFloat();
        area.removeFromBottom ((float) lockRowHeight); // reserved for the lock checkboxes
        const auto knobArea = area.removeFromTop (area.getHeight() * 0.4f);
        const auto faderArea = area;

        constexpr int numKnobs = YuViGlowAudioProcessor::numKnobs;
        const int learningKnob = processor.getLearningKnobSlot();
        const float knobCellW = knobArea.getWidth() / (float) numKnobs;

        for (int i = 0; i < numKnobs; ++i)
        {
            const auto cell = juce::Rectangle<float> (knobArea.getX() + (float) i * knobCellW,
                                                        knobArea.getY(), knobCellW, knobArea.getHeight())
                                   .reduced (6.0f);
            const float diameter = juce::jmin (cell.getWidth(), cell.getHeight());
            const auto knobBounds = juce::Rectangle<float> (diameter, diameter).withCentre (cell.getCentre());

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (knobBounds);

            if (editing)
            {
                juce::Colour stateColour = juce::Colours::red.withAlpha (0.55f);
                if (i == learningKnob)
                    stateColour = juce::Colours::yellow.withAlpha (0.85f);
                else if (processor.isKnobAssigned (i))
                    stateColour = juce::Colours::limegreen.withAlpha (0.7f);

                g.setColour (stateColour);
                g.fillEllipse (knobBounds.reduced (4.0f));
            }
            else
            {
                const float value = processor.getKnobValue (i);
                constexpr float startAngle = juce::MathConstants<float>::pi * 1.2f;
                constexpr float endAngle = juce::MathConstants<float>::pi * 2.8f;
                const float angle = startAngle + value * (endAngle - startAngle);

                juce::Path arc;
                arc.addArc (knobBounds.getX(), knobBounds.getY(), knobBounds.getWidth(), knobBounds.getHeight(),
                            startAngle, angle, true);
                g.setColour (purple);
                g.strokePath (arc, juce::PathStrokeType (3.0f));
            }

            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawEllipse (knobBounds, 1.0f);
        }

        constexpr int numFaders = YuViGlowAudioProcessor::numFaders;
        const int learningFader = processor.getLearningFaderSlot();
        const float faderCellW = faderArea.getWidth() / (float) numFaders;

        for (int i = 0; i < numFaders; ++i)
        {
            const auto cell = juce::Rectangle<float> (faderArea.getX() + (float) i * faderCellW,
                                                        faderArea.getY(), faderCellW, faderArea.getHeight())
                                   .reduced (6.0f);

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (cell, 4.0f);

            if (editing)
            {
                juce::Colour stateColour = juce::Colours::red.withAlpha (0.55f);
                if (i == learningFader)
                    stateColour = juce::Colours::yellow.withAlpha (0.85f);
                else if (processor.isFaderAssigned (i))
                    stateColour = juce::Colours::limegreen.withAlpha (0.7f);

                g.setColour (stateColour);
                g.fillRoundedRectangle (cell.reduced (3.0f), 3.0f);
            }
            else
            {
                const float value = processor.getFaderValue (i);
                const auto fillRect = cell.withTop (cell.getBottom() - cell.getHeight() * value);
                // A locked fader is drawn dimmed so it reads as held rather
                // than just happening to sit at the midpoint.
                g.setColour (teal.withAlpha (processor.isFaderLocked (i) ? 0.3f : 0.8f));
                g.fillRoundedRectangle (fillRect, 4.0f);
            }

            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawRoundedRectangle (cell, 4.0f, 1.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! processor.isEditingMappings())
            return;

        auto bounds = getLocalBounds().toFloat();
        bounds.removeFromBottom ((float) lockRowHeight);
        if (e.position.y >= bounds.getBottom())
            return; // in the lock-checkbox strip, not on a control

        const float knobAreaHeight = bounds.getHeight() * 0.4f;

        if (e.position.y < knobAreaHeight)
        {
            constexpr int numKnobs = YuViGlowAudioProcessor::numKnobs;
            const float knobCellW = bounds.getWidth() / (float) numKnobs;
            if (knobCellW <= 0.0f)
                return;
            processor.armLearnKnobSlot (juce::jlimit (0, numKnobs - 1, (int) (e.position.x / knobCellW)));
        }
        else
        {
            constexpr int numFaders = YuViGlowAudioProcessor::numFaders;
            const float faderCellW = bounds.getWidth() / (float) numFaders;
            if (faderCellW <= 0.0f)
                return;
            processor.armLearnFaderSlot (juce::jlimit (0, numFaders - 1, (int) (e.position.x / faderCellW)));
        }
    }

private:
    void timerCallback() override
    {
        for (int i = 0; i < YuViGlowAudioProcessor::numFaders; ++i)
            yuviglow::checkbox::mirror (faderLockButtons[(size_t) i], processor.isFaderLocked (i));

        repaint();
    }

    YuViGlowAudioProcessor& processor;
    std::array<juce::ToggleButton, (size_t) YuViGlowAudioProcessor::numFaders> faderLockButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelComponent)
};
