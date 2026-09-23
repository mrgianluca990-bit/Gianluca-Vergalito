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
    juce::Colour accent      { 0xffe65045 };
    juce::Colour accentSoft  { 0xfff07a70 };
    juce::Colour ringOff     { 0xff30343a };
    juce::Colour knobTop     { 0xff34383f };
    juce::Colour knobBottom  { 0xff17191d };
};

class PrimaTakeFinishAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PrimaTakeFinishAudioProcessorEditor (PrimaTakeFinishAudioProcessor&);
    ~PrimaTakeFinishAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PrimaTakeFinishAudioProcessor& processor;
    FinishLookAndFeel lookAndFeel;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        juce::Label  hint;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob drive, body, presence, control, mix, output;
    std::array<Knob*, 6> knobs { &drive, &body, &presence, &control, &mix, &output };

    void setupKnob (Knob& knob,
                    const juce::String& name,
                    const juce::String& hintText,
                    const juce::String& parameterID);

    void drawHeader (juce::Graphics&, juce::Rectangle<float>);
    void drawControlCard (juce::Graphics&, juce::Rectangle<float>, bool highlighted);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrimaTakeFinishAudioProcessorEditor)
};
