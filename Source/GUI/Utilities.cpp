/*
  ==============================================================================

    Utilities.cpp
    Created: 30 Oct 2021 1:02:08am
    Author:  matkatmusic

  ==============================================================================
*/

#include "Utilities.h"

#include "LookAndFeel.h"

namespace SimpleMBComp
{
juce::String getValString(const juce::RangedAudioParameter& param,
                          bool getLow,
                          juce::String suffix)
{
    juce::String str;
    
    auto val = getLow ? param.getNormalisableRange().start :
                        param.getNormalisableRange().end;
    
    bool useK = truncateKiloValue(val);
    str << val;
    
    if( useK )
        str << "k";
    
    str << suffix;
    
    return str;
}

juce::Rectangle<int> getModuleBackgroundArea(juce::Rectangle<int> bounds)
{
    bounds.reduce(3, 3);
    
    return bounds;
}

void drawModuleBackground(juce::Graphics &g,
                          juce::Rectangle<int> bounds)
{
    using namespace juce;
    g.setColour(ColorScheme::getModuleBorderColor());
    g.fillAll();
    
    auto localBounds = bounds;
    
    bounds.reduce(3, 3);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 3);
    
    g.drawRect(localBounds);
}
} //end namespace SimpleMBComp
