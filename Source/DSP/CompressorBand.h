/*
  ==============================================================================

    CompressorBand.h
    Created: 30 Oct 2021 1:06:32am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../GUI/Utilities.h"

struct CompressorBand
{
    juce::AudioParameterFloat* attack { nullptr };
    juce::AudioParameterFloat* release { nullptr };
    juce::AudioParameterFloat* threshold { nullptr };
    juce::AudioParameterChoice* ratio { nullptr };
    juce::AudioParameterBool* bypassed { nullptr };
    juce::AudioParameterBool* mute { nullptr };
    juce::AudioParameterBool* solo { nullptr };
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    
    void updateCompressorSettings();
    
    void process(juce::AudioBuffer<float>& buffer);
    
    float getRMSOutputLevelDb() const { return rmsOutputLevelDb; }
    float getRMSInputLevelDb() const { return rmsInputLevelDb; }
private:
    juce::dsp::Compressor<float> compressor;
    
    std::atomic<float> rmsInputLevelDb { SimpleMBComp::NEG_INFINITY };
    std::atomic<float> rmsOutputLevelDb { SimpleMBComp::NEG_INFINITY };
    
    template<typename T>
    float computeRMSLevel(const T& buffer)
    {
        int numChannels = static_cast<int>(buffer.getNumChannels());
        int numSamples = static_cast<int>(buffer.getNumSamples());
        auto rms = 0.f;
        for( int chan = 0; chan < numChannels; ++chan )
        {
            rms += buffer.getRMSLevel(chan, 0, numSamples);
        }
        
        rms /= static_cast<float>(numChannels);
        return rms;
    }
};
