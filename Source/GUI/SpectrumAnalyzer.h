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

namespace SimpleMBComp
{
struct SpectrumAnalyzerUtils
{
    static juce::Rectangle<int> getRenderArea(juce::Rectangle<int> bounds);
    
    static juce::Rectangle<int> getAnalysisArea(juce::Rectangle<int> bounds);
};

struct SpectrumAnalyzer: juce::Component,
juce::Timer
{
    using SCSF = SingleChannelSampleFifo<juce::AudioBuffer<float>>;
    SpectrumAnalyzer(juce::AudioProcessor& processor, SCSF& left, SCSF& right);
    ~SpectrumAnalyzer() override = default;
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
    
private:
    double sampleRate;
    bool shouldShowFFTAnalysis = true;
    
    void drawBackgroundGrid(juce::Graphics& g,
                            juce::Rectangle<int> bounds);
    
    void drawTextLabels(juce::Graphics& g,
                        juce::Rectangle<int> bounds);
    
    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    PathProducer<juce::AudioBuffer<float>> leftPathProducer, rightPathProducer;
    
    void drawFFTAnalysis(juce::Graphics& g,
                         juce::Rectangle<int> bounds);
};

struct MBCompAnalyzerOverlay : juce::Component, juce::Timer
{
    MBCompAnalyzerOverlay(juce::AudioParameterFloat& lowXover,
                          juce::AudioParameterFloat& midXover,
                          juce::AudioParameterFloat& lowThresh,
                          juce::AudioParameterFloat& midThresh,
                          juce::AudioParameterFloat& highThresh);
                            
    void drawCrossovers(juce::Graphics& g,
                        juce::Rectangle<int> bounds);
    
    
    void update(const std::vector<float>& values);
    
    void paint(juce::Graphics& g) override;
    
    void timerCallback() override;
    
    juce::AudioParameterFloat* lowMidXoverParam { nullptr };
    juce::AudioParameterFloat* midHighXoverParam { nullptr };
    
    juce::AudioParameterFloat* lowThresholdParam { nullptr };
    juce::AudioParameterFloat* midThresholdParam { nullptr };
    juce::AudioParameterFloat* highThresholdParam { nullptr };
    
    float lowBandGR { 0.f };
    float midBandGR { 0.f };
    float highBandGR { 0.f };
};

} //end namespace SimpleMBComp
