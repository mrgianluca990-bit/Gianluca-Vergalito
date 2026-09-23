#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class FinishLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    FinishLookAndFeel();

    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

private:
    juce::Colour accentHot  { 0xffff5d4d };
    juce::Colour accentWarm { 0xffff9f4d };
    juce::Colour accentCool { 0xff52d3ff };
    juce::Colour ringOff    { 0xff2a2f36 };
};

class PrimaTakeFinishAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit PrimaTakeFinishAudioProcessorEditor (PrimaTakeFinishAudioProcessor&);
    ~PrimaTakeFinishAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PrimaTakeFinishAudioProcessor& processor;
    FinishLookAndFeel finishLook;

    struct Knob
    {
        juce::Slider slider;
        juce::Label name;
        juce::Label hint;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob drive, body, detail, glue, punch, space, mix, output;
    std::array<Knob*, 8> knobs {
        &drive, &body, &detail, &glue,
        &punch, &space, &mix, &output
    };

    void setupKnob (Knob& knob,
                    const juce::String& name,
                    const juce::String& hint,
                    const juce::String& parameterID);

    void drawTopBar (juce::Graphics&, juce::Rectangle<float>);
    void drawEngine (juce::Graphics&, juce::Rectangle<float>);
    void drawMeter (juce::Graphics&, juce::Rectangle<float>,
                    float value, juce::Colour colour,
                    const juce::String& label);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrimaTakeFinishAudioProcessorEditor)
};
