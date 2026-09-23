#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

PrimaTakeFinishAudioProcessor::PrimaTakeFinishAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
PrimaTakeFinishAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 28.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "body", "Body",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 8.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "detail", "Detail",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 10.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "glue", "Glue",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 28.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "punch", "Punch",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "space", "Space",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 12.0f, "%"));

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
    const auto mainIn  = layouts.getMainInputChannelSet();
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
    const juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (samplesPerBlock),
        channels
    };

    bodyFilter.prepare (spec);
    detailFilter.prepare (spec);
    glueCompressor.prepare (spec);

    bodyFilter.reset();
    detailFilter.reset();
    glueCompressor.reset();

    dryBuffer.setSize (static_cast<int> (channels), samplesPerBlock);

    driveAmount.reset (sampleRate, 0.025);
    wetMix.reset      (sampleRate, 0.025);
    outputGain.reset  (sampleRate, 0.025);
    stereoWidth.reset (sampleRate, 0.04);

    driveAmount.setCurrentAndTargetValue (0.28f);
    wetMix.setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);
    stereoWidth.setCurrentAndTargetValue (1.0f);

    fastEnv.fill (0.0f);
    slowEnv.fill (0.0f);

    updateEnvelopeCoefficients();
    updateToneFilters();
}

void PrimaTakeFinishAudioProcessor::updateEnvelopeCoefficients()
{
    auto coeff = [this] (double ms)
    {
        return static_cast<float> (std::exp (-1.0 / (0.001 * ms * currentSampleRate)));
    };

    fastAttackCoeff  = coeff (2.5);
    fastReleaseCoeff = coeff (55.0);
    slowCoeff        = coeff (180.0);
}

void PrimaTakeFinishAudioProcessor::updateToneFilters()
{
    const auto bodyParam   = apvts.getRawParameterValue ("body")->load() / 100.0f;
    const auto detailParam = apvts.getRawParameterValue ("detail")->load() / 100.0f;

    const float bodyDb   = 5.5f * bodyParam;
    const float detailDb = 5.0f * detailParam;

    *bodyFilter.state = *Coefficients::makeLowShelf (
        currentSampleRate,
        165.0,
        0.72f,
        juce::Decibels::decibelsToGain (bodyDb));

    *detailFilter.state = *Coefficients::makeHighShelf (
        currentSampleRate,
        4300.0,
        0.72f,
        juce::Decibels::decibelsToGain (detailDb));
}

float PrimaTakeFinishAudioProcessor::processSaturation (float x,
                                                         float amount,
                                                         float envelope) const noexcept
{
    // "Adaptive Density": more harmonic density at lower levels, less extra
    // push on already-loud material. This keeps the finish effect musical.
    const float adaptive = juce::jlimit (0.35f, 1.0f, 1.0f - envelope * 0.40f);
    const float a = amount * adaptive;

    const float preGain = 1.0f + a * 5.5f;
    const float shaped  = std::tanh (x * preGain);

    // Progressive blend prevents DRIVE from becoming a simple fuzz control.
    const float blend = juce::jlimit (0.0f, 0.88f, a * 0.88f);
    const float compensation = 1.0f / (1.0f + a * 0.75f);

    return (x + (shaped - x) * blend) * compensation;
}

float PrimaTakeFinishAudioProcessor::processTransient (float x,
                                                        int channel,
                                                        float punchAmount) noexcept
{
    const int ch = juce::jlimit (0, 1, channel);
    const float rectified = std::abs (x);

    const float fastCoeff = rectified > fastEnv[static_cast<size_t> (ch)]
                                ? fastAttackCoeff
                                : fastReleaseCoeff;

    fastEnv[static_cast<size_t> (ch)] =
        fastCoeff * fastEnv[static_cast<size_t> (ch)]
        + (1.0f - fastCoeff) * rectified;

    slowEnv[static_cast<size_t> (ch)] =
        slowCoeff * slowEnv[static_cast<size_t> (ch)]
        + (1.0f - slowCoeff) * rectified;

    const float transient = fastEnv[static_cast<size_t> (ch)]
                          - slowEnv[static_cast<size_t> (ch)];

    // ±100 maps to roughly ±4 dB on strong transients.
    const float gainDb = juce::jlimit (-4.0f, 4.0f,
                                      transient * punchAmount * 24.0f);

    return x * juce::Decibels::decibelsToGain (gainDb);
}

void PrimaTakeFinishAudioProcessor::processStereoSpace (juce::AudioBuffer<float>& buffer,
                                                         float width)
{
    if (buffer.getNumChannels() < 2)
        return;

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = left[i];
        const float r = right[i];

        const float mid  = 0.5f * (l + r);
        const float side = 0.5f * (l - r) * width;

        // Small energy compensation as width increases.
        const float compensation = 1.0f / std::sqrt (juce::jmax (1.0f, width));

        left[i]  = (mid + side) * compensation;
        right[i] = (mid - side) * compensation;
    }
}

void PrimaTakeFinishAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    dryBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    dryBuffer.makeCopyOf (buffer, true);

    float inPeak = 0.0f;
    float blockEnergy = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        inPeak = juce::jmax (inPeak, buffer.getMagnitude (ch, 0, numSamples));
        blockEnergy += buffer.getRMSLevel (ch, 0, numSamples);
    }

    blockEnergy /= static_cast<float> (juce::jmax (1, buffer.getNumChannels()));
    inputMeter.store (juce::jlimit (0.0f, 1.0f, inPeak));
    energyMeter.store (juce::jlimit (0.0f, 1.0f, blockEnergy * 2.2f));

    updateToneFilters();

    const float drive  = apvts.getRawParameterValue ("drive")->load() / 100.0f;
    const float glue   = apvts.getRawParameterValue ("glue")->load() / 100.0f;
    const float punch  = apvts.getRawParameterValue ("punch")->load() / 100.0f;
    const float space  = apvts.getRawParameterValue ("space")->load() / 100.0f;
    const float mix    = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const float output = apvts.getRawParameterValue ("output")->load();

    driveAmount.setTargetValue (drive);
    wetMix.setTargetValue (juce::jlimit (0.0f, 1.0f, mix));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (output));
    stereoWidth.setTargetValue (1.0f + space * 0.62f);

    // Program-dependent glue: stronger settings react faster, while release
    // follows the material's average energy.
    glueCompressor.setThreshold (juce::jmap (glue, 0.0f, 1.0f, -2.0f, -26.0f));
    glueCompressor.setRatio     (juce::jmap (glue, 0.0f, 1.0f, 1.0f, 4.5f));
    glueCompressor.setAttack    (juce::jmap (glue, 0.0f, 1.0f, 24.0f, 6.0f));

    const float energyNorm = juce::jlimit (0.0f, 1.0f, blockEnergy * 2.0f);
    glueCompressor.setRelease (75.0f + (1.0f - energyNorm) * 145.0f);

    // Adaptive density + transient stage.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float d = driveAmount.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const int envCh = juce::jmin (ch, 1);
            const float env = slowEnv[static_cast<size_t> (envCh)];

            float x = processSaturation (data[sample], d, env);
            x = processTransient (x, envCh, punch);
            data[sample] = x;
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    bodyFilter.process (context);
    detailFilter.process (context);
    glueCompressor.process (context);

    // Width is intentionally after glue so dynamics remain coherent.
    const float width = stereoWidth.getNextValue();
    processStereoSpace (buffer, width);

    // Parallel blend + output trim.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float wet = wetMix.getNextValue();
        const float dry = 1.0f - wet;
        const float out = outputGain.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float processed = buffer.getSample (ch, sample);
            const float original  = dryBuffer.getSample (ch, sample);

            buffer.setSample (ch, sample,
                              (processed * wet + original * dry) * out);
        }
    }

    float outPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        outPeak = juce::jmax (outPeak, buffer.getMagnitude (ch, 0, numSamples));

    outputMeter.store (juce::jlimit (0.0f, 1.0f, outPeak));
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
