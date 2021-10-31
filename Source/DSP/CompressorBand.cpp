/*
  ==============================================================================

    CompressorBand.cpp
    Created: 30 Oct 2021 1:06:32am
    Author:  matkatmusic

  ==============================================================================
*/

#include "CompressorBand.h"

void CompressorBand::prepare(const juce::dsp::ProcessSpec& spec)
{
    compressor.prepare(spec);
}

void CompressorBand::updateCompressorSettings()
{
    compressor.setAttack(attack->get());
    compressor.setRelease(release->get());
    compressor.setThreshold(threshold->get());
    compressor.setRatio( ratio->getCurrentChoiceName().getFloatValue() );
}

void CompressorBand::process(juce::AudioBuffer<float>& buffer)
{
    auto preRMS = computeRMSLevel(buffer);
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    
    context.isBypassed = bypassed->get();
    
    compressor.process(context);
    
    auto postRMS = computeRMSLevel(buffer);
    
    auto convertToDb = [](auto input)
    {
        return juce::Decibels::gainToDecibels(input);
    };
    
    rmsInputLevelDb.store(convertToDb(preRMS));
    rmsOutputLevelDb.store(convertToDb(postRMS));
}
