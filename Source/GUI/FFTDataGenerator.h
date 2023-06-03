/*
  ==============================================================================

    FFTDataGenerator.h
    Created: 30 Oct 2021 11:27:47am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Utilities.h"
#include "../DSP/Fifo.h"

namespace SimpleMBComp
{

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        // first apply a windowing function to our data
        window->multiplyWithWindowingTable (fftData.data(),
                                            static_cast<size_t>(fftSize));       // [1]
        
        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());  // [2]
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values.
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[static_cast<size_t>(i)];
//            fftData[i] /= (float) numBins;
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[static_cast<size_t>(i)] = v;
        }
        
        float max = negativeInfinity;
        //convert them to decibels
        for( int i = 0; i < numBins; ++i )
        {
            auto data = juce::Decibels::gainToDecibels(fftData[static_cast<size_t>(i)],
                                                       negativeInfinity);
            fftData[static_cast<size_t>(i)] = data;
            max = juce::jmax(data, max);
        }
        
//        jassertfalse;
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>
        
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(static_cast<size_t>(fftSize * 2), 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& data) { return fftDataFifo.pull(data); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    Fifo<BlockType> fftDataFifo;
};

} //end namespace SimpleMBComp
