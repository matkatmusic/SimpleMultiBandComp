/*
  ==============================================================================

    SpectrumAnalyzer.h
    Created: 30 Oct 2021 11:44:32am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "PathProducer.h"

struct SpectrumAnalyzer: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    SpectrumAnalyzer(SimpleMBCompAudioProcessor&);
    ~SpectrumAnalyzer();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { }
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
private:
    SimpleMBCompAudioProcessor& audioProcessor;

    bool shouldShowFFTAnalysis = true;

    juce::Atomic<bool> parametersChanged { false };
    
//    void drawBackgroundGrid(juce::Graphics& g);
    void drawBackgroundGrid(juce::Graphics& g,
                            juce::Rectangle<int> bounds);
    
    void drawTextLabels(juce::Graphics& g,
                        juce::Rectangle<int> bounds);
    
    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    juce::Rectangle<int> getRenderArea(juce::Rectangle<int> bounds);
    
    juce::Rectangle<int> getAnalysisArea(juce::Rectangle<int> bounds);
    
    PathProducer leftPathProducer, rightPathProducer;
};
