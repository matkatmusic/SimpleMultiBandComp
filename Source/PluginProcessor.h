/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

/*
GUI RoadMap:
 1) Global Controls (x-over sliders, gain sliders)
 2) Main Band Controls (attack, release, threshold, ratio)
 3) add solo/mute/bypass buttons
 4) Band Select Functionality
 5) Band Select Buttons reflect the Solo/Mute/Bypass state
 6) Custom Look and Feel for Sliders and Toggle Buttons.
 7) Spectrum Analyzer Overview
 8) Data Structures for Spectrum Analyzer.
 9) Fifo usage in pluginProcessor::processBlock
 10) implementation of the analyzer rendering pre-computed paths.
 11) Drawing crossovers on top of the Analyzer Plot
 12) Drawing gain reduction on top of the analyzer
 13) Analyzer Bypass.
 14) Global Bypass button
 
 
 */

#include <JuceHeader.h>

namespace Params
{
enum Names
{
    Low_Mid_Crossover_Freq,
    Mid_High_Crossover_Freq,
    
    Threshold_Low_Band,
    Threshold_Mid_Band,
    Threshold_High_Band,
    
    Attack_Low_Band,
    Attack_Mid_Band,
    Attack_High_Band,
    
    Release_Low_Band,
    Release_Mid_Band,
    Release_High_Band,
    
    Ratio_Low_Band,
    Ratio_Mid_Band,
    Ratio_High_Band,
    
    Bypassed_Low_Band,
    Bypassed_Mid_Band,
    Bypassed_High_Band,
    
    Mute_Low_Band,
    Mute_Mid_Band,
    Mute_High_Band,
    
    Solo_Low_Band,
    Solo_Mid_Band,
    Solo_High_Band,
    
    Gain_In,
    Gain_Out,
};

inline const std::map<Names, juce::String>& GetParams()
{
    static std::map<Names, juce::String> params =
    {
        {Low_Mid_Crossover_Freq, "Low-Mid Crossover Freq"},
        {Mid_High_Crossover_Freq,"Mid_High Crossover Freq"},
        {Threshold_Low_Band,"Threshold Low Band"},
        {Threshold_Mid_Band,"Threshold Mid Band"},
        {Threshold_High_Band,"Threshold High Band"},
        {Attack_Low_Band,"Attack Low Band"},
        {Attack_Mid_Band,"Attack Mid Band"},
        {Attack_High_Band,"Attack High Band"},
        {Release_Low_Band,"Release Low Band"},
        {Release_Mid_Band,"Release Mid Band"},
        {Release_High_Band,"Release High Band"},
        {Ratio_Low_Band,"Ratio Low Band"},
        {Ratio_Mid_Band,"Ratio Mid Band"},
        {Ratio_High_Band,"Ratio High Band"},
        {Bypassed_Low_Band,"Bypassed Low Band"},
        {Bypassed_Mid_Band,"Bypassed Mid Band"},
        {Bypassed_High_Band,"Bypassed High Band"},
        
        {Mute_Low_Band,"Mute Low Band"},
        {Mute_Mid_Band,"Mute Mid Band"},
        {Mute_High_Band,"Mute High Band"},
        
        {Solo_Low_Band,"Solo Low Band"},
        {Solo_Mid_Band,"Solo Mid Band"},
        {Solo_High_Band,"Solo High Band"},
        
        {Gain_In,"Gain In"},
        {Gain_Out,"Gain Out"},
    };
    
    return params;
}
}

struct CompressorBand
{
    juce::AudioParameterFloat* attack { nullptr };
    juce::AudioParameterFloat* release { nullptr };
    juce::AudioParameterFloat* threshold { nullptr };
    juce::AudioParameterChoice* ratio { nullptr };
    juce::AudioParameterBool* bypassed { nullptr };
    juce::AudioParameterBool* mute { nullptr };
    juce::AudioParameterBool* solo { nullptr };
    
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        compressor.prepare(spec);
    }
    
    void updateCompressorSettings()
    {
        compressor.setAttack(attack->get());
        compressor.setRelease(release->get());
        compressor.setThreshold(threshold->get());
        compressor.setRatio( ratio->getCurrentChoiceName().getFloatValue() );
    }
    
    void process(juce::AudioBuffer<float>& buffer)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        
        context.isBypassed = bypassed->get();
        
        compressor.process(context);
    }
private:
    juce::dsp::Compressor<float> compressor;
};

//==============================================================================
/**
*/
class SimpleMBCompAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleMBCompAudioProcessor();
    ~SimpleMBCompAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    
    APVTS apvts {*this, nullptr, "Parameters", createParameterLayout() };
private:
    std::array<CompressorBand, 3> compressors;
    CompressorBand& lowBandComp = compressors[0];
    CompressorBand& midBandComp = compressors[1];
    CompressorBand& highBandComp = compressors[2];
    
    
    using Filter = juce::dsp::LinkwitzRileyFilter<float>;
    //      fc0     fc1
    Filter  LP1,    AP2,
            HP1,    LP2,
                    HP2;
    
//    Filter invAP1, invAP2;
//    juce::AudioBuffer<float> invAPBuffer;
    
    juce::AudioParameterFloat* lowMidCrossover { nullptr };
    juce::AudioParameterFloat* midHighCrossover { nullptr };
    
    std::array<juce::AudioBuffer<float>, 3> filterBuffers;
    
    juce::dsp::Gain<float> inputGain, outputGain;
    juce::AudioParameterFloat* inputGainParam { nullptr };
    juce::AudioParameterFloat* outputGainParam { nullptr };
    
    template<typename T, typename U>
    void applyGain(T& buffer, U& gain)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto ctx = juce::dsp::ProcessContextReplacing<float>(block);
        gain.process(ctx);
    }
    
    void updateState();
    
    void splitBands(const juce::AudioBuffer<float>& inputBuffer);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleMBCompAudioProcessor)
};
