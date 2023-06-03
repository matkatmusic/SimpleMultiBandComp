/*
  ==============================================================================

    AnalyzerPathGenerator.h
    Created: 30 Oct 2021 11:44:48am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace SimpleMBComp
{
template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
//        auto bottom = fftBounds.getHeight();
        auto bottom = fftBounds.getBottom();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
//                              negativeInfinity, 0.f,
                              negativeInfinity, SimpleMBComp::MAX_DB,
//                              float(bottom+10),   top);
                              bottom, top);
        };

        auto y = map(renderData[0]);

//        jassert( !std::isnan(y) && !std::isinf(y) );
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[static_cast<size_t>(binNum)]);

//            jassert( !std::isnan(y) && !std::isinf(y) );

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, MIN_FREQUENCY, MAX_FREQUENCY);
                int binX = static_cast<int>(std::floor(normalizedBinX * width));
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

}// end namespace SimpleMBComp
