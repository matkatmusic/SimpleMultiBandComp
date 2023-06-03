/*
  ==============================================================================

    PathProducer.h
    Created: 30 Oct 2021 11:44:41am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "FFTDataGenerator.h"
#include "AnalyzerPathGenerator.h"
#include "../PluginProcessor.h"

namespace SimpleMBComp
{
template<typename BlockType>
struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<BlockType>& scsf) :
    singleChannelSampleFifo(&scsf)
    {
        fftDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, fftDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate)
    {
        juce::AudioBuffer<float> tempIncomingBuffer;
        while( singleChannelSampleFifo->getNumCompleteBuffersAvailable() > 0 )
        {
            if( singleChannelSampleFifo->getAudioBuffer(tempIncomingBuffer) )
            {
                auto size = tempIncomingBuffer.getNumSamples();
                
                jassert(size <= monoBuffer.getNumSamples());
                size = juce::jmin(size, monoBuffer.getNumSamples());
                
                auto writePointer = monoBuffer.getWritePointer(0, 0);
                auto readPointer = monoBuffer.getReadPointer(0, size);
                
                std::copy(readPointer,
                          readPointer + (monoBuffer.getNumSamples() - size),
                          writePointer);

    //            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
    //                                              monoBuffer.getReadPointer(0, size),
    //                                              monoBuffer.getNumSamples() - size);

                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                                  tempIncomingBuffer.getReadPointer(0, 0),
                                                  size);
                
                fftDataGenerator.produceFFTDataForRendering(monoBuffer, negativeInfinity);
            }
        }
        
        const auto fftSize = fftDataGenerator.getFFTSize();
        const auto binWidth = sampleRate / double(fftSize);

        while( fftDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
        {
            std::vector<float> fftData;
            if( fftDataGenerator.getFFTData( fftData) )
            {
                pathProducer.generatePath(fftData,
                                          fftBounds,
                                          fftSize,
                                          static_cast<float>(binWidth),
                                          negativeInfinity);
            }
        }
        
        while( pathProducer.getNumPathsAvailable() > 0 )
        {
            pathProducer.getPath( fftPath );
        }
    }
    
    juce::Path getPath() { return fftPath; }
    
    void updateNegativeInfinity(float nf) { negativeInfinity = nf; }
private:
    SingleChannelSampleFifo<BlockType>* singleChannelSampleFifo;
    
    BlockType monoBuffer;
    
    FFTDataGenerator<std::vector<float>> fftDataGenerator;
    
    AnalyzerPathGenerator<juce::Path> pathProducer;
    
    juce::Path fftPath;
    
    float negativeInfinity { -48.f };
};

} //end namespace SimpleMBComp
