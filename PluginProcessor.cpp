#include "PluginProcessor.h"
#include "PluginEditor.h"

PrimaTakeFinishAudioProcessor::PrimaTakeFinishAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PrimaTakeFinishAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float> { 0.0f, 24.0f, 0.01f }, 4.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "body", "Body",
        juce::NormalisableRange<float> { -6.0f, 6.0f, 0.01f }, 0.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "presence", "Presence",
        juce::NormalisableRange<float> { -6.0f, 6.0f, 0.01f }, 0.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "control", "Control",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 25.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 100.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "output", "Output",
        juce::NormalisableRange<float> { -18.0f, 6.0f, 0.01f }, 0.0f, "dB"));

    return { params.begin(), params.end() };
}

bool PrimaTakeFinishAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void PrimaTakeFinishAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    const auto channels = static_cast<juce::uint32> (juce::jmax (1, getTotalNumOutputChannels()));
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), channels };

    bodyFilter.prepare (spec);
    presenceFilter.prepare (spec);
    compressor.prepare (spec);

    bodyFilter.reset();
    presenceFilter.reset();
    compressor.reset();

    compressor.setAttack (12.0f);
    compressor.setRelease (110.0f);

    dryBuffer.setSize (static_cast<int> (channels), samplesPerBlock);

    driveGain.reset (sampleRate, 0.02);
    wetMix.reset (sampleRate, 0.02);
    outputGain.reset (sampleRate, 0.02);

    driveGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (4.0f));
    wetMix.setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);

    updateFilters();
}

void PrimaTakeFinishAudioProcessor::updateFilters()
{
    const auto bodyDb = apvts.getRawParameterValue ("body")->load();
    const auto presenceDb = apvts.getRawParameterValue ("presence")->load();

    *bodyFilter.state = *Coefficients::makeLowShelf (
        currentSampleRate, 180.0, 0.70f, juce::Decibels::decibelsToGain (bodyDb));

    *presenceFilter.state = *Coefficients::makePeakFilter (
        currentSampleRate, 3200.0, 0.85f, juce::Decibels::decibelsToGain (presenceDb));
}

float PrimaTakeFinishAudioProcessor::saturate (float x) noexcept
{
    // Soft, symmetrical saturation. Normalisation keeps unity-ish gain at low levels.
    constexpr float norm = 1.0f / 0.76159415595f; // 1 / tanh(1)
    return std::tanh (x) * norm;
}

void PrimaTakeFinishAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();

    for (auto ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    dryBuffer.setSize (buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    dryBuffer.makeCopyOf (buffer, true);

    updateFilters();

    const auto driveDb = apvts.getRawParameterValue ("drive")->load();
    const auto control = apvts.getRawParameterValue ("control")->load() / 100.0f;
    const auto mix = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const auto outputDb = apvts.getRawParameterValue ("output")->load();

    driveGain.setTargetValue (juce::Decibels::decibelsToGain (driveDb));
    wetMix.setTargetValue (juce::jlimit (0.0f, 1.0f, mix));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputDb));

    // More Control = lower threshold + higher ratio, while remaining a gentle finishing compressor.
    compressor.setThreshold (juce::jmap (control, 0.0f, 1.0f, 0.0f, -28.0f));
    compressor.setRatio (juce::jmap (control, 0.0f, 1.0f, 1.0f, 4.0f));

    const auto numSamples = buffer.getNumSamples();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float g = driveGain.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            data[sample] = saturate (data[sample] * g);
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    bodyFilter.process (context);
    presenceFilter.process (context);
    compressor.process (context);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float wet = wetMix.getNextValue();
        const float dry = 1.0f - wet;
        const float out = outputGain.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float processed = buffer.getSample (ch, sample);
            const float original = dryBuffer.getSample (ch, sample);
            buffer.setSample (ch, sample, (processed * wet + original * dry) * out);
        }
    }
}

void PrimaTakeFinishAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PrimaTakeFinishAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessorEditor* PrimaTakeFinishAudioProcessor::createEditor()
{
    return new PrimaTakeFinishAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrimaTakeFinishAudioProcessor();
}
