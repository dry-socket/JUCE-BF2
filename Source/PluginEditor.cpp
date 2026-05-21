#include "PluginEditor.h"

namespace
{
const auto background = juce::Colour (0xff2f3340);
const auto panel = juce::Colour (0xffd7d0bd);
const auto ink = juce::Colour (0xff20242d);
const auto accent = juce::Colour (0xff7a2f2f);

void styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff5a5e68));
    slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    slider.setColour (juce::Slider::textBoxTextColourId, ink);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
}

BF2StyleFlangerAudioProcessorEditor::Knob::Knob (
    juce::AudioProcessorValueTreeState& state,
    const juce::String& parameterId,
    const juce::String& labelText)
{
    styleKnob (slider);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, ink);
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (label);

    attachment = std::make_unique<SliderAttachment> (state, parameterId, slider);
}

void BF2StyleFlangerAudioProcessorEditor::Knob::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (22));
    slider.setBounds (area);
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
        std::make_unique<Knob> (state, "mix", "MIX"),
        std::make_unique<Knob> (state, "output", "OUTPUT")
    };

    for (auto& knob : knobs)
        addAndMakeVisible (*knob);

    setSize (620, 330);
}

void BF2StyleFlangerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    auto bounds = getLocalBounds().toFloat().reduced (18.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 8.0f);

    g.setColour (accent);
    g.fillRoundedRectangle (bounds.removeFromTop (64.0f), 8.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (27.0f, juce::Font::bold));
    g.drawText ("BF-2 STYLE FLANGER", 38, 26, getWidth() - 76, 34, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("VST3 modulation effect", 40, 58, getWidth() - 80, 20, juce::Justification::centredLeft);
}

void BF2StyleFlangerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (38);
    area.removeFromTop (82);

    auto grid = area.removeFromTop (178);
    const auto knobWidth = grid.getWidth() / static_cast<int> (knobs.size());

    for (auto& knob : knobs)
        knob->setBounds (grid.removeFromLeft (knobWidth).reduced (6, 0));
}
