#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ToggleCheckbox.h"
#include <array>

// 4x4 grid of the 16 pads. Normally shows live touch feedback: a purple
// glow that's sustained (not time-decayed) for as long as the pad is
// physically held down, then a teal afterglow that begins the instant the
// pad is released and fades over ~1800ms — see
// YuViGlowAudioProcessor::getPadTouchAmount() / getPadAfterglowAmount().
// Requires the processor to track note-off, not just note-on, per pad.
// While the processor's global mapping editor is
// active (YuViGlowAudioProcessor::isEditingMappings()), this switches to a
// flat red/yellow/green assignment-state display instead — red unassigned,
// yellow the slot currently armed for learning, green already assigned —
// and squares become clickable to arm a specific slot
// (YuViGlowAudioProcessor::armLearnPadSlot()).
//
// A narrow gutter down the left edge carries one checkbox per momentary row
// (the top two rows, pads 9-16 in the user-facing 1-based numbering). Checked
// — the default — means that row's four pads are hold-to-play; unchecked, they
// latch like the bottom two rows. See plan/issues/22.
class PadGridComponent : public juce::Component,
                          private juce::Timer
{
public:
    // Width of the checkbox gutter down the left edge, sized to fit one
    // checkbox. The 4x4 grid gets everything to the right of it.
    static constexpr int rowToggleGutterWidth = yuviglow::checkbox::width;

    explicit PadGridComponent (YuViGlowAudioProcessor& p) : processor (p)
    {
        for (int i = 0; i < YuViGlowAudioProcessor::numMomentaryRows; ++i)
        {
            auto& button = holdRowButtons[(size_t) i];
            button.setToggleState (processor.isMomentaryRowEnabled (i), juce::dontSendNotification);
            button.setTooltip ("Hold-to-play for this row — pads play only while held down");
            button.onClick = [this, i] { processor.setMomentaryRowEnabled (i, holdRowButtons[(size_t) i].getToggleState()); };
            addAndMakeVisible (button);
        }

        startTimerHz (30);
    }

    void resized() override
    {
        // Screen row 0 is the top row (pads 13-16 = momentary row offset 1);
        // screen row 1 sits below it (pads 9-12 = offset 0). Only these two
        // rows get a checkbox, so the gutter beside the bottom half is empty.
        const auto bounds = getLocalBounds();
        const int rowHeight = bounds.getHeight() / 4;

        for (int screenRow = 0; screenRow < YuViGlowAudioProcessor::numMomentaryRows; ++screenRow)
        {
            const int rowOffset = YuViGlowAudioProcessor::numMomentaryRows - 1 - screenRow;
            holdRowButtons[(size_t) rowOffset].setBounds (bounds.getX(),
                                                          bounds.getY() + screenRow * rowHeight + (rowHeight - yuviglow::checkbox::height) / 2,
                                                          yuviglow::checkbox::width, yuviglow::checkbox::height);
        }
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

        const auto bounds = getGridBounds();
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

        const auto bounds = getGridBounds();
        const float cellW = bounds.getWidth() / (float) cols;
        const float cellH = bounds.getHeight() / (float) rows;
        if (cellW <= 0.0f || cellH <= 0.0f)
            return;

        // Clicks in the checkbox gutter belong to the checkboxes, which are
        // real child components and get them first — anything reaching here
        // to the left of the grid is a miss, not pad 1.
        if (e.position.x < bounds.getX())
            return;

        const int col = juce::jlimit (0, cols - 1, (int) ((e.position.x - bounds.getX()) / cellW));
        const int row = juce::jlimit (0, rows - 1, (int) ((e.position.y - bounds.getY()) / cellH));
        const int padIndex = (rows - 1 - row) * cols + col;

        processor.armLearnPadSlot (padIndex);
    }

private:
    juce::Rectangle<float> getGridBounds() const
    {
        return getLocalBounds().toFloat().withTrimmedLeft ((float) rowToggleGutterWidth);
    }

    void timerCallback() override
    {
        for (int i = 0; i < YuViGlowAudioProcessor::numMomentaryRows; ++i)
            yuviglow::checkbox::mirror (holdRowButtons[(size_t) i], processor.isMomentaryRowEnabled (i));

        repaint();
    }

    YuViGlowAudioProcessor& processor;
    std::array<juce::ToggleButton, (size_t) YuViGlowAudioProcessor::numMomentaryRows> holdRowButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGridComponent)
};
