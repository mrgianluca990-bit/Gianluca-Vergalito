#include "PluginEditor.h"
#include <cmath>

FinishLookAndFeel::FinishLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xfff4f5f7));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff111419));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
}

void FinishLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                           static_cast<float> (y),
                                           static_cast<float> (width),
                                           static_cast<float> (height))
                    .reduced (10.0f);

    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto dial = juce::Rectangle<float> (0, 0, size, size).withCentre (bounds.getCentre());

    const auto centre = dial.getCentre();
    const float angle = rotaryStartAngle
                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    const float radius = dial.getWidth() * 0.5f;
    const float ringRadius = radius - 4.0f;
    const float ringWidth = juce::jmax (3.5f, dial.getWidth() * 0.045f);

    // Shadow
    g.setColour (juce::Colour (0x88000000));
    g.fillEllipse (dial.translated (0.0f, 6.0f).expanded (3.0f));

    // Background arc
    juce::Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y,
                         ringRadius, ringRadius,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (ringOff);
    g.strokePath (bgArc,
                  juce::PathStrokeType (ringWidth,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // Parameter-specific accent
    juce::Colour accent = accentHot;
    const auto name = slider.getName();

    if (name == "BODY" || name == "PUNCH")
        accent = accentWarm;
    else if (name == "DETAIL" || name == "SPACE")
        accent = accentCool;

    juce::Path activeArc;
    activeArc.addCentredArc (centre.x, centre.y,
                             ringRadius, ringRadius,
                             0.0f, rotaryStartAngle, angle, true);

    juce::ColourGradient arcGradient (
        accent.brighter (0.18f),
        centre.x - radius, centre.y - radius,
        accent.darker (0.12f),
        centre.x + radius, centre.y + radius,
        false);

    g.setGradientFill (arcGradient);
    g.strokePath (activeArc,
                  juce::PathStrokeType (ringWidth,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // Knob body
    auto knob = dial.reduced (ringWidth + 8.0f);

    juce::ColourGradient knobGradient (
        juce::Colour (0xff3b414a),
        knob.getCentreX(), knob.getY(),
        juce::Colour (0xff14171c),
        knob.getCentreX(), knob.getBottom(),
        false);

    g.setGradientFill (knobGradient);
    g.fillEllipse (knob);

    g.setColour (juce::Colour (0xff515862));
    g.drawEllipse (knob, 1.0f);

    // Inner bevel
    g.setColour (juce::Colour (0x26ffffff));
    g.drawEllipse (knob.reduced (3.0f), 1.0f);

    // Pointer
    juce::Path pointer;
    const float pointerW = juce::jmax (2.0f, size * 0.026f);
    const float pointerH = knob.getHeight() * 0.30f;

    pointer.addRoundedRectangle (-pointerW * 0.5f,
                                 -knob.getHeight() * 0.42f,
                                 pointerW, pointerH,
                                 pointerW * 0.5f);

    g.setColour (juce::Colour (0xfff5f6f7));
    g.fillPath (pointer,
                juce::AffineTransform::rotation (angle)
                    .translated (centre.x, centre.y));

    // Accent centre
    g.setColour (juce::Colour (0xff0d1014));
    g.fillEllipse (juce::Rectangle<float> (10, 10).withCentre (centre));
    g.setColour (accent);
    g.fillEllipse (juce::Rectangle<float> (4, 4).withCentre (centre));
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

PrimaTakeFinishAudioProcessorEditor::PrimaTakeFinishAudioProcessorEditor (
    PrimaTakeFinishAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&finishLook);

    setupKnob (drive,  "DRIVE",  "adaptive density", "drive");
    setupKnob (body,   "BODY",   "low-end weight",   "body");
    setupKnob (detail, "DETAIL", "air + definition", "detail");
    setupKnob (glue,   "GLUE",   "program control",  "glue");
    setupKnob (punch,  "PUNCH",  "transient shape",  "punch");
    setupKnob (space,  "SPACE",  "stereo dimension", "space");
    setupKnob (mix,    "MIX",    "parallel blend",   "mix");
    setupKnob (output, "OUTPUT", "final trim",       "output");

    setResizable (true, true);
    setResizeLimits (860, 520, 1450, 900);
    setSize (1120, 650);

    startTimerHz (30);
}

PrimaTakeFinishAudioProcessorEditor::~PrimaTakeFinishAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PrimaTakeFinishAudioProcessorEditor::setupKnob (
    Knob& knob,
    const juce::String& name,
    const juce::String& hint,
    const juce::String& parameterID)
{
    knob.slider.setName (name);
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f,
                                     juce::MathConstants<float>::pi * 2.78f,
                                     true);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 92, 25);
    knob.slider.setPopupDisplayEnabled (false, false, this);
    addAndMakeVisible (knob.slider);

    knob.name.setText (name, juce::dontSendNotification);
    knob.name.setJustificationType (juce::Justification::centred);
    knob.name.setColour (juce::Label::textColourId, juce::Colour (0xfff6f6f7));
    knob.name.setFont (juce::Font (14.0f, juce::Font::bold));
    addAndMakeVisible (knob.name);

    knob.hint.setText (hint.toUpperCase(), juce::dontSendNotification);
    knob.hint.setJustificationType (juce::Justification::centred);
    knob.hint.setColour (juce::Label::textColourId, juce::Colour (0xff727984));
    knob.hint.setFont (juce::Font (9.5f));
    addAndMakeVisible (knob.hint);

    knob.attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.getAPVTS(), parameterID, knob.slider);
}

void PrimaTakeFinishAudioProcessorEditor::drawTopBar (
    juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xfff2f4f6));
    g.setFont (juce::Font (31.0f, juce::Font::bold));
    g.drawText ("PRIMA TAKE",
                area.getX(), area.getY() + 5.0f,
                260.0f, 36.0f,
                juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffff5d4d));
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText ("FINISH ENGINE",
                area.getX() + 2.0f, area.getY() + 42.0f,
                150.0f, 22.0f,
                juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff6d737c));
    g.setFont (juce::Font (11.0f));
    g.drawText ("ADAPTIVE DENSITY  •  TRANSIENT SHAPE  •  PROGRAM GLUE  •  SPACE",
                area.getX() + 165.0f, area.getY() + 43.0f,
                area.getWidth() - 330.0f, 22.0f,
                juce::Justification::centredLeft);

    auto badge = juce::Rectangle<float> (
        area.getRight() - 110.0f,
        area.getY() + 12.0f,
        110.0f, 32.0f);

    g.setColour (juce::Colour (0xff1c2128));
    g.fillRoundedRectangle (badge, 16.0f);
    g.setColour (juce::Colour (0xff343b45));
    g.drawRoundedRectangle (badge, 16.0f, 1.0f);

    g.setColour (juce::Colour (0xffaeb4bc));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("v0.4  •  UNIQUE",
                badge.toNearestInt(),
                juce::Justification::centred);
}

void PrimaTakeFinishAudioProcessorEditor::drawMeter (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    float value,
    juce::Colour colour,
    const juce::String& label)
{
    value = juce::jlimit (0.0f, 1.0f, value);

    g.setColour (juce::Colour (0xff0d1116));
    g.fillRoundedRectangle (area, 5.0f);

    auto fill = area.reduced (3.0f);
    fill.setWidth (fill.getWidth() * value);

    juce::ColourGradient meterGradient (
        colour.darker (0.18f),
        fill.getX(), fill.getCentreY(),
        colour.brighter (0.12f),
        fill.getRight(), fill.getCentreY(),
        false);

    g.setGradientFill (meterGradient);
    g.fillRoundedRectangle (fill, 3.0f);

    g.setColour (juce::Colour (0xff6e7680));
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText (label,
                static_cast<int> (area.getX()),
                static_cast<int> (area.getY() - 15.0f),
                static_cast<int> (area.getWidth()),
                12,
                juce::Justification::centredLeft);
}

void PrimaTakeFinishAudioProcessorEditor::drawEngine (
    juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient panel (
        juce::Colour (0xff1a1f26),
        area.getCentreX(), area.getY(),
        juce::Colour (0xff10141a),
        area.getCentreX(), area.getBottom(),
        false);

    g.setGradientFill (panel);
    g.fillRoundedRectangle (area, 18.0f);

    g.setColour (juce::Colour (0xff2b333d));
    g.drawRoundedRectangle (area, 18.0f, 1.0f);

    auto left = area.reduced (22.0f);
    auto meterArea = left.removeFromRight (250.0f);

    g.setColour (juce::Colour (0xfff1f3f5));
    g.setFont (juce::Font (17.0f, juce::Font::bold));
    g.drawText ("LIVE FINISH CORE",
                left.getX(), left.getY(),
                220.0f, 24.0f,
                juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff747b85));
    g.setFont (juce::Font (11.0f));
    g.drawText ("The engine reacts to level, transient shape and stereo energy.",
                left.getX(), left.getY() + 26.0f,
                left.getWidth() - 20.0f, 20.0f,
                juce::Justification::centredLeft);

    auto meters = meterArea.reduced (4.0f, 8.0f);
    auto row = meters.removeFromTop (15.0f);
    juce::ignoreUnused (row);

    drawMeter (g, meters.removeFromTop (13.0f),
               processor.getInputMeter(),
               juce::Colour (0xff52d3ff), "INPUT");

    meters.removeFromTop (14.0f);

    drawMeter (g, meters.removeFromTop (13.0f),
               processor.getEnergyMeter(),
               juce::Colour (0xffff9f4d), "ENERGY");

    meters.removeFromTop (14.0f);

    drawMeter (g, meters.removeFromTop (13.0f),
               processor.getOutputMeter(),
               juce::Colour (0xffff5d4d), "OUTPUT");
}

void PrimaTakeFinishAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (
        juce::Colour (0xff171b21),
        bounds.getCentreX(), bounds.getY(),
        juce::Colour (0xff090c10),
        bounds.getCentreX(), bounds.getBottom(),
        false);

    g.setGradientFill (background);
    g.fillAll();

    // subtle top glow
    juce::ColourGradient glow (
        juce::Colour (0x22ff5d4d),
        bounds.getCentreX(), 0.0f,
        juce::Colours::transparentBlack,
        bounds.getCentreX(), 180.0f,
        false);
    g.setGradientFill (glow);
    g.fillRect (bounds.withHeight (190.0f));

    auto content = bounds.reduced (28.0f);

    auto top = content.removeFromTop (76.0f);
    drawTopBar (g, top);

    content.removeFromTop (10.0f);

    auto engine = content.removeFromTop (100.0f);
    drawEngine (g, engine);

    content.removeFromTop (14.0f);

    const float rowGap = 14.0f;
    auto topRow = content.removeFromTop ((content.getHeight() - rowGap) * 0.52f);
    content.removeFromTop (rowGap);
    auto bottomRow = content;

    auto drawCards = [&g] (juce::Rectangle<float> row, int count)
    {
        const float gap = 12.0f;
        const float w = (row.getWidth() - gap * static_cast<float> (count - 1))
                      / static_cast<float> (count);

        for (int i = 0; i < count; ++i)
        {
            auto card = juce::Rectangle<float> (
                row.getX() + static_cast<float> (i) * (w + gap),
                row.getY(), w, row.getHeight());

            juce::ColourGradient cardGradient (
                juce::Colour (0xff191e25),
                card.getCentreX(), card.getY(),
                juce::Colour (0xff11151a),
                card.getCentreX(), card.getBottom(),
                false);

            g.setGradientFill (cardGradient);
            g.fillRoundedRectangle (card, 15.0f);

            g.setColour (juce::Colour (0xff272e37));
            g.drawRoundedRectangle (card, 15.0f, 1.0f);
        }
    };

    drawCards (topRow, 4);
    drawCards (bottomRow, 4);

    g.setColour (juce::Colour (0xff4d5560));
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("FINISH • CHARACTER WITHOUT LOSING THE SOURCE",
                28, getHeight() - 20, getWidth() - 56, 14,
                juce::Justification::centred);
}

void PrimaTakeFinishAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28);
    area.removeFromTop (86);
    area.removeFromTop (100);
    area.removeFromTop (14);

    const int rowGap = 14;
    auto topRow = area.removeFromTop ((area.getHeight() - rowGap) / 2);
    area.removeFromTop (rowGap);
    auto bottomRow = area;

    auto layoutRow = [] (juce::Rectangle<int> row,
                         std::array<Knob*, 4> rowKnobs)
    {
        const int gap = 12;
        const int width = (row.getWidth() - gap * 3) / 4;

        for (auto* knob : rowKnobs)
        {
            auto cell = row.removeFromLeft (width).reduced (8, 8);

            knob->name.setBounds (cell.removeFromTop (25));
            knob->hint.setBounds (cell.removeFromTop (16));
            cell.removeFromTop (2);
            knob->slider.setBounds (cell);

            row.removeFromLeft (gap);
        }
    };

    layoutRow (topRow, { &drive, &body, &detail, &glue });
    layoutRow (bottomRow, { &punch, &space, &mix, &output });
}

void PrimaTakeFinishAudioProcessorEditor::timerCallback()
{
    repaint();
}
