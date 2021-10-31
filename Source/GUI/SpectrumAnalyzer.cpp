/*
  ==============================================================================

    SpectrumAnalyzer.cpp
    Created: 30 Oct 2021 11:44:32am
    Author:  matkatmusic

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"
#include "Utilities.h"
#include "../DSP/Params.h"
#include "LookAndFeel.h"

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer(SimpleMBCompAudioProcessor& p) :
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    using namespace Params;
    const auto& paramNames = GetParams();
    
    auto floatHelper = [&apvts = audioProcessor.apvts, &paramNames](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramNames.at(paramName)));
        jassert(param != nullptr);
    };
    
    floatHelper(lowMidXoverParam, Names::Low_Mid_Crossover_Freq);
    floatHelper(midHighXoverParam, Names::Mid_High_Crossover_Freq);
    
    floatHelper(lowThresholdParam, Names::Threshold_Low_Band);
    floatHelper(midThresholdParam, Names::Threshold_Mid_Band);
    floatHelper(highThresholdParam, Names::Threshold_High_Band);
    
    startTimerHz(60);
}

void SpectrumAnalyzer::drawFFTAnalysis(juce::Graphics &g, juce::Rectangle<int> bounds)
{
    using namespace juce;
    auto responseArea = getAnalysisArea(bounds);
    
    Graphics::ScopedSaveState sss(g);
    g.reduceClipRegion(responseArea);
    
    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), 0
                                                                    //responseArea.getY()
                                                                    ));
    
//    g.setColour(Colour(97u, 18u, 167u)); //purple-
    g.setColour(ColorScheme::getInputSignalColor());
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
    
    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), 0
                                                                     //responseArea.getY()
                                                                     ));
    
//    g.setColour(Colour(215u, 201u, 134u));
    g.setColour(ColorScheme::getOutputSignalColor());
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
}

void SpectrumAnalyzer::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    auto bounds = drawModuleBackground(g, getLocalBounds());

    drawBackgroundGrid(g, bounds);
    
    
    
    if( shouldShowFFTAnalysis )
    {
        drawFFTAnalysis(g, bounds);
    }
    
//    Path border;
//
//    border.setUsingNonZeroWinding(false);
//
//    border.addRoundedRectangle(getRenderArea(bounds), 4);
//    border.addRectangle(getLocalBounds());
//
//    g.setColour(Colours::black);
    
//    g.fillPath(border);
    
    drawCrossovers(g, bounds);
    
    drawTextLabels(g, bounds);
    
//    g.setColour(Colours::orange);
//    g.drawRoundedRectangle(getRenderArea(bounds).toFloat(), 4.f, 1.f);
}

void SpectrumAnalyzer::drawCrossovers(juce::Graphics &g, juce::Rectangle<int> bounds)
{
    using namespace juce;
    bounds = getAnalysisArea(bounds);
    
    const float top { static_cast<float>(bounds.getY()) };
    const float bottom { static_cast<float>(bounds.getBottom()) };
    
    g.setColour(ColorScheme::getGainReductionColor().withAlpha(0.5f));
    auto lowMidX = mapX(lowMidXoverParam->get(), bounds.toFloat());
     
    auto zeroDb = mapY(0.f, bottom, top);
    
    g.fillRect(Rectangle<float>::leftTopRightBottom(bounds.getX(),
                                                    zeroDb,
                                                    lowMidX,
                                                    mapY(lowBandGR, bottom, top)));
    
    auto midHighX = mapX(midHighXoverParam->get(), bounds.toFloat());
    g.fillRect(Rectangle<float>::leftTopRightBottom(lowMidX,
                                                    zeroDb,
                                                    midHighX,
                                                    mapY(midBandGR, bottom, top)));
    
    g.fillRect(Rectangle<float>::leftTopRightBottom(midHighX,
                                                    zeroDb,
                                                    bounds.getRight(),
                                                    mapY(highBandGR, bottom, top)));
    
    g.setColour(Colours::lightblue);
    g.drawVerticalLine(static_cast<int>(lowMidX), top, bottom);
    g.drawVerticalLine(static_cast<int>(midHighX), top, bottom);
    
    g.setColour(ColorScheme::getThresholdColor());
    /*
     draw crossovers after GR rectangles
     */
    g.setColour(Colours::lightblue);
    g.drawVerticalLine(static_cast<int>(lowMidX), top, bottom);
    g.drawVerticalLine(static_cast<int>(midHighX), top, bottom);
    
    g.setColour(ColorScheme::getThresholdColor());
    auto drawThreshold = [&g](auto left, auto right, auto top_, auto bottom_)
    {
        g.fillRect(Rectangle<float>::leftTopRightBottom(left, top_, right, bottom_));
    };
    
    auto lowTh = mapY(lowThresholdParam->get(), bottom, top);
    auto midTh = mapY(midThresholdParam->get(), bottom, top);
    auto highTh = mapY(highThresholdParam->get(), bottom, top);
    
    auto offset = 1; //JUCE_LIVE_CONSTANT(2);
    
    drawThreshold(bounds.getX(), lowMidX, lowTh - offset, lowTh + offset);
    drawThreshold(lowMidX, midHighX, midTh - offset, midTh + offset);
    drawThreshold(midHighX, bounds.getRight(), highTh - offset, highTh + offset);
}

void SpectrumAnalyzer::update(const std::vector<float> &values)
{
    jassert(values.size() == 6);
    
    enum
    {
        LowBandIn,
        LowBandOut,
        MidBandIn,
        MidBandOut,
        HighBandIn,
        HighBandOut
    };
    
    lowBandGR = values[LowBandOut] - values[LowBandIn];
    midBandGR = values[MidBandOut] - values[MidBandIn];
    highBandGR = values[HighBandOut] - values[HighBandIn];
    
    repaint();
}

std::vector<float> SpectrumAnalyzer::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
}

std::vector<float> SpectrumAnalyzer::getGains()
{
//    return std::vector<float>
//    {
//        -24, -12, 0, 12, 24
//    };
    std::vector<float> values;
    
    auto increment = MAX_DECIBELS; //12 db steps
    for( auto db = NEGATIVE_INFINITY; db <= MAX_DECIBELS; db += increment)
    {
        values.push_back(db);
    }
    
    return values;
}

std::vector<float> SpectrumAnalyzer::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    for( auto f : freqs )
    {
        auto normX = juce::mapFromLog10(f, MIN_FREQUENCY, MAX_FREQUENCY);
        xs.push_back( left + width * normX );
    }
    
    return xs;
}

void SpectrumAnalyzer::drawBackgroundGrid(juce::Graphics &g,
                                          juce::Rectangle<int> bounds)
{
    using namespace juce;
    auto freqs = getFrequencies();
    
    auto renderArea = getAnalysisArea(bounds);
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto xs = getXs(freqs, left, width);
    
    g.setColour(ColorScheme::getAnalyzerGridColor());
    for( auto x : xs )
    {
        g.drawVerticalLine(static_cast<int>(x), top, bottom);
    }
    
    auto gain = getGains();
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, NEGATIVE_INFINITY, MAX_DECIBELS,
                      float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? ColorScheme::getSliderRangeTextColor().withAlpha(0.75f) : ColorScheme::getAnalyzerGridColor() );
        g.drawHorizontalLine(static_cast<int>(y), left, right);
    }
}

void SpectrumAnalyzer::drawTextLabels(juce::Graphics &g, juce::Rectangle<int> bounds)
{
    using namespace juce;
    g.setColour(ColorScheme::getScaleTextColor());
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    auto renderArea = getAnalysisArea(bounds);
    auto left = renderArea.getX() + 1;
    
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);
    
    for( size_t i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(static_cast<int>(x), 0);
        r.setY(bounds.getY());
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for( auto gDb : gain )
    {
        auto y = jmap(gDb, NEGATIVE_INFINITY, MAX_DECIBELS,
                      float(bottom), float(top));
        
        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(bounds.getRight() - textWidth);
        r.setCentre(r.getCentreX(), static_cast<int>(y));
        
        g.setColour(gDb == 0.f ? ColorScheme::getSliderRangeTextColor() : ColorScheme::getScaleTextColor() );
        
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        r.setX(bounds.getX() + 1);

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void SpectrumAnalyzer::resized()
{
    using namespace juce;
    auto bounds = getLocalBounds();
    auto fftBounds = getAnalysisArea(bounds).toFloat();
    auto negInf = jmap(bounds.toFloat().getBottom(),
                       fftBounds.getBottom(), fftBounds.getY(),
//                       -48.f, 0.f);
                       NEGATIVE_INFINITY, MAX_DECIBELS);
    DBG( "Negative infinity: " << negInf );
    leftPathProducer.updateNegativeInfinity(negInf);
    rightPathProducer.updateNegativeInfinity(negInf);
}

void SpectrumAnalyzer::timerCallback()
{
    if( shouldShowFFTAnalysis )
    {
        auto bounds = getLocalBounds();
        auto fftBounds = getAnalysisArea(bounds).toFloat();
        fftBounds.setBottom(bounds.getBottom());
        auto sampleRate = audioProcessor.getSampleRate();
        
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }
    
    repaint();
}

juce::Rectangle<int> SpectrumAnalyzer::getRenderArea(juce::Rectangle<int> bounds)
{
//    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    
    return bounds;
}

juce::Rectangle<int> SpectrumAnalyzer::getAnalysisArea(juce::Rectangle<int> bounds)
{
    bounds = getRenderArea(bounds);
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
