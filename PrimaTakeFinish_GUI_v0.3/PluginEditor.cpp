#include "PluginEditor.h"

//==============================================================================
// Look & Feel
FinishLookAndFeel::FinishLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,        juce::Colour (0xfff2f2f3));
    setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colour (0xff16181c));
    setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,                juce::Colour (0xfff2f2f3));
}

void FinishLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                 static_cast<float> (y),
                                                 static_cast<float> (width),
                                                 static_cast<float> (height))
                            .reduced (11.0f);

    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto dial = juce::Rectangle<float> (0.0f, 0.0f, size, size)
                        .withCentre (bounds.getCentre());

    const auto centre = dial.getCentre();
    const float radius = dial.getWidth() * 0.5f;
    const float ringRadius = radius - 4.0f;
    const float lineWidth = juce::jmax (3.0f, size * 0.045f);
    const float angle = rotaryStartAngle
                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Outer shadow
    g.setColour (juce::Colour (0x66000000));
    g.fillEllipse (dial.translated (0.0f, 5.0f).expanded (2.0f));

    // Inactive arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y,
                                 ringRadius, ringRadius,
                                 0.0f,
                                 rotaryStartAngle,
                                 rotaryEndAngle,
                                 true);

    g.setColour (ringOff);
    g.strokePath (backgroundArc,
                  juce::PathStrokeType (lineWidth,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // Active arc
    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y,
                            ringRadius, ringRadius,
                            0.0f,
                            rotaryStartAngle,
                            angle,
                            true);

    juce::ColourGradient arcGradient (accentSoft,
                                      centre.x - radius, centre.y - radius,
                                      accent,
                                      centre.x + radius, centre.y + radius,
                                      false);
    g.setGradientFill (arcGradient);
    g.strokePath (valueArc,
                  juce::PathStrokeType (lineWidth,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // Knob body
    auto knob = dial.reduced (lineWidth + 7.0f);

    juce::ColourGradient knobGradient (knobTop,
                                       knob.getCentreX(), knob.getY(),
                                       knobBottom,
                                       knob.getCentreX(), knob.getBottom(),
                                       false);
    g.setGradientFill (knobGradient);
    g.fillEllipse (knob);

    g.setColour (juce::Colour (0xff4c5159));
    g.drawEllipse (knob, 1.0f);

    // Inner highlight
    g.setColour (juce::Colour (0x22ffffff));
    g.drawEllipse (knob.reduced (3.0f), 1.0f);

    // Pointer
    juce::Path pointer;
    const float pointerWidth  = juce::jmax (2.0f, size * 0.025f);
    const float pointerLength = knob.getHeight() * 0.31f;

    pointer.addRoundedRectangle (-pointerWidth * 0.5f,
                                 -knob.getHeight() * 0.42f,
                                 pointerWidth,
                                 pointerLength,
                                 pointerWidth * 0.5f);

    g.setColour (juce::Colour (0xfff6f6f7));
    g.fillPath (pointer,
                juce::AffineTransform::rotation (angle)
                    .translated (centre.x, centre.y));

    // Centre cap
    g.setColour (juce::Colour (0xff111317));
    g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (centre));
    g.setColour (accent);
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (centre));
}

void FinishLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawFittedText (label.getText(),
                      label.getLocalBounds(),
                      label.getJustificationType(),
                      1);
}

//==============================================================================
// Editor
PrimaTakeFinishAudioProcessorEditor::PrimaTakeFinishAudioProcessorEditor (PrimaTakeFinishAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    setupKnob (drive,    "DRIVE",    "harmonics",  "drive");
    setupKnob (body,     "BODY",     "weight",     "body");
    setupKnob (presence, "PRESENCE", "definition", "presence");
    setupKnob (control,  "CONTROL",  "glue",       "control");
    setupKnob (mix,      "MIX",      "parallel",   "mix");
    setupKnob (output,   "OUTPUT",   "trim",       "output");

    setResizable (true, true);
    setResizeLimits (760, 360, 1300, 680);
    setSize (980, 450);
}

PrimaTakeFinishAudioProcessorEditor::~PrimaTakeFinishAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void PrimaTakeFinishAudioProcessorEditor::setupKnob (Knob& knob,
                                                       const juce::String& name,
                                                       const juce::String& hintText,
                                                       const juce::String& parameterID)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f,
                                     juce::MathConstants<float>::pi * 2.78f,
                                     true);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 26);
    knob.slider.setDoubleClickReturnValue (true, 0.0);
    knob.slider.setPopupDisplayEnabled (false, false, this);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, juce::Colour (0xfff4f4f5));
    knob.label.setFont (juce::Font (14.0f, juce::Font::bold));
    addAndMakeVisible (knob.label);

    knob.hint.setText (hintText.toUpperCase(), juce::dontSendNotification);
    knob.hint.setJustificationType (juce::Justification::centred);
    knob.hint.setColour (juce::Label::textColourId, juce::Colour (0xff777c84));
    knob.hint.setFont (juce::Font (10.0f));
    addAndMakeVisible (knob.hint);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), parameterID, knob.slider);
}

void PrimaTakeFinishAudioProcessorEditor::drawHeader (juce::Graphics& g,
                                                       juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xffe65045));
    g.fillRoundedRectangle ({ area.getX(), area.getY(), 44.0f, 4.0f }, 2.0f);

    g.setColour (juce::Colour (0xfff5f5f6));
    g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.drawText ("PRIMA TAKE",
                area.getX(),
                area.getY() + 13.0f,
                area.getWidth() * 0.55f,
                35.0f,
                juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffa4a8ae));
    g.setFont (juce::Font (13.0f));
    g.drawText ("FINISH",
                area.getX() + 2.0f,
                area.getY() + 50.0f,
                90.0f,
                22.0f,
                juce::Justification::centredLeft);

    auto badge = juce::Rectangle<float> (area.getRight() - 82.0f,
                                         area.getY() + 18.0f,
                                         82.0f,
                                         28.0f);
    g.setColour (juce::Colour (0xff1e2126));
    g.fillRoundedRectangle (badge, 14.0f);
    g.setColour (juce::Colour (0xff3a3f46));
    g.drawRoundedRectangle (badge, 14.0f, 1.0f);

    g.setColour (juce::Colour (0xffb8bcc2));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("v0.3  •  MAC",
                badge.toNearestInt(),
                juce::Justification::centred);

    g.setColour (juce::Colour (0xff555a61));
    g.drawLine (area.getX(),
                area.getBottom() - 1.0f,
                area.getRight(),
                area.getBottom() - 1.0f,
                1.0f);
}

void PrimaTakeFinishAudioProcessorEditor::drawControlCard (juce::Graphics& g,
                                                            juce::Rectangle<float> area,
                                                            bool highlighted)
{
    const auto top = highlighted ? juce::Colour (0xff202329)
                                 : juce::Colour (0xff1b1e23);
    const auto bottom = juce::Colour (0xff15171b);

    juce::ColourGradient gradient (top,
                                   area.getCentreX(), area.getY(),
                                   bottom,
                                   area.getCentreX(), area.getBottom(),
                                   false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (area, 14.0f);

    g.setColour (highlighted ? juce::Colour (0xff3c4149)
                             : juce::Colour (0xff2b2f35));
    g.drawRoundedRectangle (area, 14.0f, 1.0f);
}

void PrimaTakeFinishAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (juce::Colour (0xff17191d),
                                     bounds.getCentreX(), bounds.getY(),
                                     juce::Colour (0xff0d0f12),
                                     bounds.getCentreX(), bounds.getBottom(),
                                     false);
    g.setGradientFill (background);
    g.fillAll();

    // Very subtle vignette / lower panel
    auto lower = bounds.withTrimmedTop (104.0f);
    g.setColour (juce::Colour (0x18000000));
    g.fillRoundedRectangle (lower.reduced (14.0f, 10.0f), 20.0f);

    auto content = bounds.reduced (26.0f);
    auto header = content.removeFromTop (82.0f);
    drawHeader (g, header);

    content.removeFromTop (18.0f);

    const float gap = 12.0f;
    const float cardWidth = (content.getWidth() - gap * 5.0f) / 6.0f;

    for (int i = 0; i < 6; ++i)
    {
        auto card = juce::Rectangle<float> (content.getX() + i * (cardWidth + gap),
                                             content.getY(),
                                             cardWidth,
                                             content.getHeight() - 8.0f);
        drawControlCard (g, card, i == 0 || i == 3);
    }

    g.setColour (juce::Colour (0xff666b72));
    g.setFont (juce::Font (10.0f));
    g.drawText ("SATURATION     •     TONE     •     CONTROL     •     PARALLEL",
                28,
                getHeight() - 23,
                getWidth() - 56,
                16,
                juce::Justification::centred);
}

void PrimaTakeFinishAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (26);
    area.removeFromTop (100);

    const int gap = 12;
    const int cardWidth = (area.getWidth() - gap * 5) / 6;

    for (auto* knob : knobs)
    {
        auto cell = area.removeFromLeft (cardWidth).reduced (8, 8);

        knob->label.setBounds (cell.removeFromTop (27));
        knob->hint.setBounds  (cell.removeFromTop (17));

        cell.removeFromTop (3);
        knob->slider.setBounds (cell.reduced (1, 0));

        area.removeFromLeft (gap);
    }
}
