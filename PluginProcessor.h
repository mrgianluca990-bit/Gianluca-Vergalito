#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::ProcessorDuplicator<Filter, Coefficients> bodyFilter;
    juce::dsp::ProcessorDuplicator<Filter, Coefficients> presenceFilter;
    juce::dsp::Compressor<float> compressor;

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> driveGain;
    juce::SmoothedValue<float> wetMix;
    juce::SmoothedValue<float> outputGain;

    double currentSampleRate = 44100.0;

    void updateFilters();
    static float saturate (float x) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrimaTakeFinishAudioProcessor)
};
