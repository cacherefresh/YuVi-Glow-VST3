#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Right-hand control column matching the MPD226's physical layout: 4 knobs
// on top, 4 vertical faders below. Normally shows live position: a purple
// arc gauge per knob, a teal fill bar per fader. While the processor's
// global mapping editor is active (YuViGlowAudioProcessor::isEditingMappings()),
// this switches to the same flat red/yellow/green assignment-state display
// as PadGridComponent, and each knob/fader becomes clickable to arm it for
// learning (YuViGlowAudioProcessor::armLearnKnobSlot()/armLearnFaderSlot()).
class ControlPanelComponent : public juce::Component,
                               private juce::Timer
{
public:
    explicit ControlPanelComponent (YuViGlowAudioProcessor& p) : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto purple = juce::Colour (0xff9b59ff);
        const auto teal = juce::Colour (0xff2dd4bf);
        const bool editing = processor.isEditingMappings();

        auto area = getLocalBounds().toFloat();
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
                g.setColour (teal.withAlpha (0.8f));
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

        const auto bounds = getLocalBounds().toFloat();
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
    void timerCallback() override { repaint(); }

    YuViGlowAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelComponent)
};
