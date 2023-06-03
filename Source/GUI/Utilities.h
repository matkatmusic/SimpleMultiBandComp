/*
  ==============================================================================

    Utilities.h
    Created: 30 Oct 2021 1:02:08am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace SimpleMBComp
{
static constexpr float MIN_FREQUENCY = 20.f;
static constexpr float MAX_FREQUENCY = 20000.f;

static constexpr float NEG_INFINITY = -72.f;
static constexpr float MAX_DB = 24.f;

static constexpr float MIN_THRESHOLD = -60.f;

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<
    typename Attachment,
    typename APVTS,
    typename Params,
    typename ParamName,
    typename SliderType
        >
void makeAttachment(std::unique_ptr<Attachment>& attachment,
                    APVTS& apvts,
                    const Params& params,
                    const ParamName& name,
                    SliderType& slider)
{
    attachment = std::make_unique<Attachment>(apvts,
                                              params.at(name),
                                              slider);
}

template<
    typename APVTS,
    typename Params,
    typename Name
        >
juce::RangedAudioParameter& getParam(APVTS& apvts, const Params& params, const Name& name)
{
    auto param = apvts.getParameter(params.at(name));
    jassert( param != nullptr );
    
    return *param;
}

juce::String getValString(const juce::RangedAudioParameter& param,
                          bool getLow,
                          juce::String suffix);

template<typename T>
bool truncateKiloValue(T& value)
{
    if( value > static_cast<T>(999))
    {
        value /= static_cast<T>(1000);
        return true;
    }
    
    return false;
}

template<
    typename Labels,
    typename ParamType,
    typename SuffixType
        >
void addLabelPairs(Labels& labels, const ParamType& param, const SuffixType& suffix)
{
    labels.clear();
    labels.add({0.f, getValString(param, true, suffix)});
    labels.add({1.f, getValString(param, false, suffix)});
}

template<typename FloatType>
FloatType mapY(FloatType value, FloatType bottom, FloatType top)
{
    auto y = juce::jmap(value,
                        static_cast<FloatType>(NEG_INFINITY), static_cast<FloatType>(MAX_DB),
                        bottom, top);
    return y;
}

template<typename FloatType>
FloatType mapX(FloatType freq, juce::Rectangle<FloatType> bounds)
{
    auto x = juce::mapFromLog10(freq,
                                static_cast<FloatType>(MIN_FREQUENCY),
                                static_cast<FloatType>(MAX_FREQUENCY));
    x = x * bounds.getWidth() + bounds.getX();
    return x;
}

juce::Rectangle<int> getModuleBackgroundArea(juce::Rectangle<int> bounds);

void drawModuleBackground(juce::Graphics &g,
                                          juce::Rectangle<int> bounds);
} //end namespace SimpleMBComp
