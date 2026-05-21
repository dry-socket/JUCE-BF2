#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto parameterTreeId = "PARAMETERS";
constexpr float minDelayMs = 0.25f;
constexpr float maxDelayMs = 12.0f;

float getParameterValue (juce::AudioProcessorValueTreeState& parameters, const juce::String& id)
{
    return parameters.getRawParameterValue (id)->load();
}
}

BF2StyleFlangerAudioProcessor::BF2StyleFlangerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, parameterTreeId, createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BF2StyleFlangerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "manual", "Manual", juce::NormalisableRange<float> (0.25f, 8.0f, 0.01f, 0.55f), 2.2f, "ms"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "depth", "Depth", juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f, 0.75f), 4.8f, "ms"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "rate", "Rate", juce::NormalisableRange<float> (0.03f, 12.0f, 0.001f, 0.35f), 0.35f, "Hz"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "resonance", "Resonance", juce::NormalisableRange<float> (-0.85f, 0.85f, 0.001f), 0.48f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.58f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "output", "Output", juce::NormalisableRange<float> (-18.0f, 12.0f, 0.1f), 0.0f, "dB"));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "enabled", "Pedal", true));

    return { params.begin(), params.end() };
}

void BF2StyleFlangerAudioProcessor::DelayLine::prepare (double newSampleRate, int numChannels)
{
    sampleRate = newSampleRate;
    bufferSize = juce::roundToInt (sampleRate * 0.05);
    buffer.setSize (juce::jmax (1, numChannels), bufferSize);
    reset();
}

void BF2StyleFlangerAudioProcessor::DelayLine::reset()
{
    buffer.clear();
    writePosition = 0;
}

float BF2StyleFlangerAudioProcessor::DelayLine::readInterpolated (int channel, float delaySamples) const
{
    const auto safeDelay = juce::jlimit (1.0f, static_cast<float> (bufferSize - 2), delaySamples);
    auto readPosition = static_cast<float> (writePosition) - safeDelay;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (bufferSize);

    const auto index0 = static_cast<int> (readPosition);
    const auto index1 = (index0 + 1) % bufferSize;
    const auto fraction = readPosition - static_cast<float> (index0);

    const auto* data = buffer.getReadPointer (channel);
    return data[index0] + fraction * (data[index1] - data[index0]);
}

void BF2StyleFlangerAudioProcessor::DelayLine::push (int channel, float sample)
{
    buffer.setSample (channel, writePosition, sample);
}

void BF2StyleFlangerAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    delay.prepare (sampleRate, getTotalNumOutputChannels());
    feedbackState.fill (0.0f);
    lfoPhase = 0.0f;

    for (auto* smoother : { &manualMs, &depthMs, &feedback, &mix, &outputGain })
        smoother->reset (sampleRate, 0.025);

    manualMs.setCurrentAndTargetValue (getParameterValue (parameters, "manual"));
    depthMs.setCurrentAndTargetValue (getParameterValue (parameters, "depth"));
    feedback.setCurrentAndTargetValue (getParameterValue (parameters, "resonance"));
    mix.setCurrentAndTargetValue (getParameterValue (parameters, "mix"));
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getParameterValue (parameters, "output")));
}

bool BF2StyleFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainOutput == layouts.getMainInputChannelSet()
        && (mainOutput == juce::AudioChannelSet::mono() || mainOutput == juce::AudioChannelSet::stereo());
}

void BF2StyleFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (getParameterValue (parameters, "enabled") < 0.5f)
    {
        delay.reset();
        feedbackState.fill (0.0f);
        return;
    }

    manualMs.setTargetValue (getParameterValue (parameters, "manual"));
    depthMs.setTargetValue (getParameterValue (parameters, "depth"));
    feedback.setTargetValue (getParameterValue (parameters, "resonance"));
    mix.setTargetValue (getParameterValue (parameters, "mix"));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (getParameterValue (parameters, "output")));

    const auto rateHz = getParameterValue (parameters, "rate");
    const auto phaseStep = rateHz / static_cast<float> (currentSampleRate);
    const auto channels = juce::jmin (buffer.getNumChannels(), delay.buffer.getNumChannels());

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = lfoPhase;
        const auto leftLfo = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * phase);
        const auto rightPhase = std::fmod (phase + 0.25f, 1.0f);
        const auto rightLfo = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * rightPhase);
        const auto baseMs = manualMs.getNextValue();
        const auto sweepMs = depthMs.getNextValue();
        const auto fb = feedback.getNextValue();
        const auto wetMix = mix.getNextValue();
        const auto gain = outputGain.getNextValue();

        for (auto channel = 0; channel < channels; ++channel)
        {
            const auto* inputData = buffer.getReadPointer (channel);
            auto* outputData = buffer.getWritePointer (channel);
            const auto lfo = channel == 0 ? leftLfo : rightLfo;
            const auto delayMs = juce::jlimit (minDelayMs, maxDelayMs, baseMs + sweepMs * lfo);
            const auto delaySamples = delayMs * 0.001f * static_cast<float> (currentSampleRate);
            const auto dry = inputData[sample];
            const auto delayed = delay.readInterpolated (channel, delaySamples);
            const auto driveIn = std::tanh (dry + delayed * fb);

            delay.push (channel, driveIn);
            feedbackState[static_cast<size_t> (channel)] = delayed;
            outputData[sample] = gain * (dry + wetMix * (delayed - dry));
        }

        delay.writePosition = (delay.writePosition + 1) % delay.bufferSize;
        lfoPhase += phaseStep;

        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }
}

void BF2StyleFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    parameters.state.writeToStream (stream);
}

void BF2StyleFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto tree = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes)); tree.isValid())
        parameters.replaceState (tree);
}

juce::AudioProcessorEditor* BF2StyleFlangerAudioProcessor::createEditor()
{
    return new BF2StyleFlangerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BF2StyleFlangerAudioProcessor();
}
