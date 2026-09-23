#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class PrimaTakeFinishAudioProcessor final : public juce::AudioProcessor
{
public:
    PrimaTakeFinishAudioProcessor();
    ~PrimaTakeFinishAudioProcessor() override = default;

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

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float getInputMeter()  const noexcept { return inputMeter.load(); }
    float getOutputMeter() const noexcept { return outputMeter.load(); }
    float getEnergyMeter() const noexcept { return energyMeter.load(); }

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::ProcessorDuplicator<Filter, Coefficients> bodyFilter;
    juce::dsp::ProcessorDuplicator<Filter, Coefficients> detailFilter;
    juce::dsp::Compressor<float> glueCompressor;

    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float> driveAmount;
    juce::SmoothedValue<float> wetMix;
    juce::SmoothedValue<float> outputGain;
    juce::SmoothedValue<float> stereoWidth;

    std::array<float, 2> fastEnv { 0.0f, 0.0f };
    std::array<float, 2> slowEnv { 0.0f, 0.0f };

    double currentSampleRate = 44100.0;
    float fastAttackCoeff = 0.0f;
    float fastReleaseCoeff = 0.0f;
    float slowCoeff = 0.0f;

    std::atomic<float> inputMeter  { 0.0f };
    std::atomic<float> outputMeter { 0.0f };
    std::atomic<float> energyMeter { 0.0f };

    void updateToneFilters();
    void updateEnvelopeCoefficients();
    float processSaturation (float x, float amount, float envelope) const noexcept;
    float processTransient (float x, int channel, float punchAmount) noexcept;
    void processStereoSpace (juce::AudioBuffer<float>& buffer, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrimaTakeFinishAudioProcessor)
};
