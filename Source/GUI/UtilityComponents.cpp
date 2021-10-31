/*
  ==============================================================================

    UtilityComponents.cpp
    Created: 30 Oct 2021 1:00:51am
    Author:  matkatmusic

  ==============================================================================
*/

#include "UtilityComponents.h"

//==============================================================================
Placeholder::Placeholder()
{
    juce::Random r;
    customColor = juce::Colour(static_cast<juce::uint8>(r.nextInt(255)),
                               static_cast<juce::uint8>(r.nextInt(255)),
                               static_cast<juce::uint8>(r.nextInt(255)));
}

void Placeholder::paint(juce::Graphics& g)
{
    g.fillAll(customColor);
}
//==============================================================================
RotarySlider::RotarySlider() :
juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
             juce::Slider::TextEntryBoxPosition::NoTextBox)
{ }
