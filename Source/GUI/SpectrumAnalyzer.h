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
juce::Timer
{
    SpectrumAnalyzer(SimpleMBCompAudioProcessor&);
    ~SpectrumAnalyzer() override = default;
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
    
    void update(const std::vector<float>& values);
private:
    SimpleMBCompAudioProcessor& audioProcessor;

    bool shouldShowFFTAnalysis = true;
    
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
    
    void drawFFTAnalysis(juce::Graphics& g,
                         juce::Rectangle<int> bounds);
    
    void drawCrossovers(juce::Graphics& g,
                        juce::Rectangle<int> bounds);
    
    juce::AudioParameterFloat* lowMidXoverParam { nullptr };
    juce::AudioParameterFloat* midHighXoverParam { nullptr };
    
    juce::AudioParameterFloat* lowThresholdParam { nullptr };
    juce::AudioParameterFloat* midThresholdParam { nullptr };
    juce::AudioParameterFloat* highThresholdParam { nullptr };
    
    float lowBandGR { 0.f };
    float midBandGR { 0.f };
    float highBandGR { 0.f };
};
