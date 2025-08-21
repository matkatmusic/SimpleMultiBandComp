/*
  ==============================================================================

    SingleChannelSampleFifo.h
    Created: 30 Oct 2021 11:47:03am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Fifo.h"

namespace SimpleMBComp
{
enum Channel
{
    Left, //effectively 0
    Right, //effectively 1
};

template<typename BlockType, int FifoCapacity = 30>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(int channelToUse_) : channelToUse(channelToUse_)
    {
        jassert(channelToUse >= 0 );
        prepared.set(false);
    }
    SingleChannelSampleFifo(Channel ch) : SingleChannelSampleFifo(ch == Channel::Left ? 0 : 1)
    {
        
    }
    
    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse );
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        
        for( int i = 0; i < buffer.getNumSamples(); ++i )
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1,             //channel
                             bufferSize,    //num samples
                             false,         //keepExistingContent
                             true,          //clear extra space
                             true);         //avoid reallocating
        audioBufferFifo.prepareUsing([bufferSize](BlockType& b)
                                     {
            b.setSize(1, bufferSize);
            b.clear();
        });
//        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
    int getChannelToUse() const { return channelToUse; }
private:
    const int channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType, FifoCapacity> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);
            if( !ok )
            {
#if JUCE_DEBUG
                juce::Logger::writeToLog("Warning: Single Channel Sample FIFO audioBufferFifo is full!!  the consuming thread isn't consuming buffers fast enough");
#endif
            }
            
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

} //end namespace SimpleMBComp
