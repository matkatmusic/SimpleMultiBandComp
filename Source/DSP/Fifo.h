/*
  ==============================================================================

    Fifo.h
    Created: 30 Oct 2021 11:46:55am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace SimpleMBComp
{
template<typename T>
struct IsReferenceCountedObjectPtr : std::false_type { };

template<typename T>
struct IsReferenceCountedObjectPtr<juce::ReferenceCountedObjectPtr<T>> : std::true_type { };

template<typename T>
struct IsReferenceCountedArray : std::false_type { };

template<typename T>
struct IsReferenceCountedArray<juce::ReferenceCountedArray<T>> : std::true_type { };

template<typename T>
concept HasSetSize = requires(T t, int chan, int samp)
{
    { t.setSize(chan, samp) };
};

template<typename T>
concept HasResize = requires( T t, size_t num )
{
    { t.resize(num) };
};

template<typename T>
concept HasSize = requires( T t )
{
    { t.size() } -> std::same_as<size_t>;
};

template<typename T>
concept HasGetNumSamples = requires( T t )
{
    { t.getNumSamples() } -> std::same_as<int>;
};

template<typename T, size_t Size = 30>
struct Fifo
{
    using Type = T;
    size_t getCapacity() const { return Size; }
    
    template<typename BufferType = T>
    requires( HasSetSize<BufferType> )
    void prepare(int numChannels, int numSamples)
    {
        for( auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,   //clear everything?
                           true,    //including the extra space?
                           true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }
    
    void prepareUsing(std::function<void(T&)>&& prepareFunc)
    {
        for( auto& buffer : buffers )
        {
            prepareFunc(buffer);
        }
    }
    
    template<typename VectorType = T>
    requires( HasResize<VectorType> )
    void prepare(size_t numElements)
    {
        for( auto& buffer : buffers )
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }
    
    template<typename ModifyFunc>
    requires std::is_invocable_v<ModifyFunc, T&>
    bool pushWithModification(const T& t,
                              ModifyFunc&& modifyFunc)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            auto copy = t;
            modifyFunc(copy);
            buffers[static_cast<size_t>(write.startIndex1)] = std::move(copy);
            return true;
        }
        
        return false;
    }
    
    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            buffers[static_cast<size_t>(write.startIndex1)] = t;
            return true;
        }
        
        return false;
    }
    
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            t = buffers[static_cast<size_t>(read.startIndex1)];
            return true;
        }
        
        return false;
    }
    
    bool exchange(T&& t)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            auto idx = static_cast<size_t>(read.startIndex1);
            if constexpr( IsReferenceCountedObjectPtr<T>::value)
            {
                jassert(t.get() == nullptr);
                
                std::swap(t, buffers[idx]);
                jassert(buffers[idx].get() == nullptr );
            }
            else if constexpr( IsReferenceCountedArray<T>::value)
            {
                std::swap(buffers[idx], t);
                jassert( buffers[idx].size() == 0 );
            }
            else if constexpr ( HasSize<T> )
            {
                if( t.size() < buffers[idx].size() )
                {
                    t = buffers[idx]; //can't swap.  must copy.
                }
                else
                {
                    std::swap( t, buffers[idx] ); //ok to swap.
                }
            }
            else if constexpr( HasGetNumSamples<T> )
            {
                if( t.getNumSamples() < buffers[idx].getNumSamples() )
                {
                    t = buffers[idx]; //can't swap.  must copy
                }
                else
                {
                    std::swap( t, buffers[idx]); //ok to swap.
                }
            }
            else //T is something else.  blindly swap.  this leaves buffer[x] in a potentially invalid state that could allocate the next time it's used.
            {
                std::swap( t, buffers[idx]);
//                jassertfalse;
            }
                    
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
    
    int getFreeSpace() const
    {
        return fifo.getFreeSpace();
    }
private:
    std::array<T, Size> buffers;
    juce::AbstractFifo fifo {Size};
};

} //end namespace SimpleMBComp
