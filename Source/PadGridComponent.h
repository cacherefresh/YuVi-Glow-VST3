#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// 4x4 grid of the 16 pads. Normally shows live touch feedback: a purple
// glow (immediate, fast fade) plus a teal afterglow that lags behind it
// (delayed onset, slow fade) — see YuViGlowAudioProcessor::getPadTouchAmount()
// / getPadAfterglowAmount(). While the processor's global mapping editor is
// active (YuViGlowAudioProcessor::isEditingMappings()), this switches to a
// flat red/yellow/green assignment-state display instead — red unassigned,
// yellow the slot currently armed for learning, green already assigned —
// and squares become clickable to arm a specific slot
// (YuViGlowAudioProcessor::armLearnPadSlot()).
class PadGridComponent : public juce::Component,
                          private juce::Timer
{
public:
    explicit PadGridComponent (YuViGlowAudioProcessor& p) : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        constexpr int cols = 4;
        constexpr int rows = 4;
        constexpr float cellPadding = 4.0f;
        const auto purple = juce::Colour (0xff9b59ff);
        const auto teal = juce::Colour (0xff2dd4bf);
        const bool editing = processor.isEditingMappings();
        const int learningSlot = processor.getLearningPadSlot();

        const auto bounds = getLocalBounds().toFloat();
        const float cellW = bounds.getWidth() / (float) cols;
        const float cellH = bounds.getHeight() / (float) rows;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                // Pad 0 (learned first) is bottom-left, filling left-to-right
                // then upward — an assumed layout, not derived from hardware.
                const int padIndex = (rows - 1 - row) * cols + col;

                const juce::Rectangle<float> cell (bounds.getX() + (float) col * cellW,
                                                    bounds.getY() + (float) row * cellH,
                                                    cellW, cellH);
                const auto square = cell.reduced (cellPadding);

                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRoundedRectangle (square, 4.0f);

                if (editing)
                {
                    juce::Colour stateColour = juce::Colours::red.withAlpha (0.55f);
                    if (padIndex == learningSlot)
                        stateColour = juce::Colours::yellow.withAlpha (0.85f);
                    else if (processor.isPadAssigned (padIndex))
                        stateColour = juce::Colours::limegreen.withAlpha (0.7f);

                    g.setColour (stateColour);
                    g.fillRoundedRectangle (square.reduced (3.0f), 3.0f);
                }
                else
                {
                    const float touchAmount = processor.getPadTouchAmount (padIndex);
                    const float afterglowAmount = processor.getPadAfterglowAmount (padIndex);

                    // Teal afterglow drawn first (wider, softer, lags behind) so
                    // the purple flash reads as a sharp hit sitting on top of it.
                    if (afterglowAmount > 0.01f)
                    {
                        const auto glowBounds = square.expanded (square.getWidth() * 0.35f * afterglowAmount);
                        juce::ColourGradient glow (teal.withAlpha (juce::jlimit (0.0f, 0.6f, afterglowAmount)),
                                                    glowBounds.getCentre(),
                                                    teal.withAlpha (0.0f),
                                                    glowBounds.getCentre().translated (glowBounds.getWidth() * 0.5f, 0.0f),
                                                    true);
                        g.setGradientFill (glow);
                        g.fillRoundedRectangle (glowBounds, 8.0f);
                    }

                    if (touchAmount > 0.01f)
                    {
                        const auto glowBounds = square.expanded (square.getWidth() * 0.2f * touchAmount);
                        juce::ColourGradient glow (purple.withAlpha (juce::jlimit (0.0f, 0.85f, touchAmount)),
                                                    glowBounds.getCentre(),
                                                    purple.withAlpha (0.0f),
                                                    glowBounds.getCentre().translated (glowBounds.getWidth() * 0.5f, 0.0f),
                                                    true);
                        g.setGradientFill (glow);
                        g.fillRoundedRectangle (glowBounds, 6.0f);
                    }
                }

                g.setColour (juce::Colours::white.withAlpha (0.3f));
                g.drawRoundedRectangle (square, 4.0f, 1.0f);
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! processor.isEditingMappings())
            return;

        constexpr int cols = 4;
        constexpr int rows = 4;

        const auto bounds = getLocalBounds().toFloat();
        const float cellW = bounds.getWidth() / (float) cols;
        const float cellH = bounds.getHeight() / (float) rows;
        if (cellW <= 0.0f || cellH <= 0.0f)
            return;

        const int col = juce::jlimit (0, cols - 1, (int) (e.position.x / cellW));
        const int row = juce::jlimit (0, rows - 1, (int) (e.position.y / cellH));
        const int padIndex = (rows - 1 - row) * cols + col;

        processor.armLearnPadSlot (padIndex);
    }

private:
    void timerCallback() override { repaint(); }

    YuViGlowAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGridComponent)
};
