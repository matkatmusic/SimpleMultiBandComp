/*
  ==============================================================================

    LookAndFeel.cpp
    Created: 30 Oct 2021 12:57:21am
    Author:  matkatmusic

  ==============================================================================
*/

#include "LookAndFeel.h"
#include "RotarySliderWithLabels.h"
#include "CustomButtons.h"

void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    using namespace ColorScheme;
    
    auto bounds = Rectangle<int>(x, y, width, height).toFloat();
    
    auto enabled = slider.isEnabled();
    
//    auto fillColour = Colour(97u, 18u, 167u);
//    auto fillColour = JUCE_LIVE_CONSTANT(Colour(0xff1e0732));
    g.setColour(enabled ? getSliderFillColor() : Colours::darkgrey );
    g.fillEllipse(bounds);
    
    g.setColour(enabled ? getSliderBorderColor() : Colours::grey);
    g.drawEllipse(bounds, 2.f);
    
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom( juce::jmax(center.getY() - rswl->getTextHeight() * 1.5f,
                                bounds.getY() + 15));
        
        p.addRoundedRectangle(r, 2.f);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        
        g.fillPath(p);
        
        g.setFont(static_cast<float>(rswl->getTextHeight()));
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(static_cast<float>(strWidth + 4), 
                  static_cast<float>(rswl->getTextHeight() + 2));
        r.setCentre(bounds.getCentre());
        
//        g.setColour(enabled ? Colours::black : Colours::darkgrey);
//        g.fillRect(r);
        
        g.setColour(enabled ? ColorScheme::getTitleColor() : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool /*shouldDrawButtonAsHighlighted*/,
                                   bool /*shouldDrawButtonAsDown*/)
{
    using namespace juce;
    
    if( auto* pb = dynamic_cast<PowerButton*>(&toggleButton) )
    {
        Path powerButton;
        
        auto bounds = toggleButton.getLocalBounds();
        
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
        
        float ang = 30.f; //30.f;
        
        size -= 6;
        
        powerButton.addCentredArc(r.getCentreX(),
                                  r.getCentreY(),
                                  size * 0.5f,
                                  size * 0.5f,
                                  0.f,
                                  degreesToRadians(ang),
                                  degreesToRadians(360.f - ang),
                                  true);
        
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : ColorScheme::getSliderRangeTextColor();
        
        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if( auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton) )
    {
        auto color = ! toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        
        g.setColour(color);
        
        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        
        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
    else
    {
        auto bounds = toggleButton.getLocalBounds().reduced(2);
        
        auto buttonIsOn = toggleButton.getToggleState();
        
        const int cornerSize = 4;

        g.setColour(buttonIsOn ?
                    toggleButton.findColour(TextButton::ColourIds::buttonOnColourId) :
                    toggleButton.findColour(TextButton::ColourIds::buttonColourId));
        g.fillRoundedRectangle(bounds.toFloat(), cornerSize);
        
        g.setColour(buttonIsOn ? Colours::white : ColorScheme::getTitleColor());
        g.drawRoundedRectangle(bounds.toFloat(), cornerSize, 1);
        g.setColour(buttonIsOn ? Colours::white : ColorScheme::getTitleColor());
        g.drawFittedText(toggleButton.getButtonText(), bounds, Justification::centred, 1);
    }
}
