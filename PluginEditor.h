#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PrimaTakeFinishAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PrimaTakeFinishAudioProcessorEditor (PrimaTakeFinishAudioProcessor&);
    ~PrimaTakeFinishAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PrimaTakeFinishAudioProcessor& processor;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob drive, body, presence, control, mix, output;
    std::array<Knob*, 6> knobs { &drive, &body, &presence, &control, &mix, &output };

    void setupKnob (Knob& knob, const juce::String& name, const juce::String& parameterID);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrimaTakeFinishAudioProcessorEditor)
};
