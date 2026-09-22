#include "PluginEditor.h"

PrimaTakeFinishAudioProcessorEditor::PrimaTakeFinishAudioProcessorEditor (PrimaTakeFinishAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setupKnob (drive, "DRIVE", "drive");
    setupKnob (body, "BODY", "body");
    setupKnob (presence, "PRESENCE", "presence");
    setupKnob (control, "CONTROL", "control");
    setupKnob (mix, "MIX", "mix");
    setupKnob (output, "OUTPUT", "output");

    setResizable (true, true);
    setResizeLimits (620, 300, 1200, 600);
    setSize (820, 380);
}

void PrimaTakeFinishAudioProcessorEditor::setupKnob (Knob& knob,
                                                       const juce::String& name,
                                                       const juce::String& parameterID)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::whitesmoke);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff393939));
    knob.slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffd7493e));
    knob.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    knob.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff191919));
    knob.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, juce::Colours::white);
    knob.label.setFont (juce::Font (15.0f, juce::Font::bold));
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), parameterID, knob.slider);
}

void PrimaTakeFinishAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101010));

    auto area = getLocalBounds().toFloat();
    auto header = area.removeFromTop (84.0f);

    g.setColour (juce::Colour (0xffd7493e));
    g.fillRect (header.removeFromTop (5.0f));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.drawText ("PRIMA TAKE", 24, 18, getWidth() - 48, 34, juce::Justification::centredLeft);

    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::Font (15.0f));
    g.drawText ("FINISH  v0.1", 26, 51, getWidth() - 52, 22, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff292929));
    g.drawLine (24.0f, 82.0f, static_cast<float> (getWidth() - 24), 82.0f, 1.0f);
}

void PrimaTakeFinishAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (82);

    const int gap = 8;
    const int cellWidth = (area.getWidth() - gap * 5) / 6;

    for (auto* knob : knobs)
    {
        auto cell = area.removeFromLeft (cellWidth);
        knob->label.setBounds (cell.removeFromTop (28));
        knob->slider.setBounds (cell.reduced (4));
        area.removeFromLeft (gap);
    }
}
