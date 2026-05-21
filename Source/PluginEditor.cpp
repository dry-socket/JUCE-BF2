#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const auto desktop = juce::Colour (0xff17171a);
const auto ledOff = juce::Colour (0xff2a0808);
const auto ledOn = juce::Colour (0xffff3038);

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

juce::Rectangle<float> getPhotoBounds (juce::Rectangle<int> editorBounds, const juce::Image& image)
{
    auto area = editorBounds.toFloat().reduced (24.0f, 16.0f);

    if (! image.isValid())
        return area;

    const auto imageAspect = static_cast<float> (image.getWidth()) / static_cast<float> (image.getHeight());
    const auto areaAspect = area.getWidth() / area.getHeight();

    if (areaAspect > imageAspect)
        return area.withWidth (area.getHeight() * imageAspect).withCentre (area.getCentre());

    return area.withHeight (area.getWidth() / imageAspect).withCentre (area.getCentre());
}

void styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel (&bf2LookAndFeel);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.18f,
                                juce::MathConstants<float>::pi * 2.82f,
                                true);
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
    if (small)
        slider.setAlpha (0.01f);

    slider.setTooltip (labelText);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
    label.setFont (juce::FontOptions (small ? 11.0f : 10.0f, juce::Font::bold));
    label.setVisible (false);
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
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      pedalPhoto (juce::ImageCache::getFromMemory (BinaryData::bf_2_jpg, BinaryData::bf_2_jpgSize))
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

    footSwitch.setButtonText ({});
    footSwitch.setClickingTogglesState (true);
    footSwitch.setAlpha (0.01f);
    addAndMakeVisible (footSwitch);
    footSwitchAttachment = std::make_unique<ButtonAttachment> (state, "enabled", footSwitch);
    footSwitch.onClick = [this] { repaint(); };

    startTimerHz (24);
    setSize (390, 660);
}

void BF2StyleFlangerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (desktop);

    const auto photoBounds = getPhotoBounds (getLocalBounds(), pedalPhoto);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (photoBounds.translated (4.0f, 8.0f), 8.0f);

    if (pedalPhoto.isValid())
        g.drawImage (pedalPhoto, photoBounds, juce::RectanglePlacement::stretchToFit);

    const auto enabled = audioProcessor.getValueTreeState().getRawParameterValue ("enabled")->load() > 0.5f;
    const auto ledCentre = juce::Point<float> (photoBounds.getX() + photoBounds.getWidth() * 0.50f,
                                              photoBounds.getY() + photoBounds.getHeight() * 0.065f);
    const auto led = juce::Rectangle<float> (12.0f, 12.0f).withCentre (ledCentre);

    if (enabled)
    {
        juce::ColourGradient glow (ledOn.withAlpha (0.65f), ledCentre.x, ledCentre.y,
                                   ledOn.withAlpha (0.0f), ledCentre.x + 26.0f, ledCentre.y + 26.0f, true);
        g.setGradientFill (glow);
        g.fillEllipse (led.expanded (18.0f));
        g.setColour (ledOn);
    }
    else
    {
        g.setColour (ledOff);
    }

    g.fillEllipse (led);
    g.setColour (juce::Colours::white.withAlpha (enabled ? 0.42f : 0.12f));
    g.fillEllipse (led.reduced (3.0f).withTrimmedBottom (5.0f));
}

void BF2StyleFlangerAudioProcessorEditor::resized()
{
    const auto photo = getPhotoBounds (getLocalBounds(), pedalPhoto);
    const auto x = photo.getX();
    const auto y = photo.getY();
    const auto w = photo.getWidth();
    const auto h = photo.getHeight();

    const std::array<float, 4> knobCentres { 0.156f, 0.356f, 0.558f, 0.759f };
    const auto knobSize = juce::roundToInt (w * 0.19f);
    const auto knobY = juce::roundToInt (y + h * 0.22f - static_cast<float> (knobSize) * 0.5f);

    for (auto i = 0; i < 4; ++i)
    {
        const auto knobX = juce::roundToInt (x + w * knobCentres[static_cast<size_t> (i)]
                                             - static_cast<float> (knobSize) * 0.5f);
        knobs[static_cast<size_t> (i)]->setBounds (knobX, knobY, knobSize, knobSize);
    }

    const auto trimSize = juce::roundToInt (w * 0.16f);
    knobs[4]->setBounds (juce::roundToInt (x + w * 0.19f), juce::roundToInt (y + h * 0.57f), trimSize, trimSize);
    knobs[5]->setBounds (juce::roundToInt (x + w * 0.65f), juce::roundToInt (y + h * 0.57f), trimSize, trimSize);

    footSwitch.setBounds (juce::roundToInt (x + w * 0.15f),
                          juce::roundToInt (y + h * 0.63f),
                          juce::roundToInt (w * 0.70f),
                          juce::roundToInt (h * 0.25f));
}

void BF2StyleFlangerAudioProcessorEditor::timerCallback()
{
    repaint();
}
