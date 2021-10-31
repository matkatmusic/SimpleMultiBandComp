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

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer(SimpleMBCompAudioProcessor& p) :
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }
    
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

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
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
    
    g.setColour(Colour(97u, 18u, 167u)); //purple-
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
    
    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), 0
                                                                     //responseArea.getY()
                                                                     ));
    
    g.setColour(Colour(215u, 201u, 134u));
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
    
    const auto top = bounds.getY();
    const auto bottom = bounds.getBottom();
    const auto left = bounds.getX();
    const auto right = bounds.getRight();
    
    auto mapX = [left = bounds.getX(),
                 width = bounds.getWidth()]
                (float frequency)
    {
        auto normX = juce::mapFromLog10(frequency,
                                        MIN_FREQUENCY,
                                        MAX_FREQUENCY);
        return left + width * normX;
    };
    
    auto lowMidX = mapX(lowMidXoverParam->get());
    g.setColour(Colours::orange);
    g.drawVerticalLine(lowMidX, top, bottom);
    
    auto midHighX = mapX(midHighXoverParam->get());
    g.drawVerticalLine(midHighX, top, bottom);
    
    auto mapY = [bottom, top](float db)
    {
        return jmap(db, NEGATIVE_INFINITY, MAX_DECIBELS,
                    (float)bottom, (float)top);
    };
    
    auto zeroDb = mapY(0.f);
    g.setColour(Colours::hotpink.withAlpha(0.3f));
    
    g.fillRect(Rectangle<float>::leftTopRightBottom(left,
                                                    zeroDb,
                                                    lowMidX,
                                                    mapY(lowBandGR)));
    g.fillRect(Rectangle<float>::leftTopRightBottom(lowMidX,
                                                    zeroDb,
                                                    midHighX,
                                                    mapY(midBandGR)));
    g.fillRect(Rectangle<float>::leftTopRightBottom(midHighX,
                                                    zeroDb,
                                                    right,
                                                    mapY(highBandGR)));
    
    g.setColour(Colours::yellow);
    g.drawHorizontalLine(mapY(lowThresholdParam->get()),
                         left,
                         lowMidX);
    
    g.drawHorizontalLine(mapY(midThresholdParam->get()),
                         lowMidX,
                         midHighX);
    
    g.drawHorizontalLine(mapY(highThresholdParam->get()),
                         midHighX,
                         right);
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
    
    g.setColour(Colours::dimgrey);
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }
    
    auto gain = getGains();
    
    for( auto gDb : gain )
    {
//        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        auto y = jmap(gDb, NEGATIVE_INFINITY, MAX_DECIBELS,
                      (float)bottom, (float)top);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey );
        g.drawHorizontalLine(y, left, right);
    }
}

void SpectrumAnalyzer::drawTextLabels(juce::Graphics &g, juce::Rectangle<int> bounds)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    auto renderArea = getAnalysisArea(bounds);
    auto left = renderArea.getX();
    
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);
    
    for( int i = 0; i < freqs.size(); ++i )
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
        r.setCentre(x, 0);
//        r.setY(1);
        r.setY(bounds.getY());
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for( auto gDb : gain )
    {
//        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        auto y = jmap(gDb, NEGATIVE_INFINITY, MAX_DECIBELS,
                      (float)bottom, (float)top);
        
        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
//        r.setX(getWidth() - textWidth);
        r.setX(bounds.getRight() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey );
        
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
        
//        str.clear();
//        str << (gDb - 24.f);

        r.setX(bounds.getX() + 1);
//        textWidth = g.getCurrentFont().getStringWidth(str);
//        r.setSize(textWidth, fontHeight);
//        g.setColour(Colours::lightgrey);
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

void SpectrumAnalyzer::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
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

    if( parametersChanged.compareAndSetBool(false, true) )
    {
        
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
