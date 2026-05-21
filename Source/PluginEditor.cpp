#include "PluginEditor.h"

namespace
{
const auto desktop = juce::Colour (0xff17171a);
const auto pedalDark = juce::Colour (0xff46103b);
const auto pedalMid = juce::Colour (0xffa73b87);
const auto pedalLight = juce::Colour (0xffdf82be);
const auto topPanel = juce::Colour (0xff080a0c);
const auto cream = juce::Colour (0xffe8e2d3);
const auto darkInk = juce::Colour (0xff1a1720);
const auto rubber = juce::Colour (0xff141719);

class BF2LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           const float rotaryStartAngle,
                           const float rotaryEndAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                   static_cast<float> (width), static_cast<float> (height))
                              .reduced (4.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto knobBounds = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (knobBounds.translated (2.5f, 5.0f));

        juce::ColourGradient side (juce::Colour (0xfff5f2ea), knobBounds.getX(), knobBounds.getY(),
                                   juce::Colour (0xff6f7479), knobBounds.getRight(), knobBounds.getBottom(), false);
        side.addColour (0.42, juce::Colour (0xffd7d9d7));
        side.addColour (0.7, juce::Colour (0xff272b2f));
        g.setGradientFill (side);
        g.fillEllipse (knobBounds);

        g.setColour (juce::Colour (0xff0b0d10));
        g.drawEllipse (knobBounds, 1.4f);

        const auto cap = knobBounds.reduced (radius * 0.13f);
        juce::ColourGradient capGradient (juce::Colour (0xfffaf8ef), cap.getX(), cap.getY(),
                                          juce::Colour (0xffa1a5a5), cap.getRight(), cap.getBottom(), false);
        capGradient.addColour (0.55, juce::Colour (0xffdcdedb));
        g.setGradientFill (capGradient);
        g.fillEllipse (cap);

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.5f, -radius * 0.82f, 3.0f, radius * 0.34f, 1.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (juce::Colour (0xfff7f1e6));
        g.fillPath (pointer);
    }
};

BF2LookAndFeel bf2LookAndFeel;

void styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel (&bf2LookAndFeel);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.18f,
                                juce::MathConstants<float>::pi * 2.82f,
                                true);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre)
{
    auto bounds = juce::Rectangle<float> (12.0f, 12.0f).withCentre (centre);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (bounds.translated (1.0f, 1.5f));
    g.setColour (juce::Colour (0xffb8b4aa));
    g.fillEllipse (bounds);
    g.setColour (juce::Colour (0xff4a4643));
    g.drawLine (bounds.getX() + 3.0f, centre.y, bounds.getRight() - 3.0f, centre.y, 1.4f);
}
}

BF2StyleFlangerAudioProcessorEditor::Knob::Knob (
    juce::AudioProcessorValueTreeState& state,
    const juce::String& parameterId,
    const juce::String& labelText,
    bool isSmallKnob)
    : small (isSmallKnob)
{
    styleKnob (slider);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, small ? darkInk : juce::Colours::white);
    label.setFont (juce::FontOptions (small ? 11.0f : 10.0f, juce::Font::bold));
    addAndMakeVisible (label);

    attachment = std::make_unique<SliderAttachment> (state, parameterId, slider);
}

void BF2StyleFlangerAudioProcessorEditor::Knob::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (small ? 18 : 16));
    slider.setBounds (area.reduced (small ? 8 : 0));
}

BF2StyleFlangerAudioProcessorEditor::BF2StyleFlangerAudioProcessorEditor (BF2StyleFlangerAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    auto& state = audioProcessor.getValueTreeState();
    knobs = {
        std::make_unique<Knob> (state, "manual", "MANUAL"),
        std::make_unique<Knob> (state, "depth", "DEPTH"),
        std::make_unique<Knob> (state, "rate", "RATE"),
        std::make_unique<Knob> (state, "resonance", "RESONANCE"),
        std::make_unique<Knob> (state, "mix", "MIX", true),
        std::make_unique<Knob> (state, "output", "OUTPUT", true)
    };

    for (auto& knob : knobs)
        addAndMakeVisible (*knob);

    setSize (390, 660);
}

void BF2StyleFlangerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (desktop);

    auto pedal = getLocalBounds().toFloat().reduced (36.0f, 20.0f);
    pedal.removeFromBottom (12.0f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (pedal.translated (4.0f, 8.0f), 17.0f);

    juce::ColourGradient bodyGradient (pedalLight, pedal.getX(), pedal.getY(),
                                       pedalDark, pedal.getX(), pedal.getBottom(), false);
    bodyGradient.addColour (0.18, pedalMid);
    bodyGradient.addColour (0.72, juce::Colour (0xff84256f));
    g.setGradientFill (bodyGradient);
    g.fillRoundedRectangle (pedal, 16.0f);

    g.setColour (juce::Colour (0xff5f154f));
    g.drawRoundedRectangle (pedal.reduced (1.5f), 15.0f, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawLine (pedal.getX() + 12.0f, pedal.getY() + 6.0f, pedal.getRight() - 12.0f, pedal.getY() + 6.0f, 2.0f);

    auto top = pedal.reduced (18.0f, 16.0f).removeFromTop (150.0f);
    g.setColour (topPanel);
    g.fillRoundedRectangle (top, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawRoundedRectangle (top.reduced (1.0f), 5.0f, 1.0f);

    g.setColour (cream);
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("CHECK", top.withHeight (18.0f).translated (0.0f, -2.0f), juce::Justification::centred);

    auto led = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ top.getCentreX(), top.getY() + 26.0f });
    g.setColour (juce::Colour (0xff331515));
    g.fillEllipse (led.expanded (2.0f));
    g.setColour (juce::Colour (0xffb7282d));
    g.fillEllipse (led);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.fillEllipse (led.reduced (2.0f).withTrimmedBottom (4.0f));

    auto jackBand = juce::Rectangle<float> (pedal.getX() + 16.0f, top.getBottom() + 10.0f,
                                           pedal.getWidth() - 32.0f, 42.0f);
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.fillRoundedRectangle (jackBand, 3.0f);
    g.setColour (darkInk);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("<- OUTPUT", jackBand.removeFromLeft (125.0f), juce::Justification::centredLeft);
    g.drawText ("INPUT ->", jackBand, juce::Justification::centredRight);

    auto title = juce::Rectangle<float> (pedal.getX() + 42.0f, pedal.getY() + 290.0f,
                                        pedal.getWidth() - 84.0f, 70.0f);
    g.setColour (darkInk);
    g.setFont (juce::FontOptions (44.0f, juce::Font::plain));
    g.drawText ("Flanger", title.removeFromTop (44.0f), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("BF-2", title, juce::Justification::centredRight);

    auto foot = juce::Rectangle<float> (pedal.getX() + 36.0f, pedal.getY() + 410.0f,
                                       pedal.getWidth() - 72.0f, 165.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (foot.translated (0.0f, 4.0f), 5.0f);
    g.setColour (rubber);
    g.fillRoundedRectangle (foot, 4.0f);

    juce::ColourGradient rubberGradient (juce::Colour (0xff303538), foot.getX(), foot.getY(),
                                         juce::Colour (0xff050607), foot.getX(), foot.getBottom(), false);
    g.setGradientFill (rubberGradient);
    g.fillRoundedRectangle (foot.reduced (8.0f), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawRoundedRectangle (foot.reduced (8.0f), 3.0f, 2.0f);

    g.setFont (juce::FontOptions (35.0f, juce::Font::bold));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("BOSS", foot.reduced (18.0f).removeFromTop (54.0f), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawText ("BOSS", foot.reduced (18.0f).removeFromTop (54.0f).translated (-1.0f, -1.0f),
                juce::Justification::centred);

    auto bottomLatch = juce::Rectangle<float> (54.0f, 32.0f).withCentre ({ pedal.getCentreX(), pedal.getBottom() + 6.0f });
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillRoundedRectangle (bottomLatch, 5.0f);

    auto sideJackLeft = juce::Rectangle<float> (18.0f, 60.0f).withCentre ({ pedal.getX() - 4.0f, pedal.getY() + 215.0f });
    auto sideJackRight = sideJackLeft.withCentre ({ pedal.getRight() + 4.0f, pedal.getY() + 215.0f });
    g.setColour (juce::Colour (0xff59595a));
    g.fillRoundedRectangle (sideJackLeft, 5.0f);
    g.fillRoundedRectangle (sideJackRight, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (sideJackLeft, 5.0f, 1.0f);
    g.drawRoundedRectangle (sideJackRight, 5.0f, 1.0f);

    drawScrew (g, { pedal.getX() + 18.0f, top.getY() + 102.0f });
    drawScrew (g, { pedal.getRight() - 18.0f, top.getY() + 102.0f });
}

void BF2StyleFlangerAudioProcessorEditor::resized()
{
    auto pedal = getLocalBounds().reduced (36, 20);
    pedal.removeFromBottom (12);

    auto top = pedal.reduced (18, 16).removeFromTop (150);
    auto knobRow = top.reduced (6, 20).withTrimmedTop (10);
    const auto largeKnobWidth = knobRow.getWidth() / 4;

    for (auto i = 0; i < 4; ++i)
        knobs[static_cast<size_t> (i)]->setBounds (knobRow.removeFromLeft (largeKnobWidth).reduced (3, 0));

    auto trimArea = juce::Rectangle<int> (pedal.getX() + 52, pedal.getY() + 362, pedal.getWidth() - 104, 52);
    knobs[4]->setBounds (trimArea.removeFromLeft (82));
    knobs[5]->setBounds (trimArea.removeFromRight (82));
}
