#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class BF2StyleFlangerAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BF2StyleFlangerAudioProcessorEditor (BF2StyleFlangerAudioProcessor&);
    ~BF2StyleFlangerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Knob final : public juce::Component
    {
        Knob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId, const juce::String& labelText);

        void resized() override;

        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    BF2StyleFlangerAudioProcessor& audioProcessor;
    std::array<std::unique_ptr<Knob>, 6> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BF2StyleFlangerAudioProcessorEditor)
};
