#pragma once

#include <JuceHeader.h>

class BF2StyleFlangerAudioProcessor final : public juce::AudioProcessor
{
public:
    BF2StyleFlangerAudioProcessor();
    ~BF2StyleFlangerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct DelayLine
    {
        void prepare (double newSampleRate, int numChannels);
        void reset();
        float readInterpolated (int channel, float delaySamples) const;
        void push (int channel, float sample);

        juce::AudioBuffer<float> buffer;
        int writePosition = 0;
        int bufferSize = 1;
        double sampleRate = 44100.0;
    };

    DelayLine delay;
    juce::AudioProcessorValueTreeState parameters;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> manualMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedback;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;

    std::array<float, 2> feedbackState { 0.0f, 0.0f };
    double currentSampleRate = 44100.0;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BF2StyleFlangerAudioProcessor)
};
