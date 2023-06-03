/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/Params.h"

//==============================================================================
SimpleMBCompAudioProcessor::SimpleMBCompAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    using namespace Params;
    const auto& params = GetParams();
    
    auto floatHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };
    
    floatHelper(lowBandComp.attack, Names::Attack_Low_Band);
    floatHelper(lowBandComp.release, Names::Release_Low_Band);
    floatHelper(lowBandComp.threshold, Names::Threshold_Low_Band);
    
    floatHelper(midBandComp.attack,     Names::Attack_Mid_Band);
    floatHelper(midBandComp.release,    Names::Release_Mid_Band);
    floatHelper(midBandComp.threshold,  Names::Threshold_Mid_Band);
    
    floatHelper(highBandComp.attack,    Names::Attack_High_Band);
    floatHelper(highBandComp.release,   Names::Release_High_Band);
    floatHelper(highBandComp.threshold, Names::Threshold_High_Band);
    
    lowThresholdParam = lowBandComp.threshold;
    midThresholdParam = midBandComp.threshold;
    highThresholdParam = highBandComp.threshold;
    
    auto choiceHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };
    
    choiceHelper(lowBandComp.ratio, Names::Ratio_Low_Band);
    choiceHelper(midBandComp.ratio, Names::Ratio_Mid_Band);
    choiceHelper(highBandComp.ratio, Names::Ratio_High_Band);
    
    auto boolHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };
    
    boolHelper(lowBandComp.bypassed, Names::Bypassed_Low_Band);
    boolHelper(midBandComp.bypassed, Names::Bypassed_Mid_Band);
    boolHelper(highBandComp.bypassed, Names::Bypassed_High_Band);
    
    boolHelper(lowBandComp.mute, Names::Mute_Low_Band);
    boolHelper(midBandComp.mute, Names::Mute_Mid_Band);
    boolHelper(highBandComp.mute, Names::Mute_High_Band);
    
    boolHelper(lowBandComp.solo, Names::Solo_Low_Band);
    boolHelper(midBandComp.solo, Names::Solo_Mid_Band);
    boolHelper(highBandComp.solo, Names::Solo_High_Band);
    
    floatHelper(lowMidCrossover, Names::Low_Mid_Crossover_Freq);
    floatHelper(midHighCrossover, Names::Mid_High_Crossover_Freq);
    
    floatHelper(inputGainParam, Names::Gain_In);
    floatHelper(outputGainParam, Names::Gain_Out);
    
    LP1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    HP1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    
    AP2.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    
    LP2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    HP2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    
//    invAP1.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
//    invAP2.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
}

SimpleMBCompAudioProcessor::~SimpleMBCompAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleMBCompAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleMBCompAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleMBCompAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleMBCompAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleMBCompAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleMBCompAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleMBCompAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleMBCompAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String SimpleMBCompAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void SimpleMBCompAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void SimpleMBCompAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    spec.sampleRate = sampleRate;
    
    for( auto& comp : compressors )
        comp.prepare(spec);
    
    LP1.prepare(spec);
    HP1.prepare(spec);
    
    AP2.prepare(spec);
    
    LP2.prepare(spec);
    HP2.prepare(spec);
    
//    invAP1.prepare(spec);
//    invAP2.prepare(spec);
//
//    invAPBuffer.setSize(spec.numChannels, samplesPerBlock);
    
    inputGain.prepare(spec);
    outputGain.prepare(spec);
    
    inputGain.setRampDurationSeconds(0.05); //50 ms
    outputGain.setRampDurationSeconds(0.05);
    
    for( auto& buffer : filterBuffers )
    {
        buffer.setSize(static_cast<int>(spec.numChannels), samplesPerBlock);
    }
    
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
    
#if USE_TEST_OSC
    osc.initialise([](float x){ return std::sin(x); });
    osc.prepare(spec);
    
    osc.setFrequency(getSampleRate() / ((2 << FFTOrder::order2048) - 1) * 50);
    
    gain.prepare(spec);
    gain.setGainDecibels(-12.f);
#endif
}

void SimpleMBCompAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleMBCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleMBCompAudioProcessor::updateState()
{
    for( auto& compressor : compressors )
        compressor.updateCompressorSettings();
    
    auto lowMidCutoffFreq = lowMidCrossover->get();
    LP1.setCutoffFrequency(lowMidCutoffFreq);
    HP1.setCutoffFrequency(lowMidCutoffFreq);
    
    auto midHighCutoffFreq = midHighCrossover->get();
    AP2.setCutoffFrequency(midHighCutoffFreq);
    LP2.setCutoffFrequency(midHighCutoffFreq);
    HP2.setCutoffFrequency(midHighCutoffFreq);
    
    inputGain.setGainDecibels(inputGainParam->get());
    outputGain.setGainDecibels(outputGainParam->get());
}

void SimpleMBCompAudioProcessor::splitBands(const juce::AudioBuffer<float> &inputBuffer)
{
    for( auto& fb : filterBuffers )
    {
        fb = inputBuffer;
    }
    
    auto fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
    auto fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);
    auto fb2Block = juce::dsp::AudioBlock<float>(filterBuffers[2]);
    
    auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
    auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);
    auto fb2Ctx = juce::dsp::ProcessContextReplacing<float>(fb2Block);
    
    LP1.process(fb0Ctx);
    AP2.process(fb0Ctx);
    
    HP1.process(fb1Ctx);
    filterBuffers[2] = filterBuffers[1];
    LP2.process(fb1Ctx);
    
    HP2.process(fb2Ctx);
}

void SimpleMBCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateState();
    
#if USE_TEST_OSC
    {
        buffer.clear();
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto ctx = juce::dsp::ProcessContextReplacing<float>(block);
        osc.process(ctx);
        
        gain.setGainDecibels(JUCE_LIVE_CONSTANT(-12));
        gain.process(ctx);
    }
#endif
    
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
    
    applyGain(buffer, inputGain);

    splitBands(buffer);
    
    for( size_t i = 0; i < filterBuffers.size(); ++i )
    {
        compressors[i].process(filterBuffers[i]);
    }
    
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    buffer.clear();
    
    auto addFilterBand = [nc = numChannels, ns = numSamples](auto& inputBuffer, const auto& source)
    {
        for( auto i = 0; i < nc; ++i )
        {
            inputBuffer.addFrom(i, 0, source, i, 0, ns);
        }
    };
    
    auto bandsAreSoloed = false;
    for( auto& comp : compressors)
    {
        if( comp.solo->get() )
        {
            bandsAreSoloed = true;
            break;
        }
    }
    
    if( bandsAreSoloed )
    {
        for( size_t i = 0; i < compressors.size(); ++i )
        {
            auto& comp = compressors[i];
            if( comp.solo->get() )
            {
                addFilterBand(buffer, filterBuffers[i]);
            }
        }
    }
    else
    {
        for( size_t i = 0; i < compressors.size(); ++i )
        {
            auto& comp = compressors[i];
            if( ! comp.mute->get() )
            {
                addFilterBand(buffer, filterBuffers[i]);
            }
        }
    }
    
    applyGain(buffer, outputGain);
    
}

//==============================================================================
bool SimpleMBCompAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleMBCompAudioProcessor::createEditor()
{
    return new SimpleMBCompAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleMBCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleMBCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data,
                                              static_cast<size_t>(sizeInBytes));
    if( tree.isValid() )
    {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleMBCompAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    using namespace Params;
    const auto& params = GetParams();
    
    auto gainRange = NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f);
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Gain_In),
                                                     params.at(Names::Gain_In),
                                                     gainRange,
                                                     0));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Gain_Out),
                                                     params.at(Names::Gain_Out),
                                                     gainRange,
                                                     0));
    
    auto thresholdRange = NormalisableRange<float>(SimpleMBComp::MIN_THRESHOLD, SimpleMBComp::MAX_DB, 1, 1);
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Threshold_Low_Band),
                                                     params.at(Names::Threshold_Low_Band),
                                                     thresholdRange,
                                                     0));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Threshold_Mid_Band),
                                                     params.at(Names::Threshold_Mid_Band),
                                                     thresholdRange,
                                                     0));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Threshold_High_Band),
                                                     params.at(Names::Threshold_High_Band),
                                                     thresholdRange,
                                                     0));
    
    auto attackReleaseRange = NormalisableRange<float>(5, 500, 1, 1);
    
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Attack_Low_Band),
                                                     params.at(Names::Attack_Low_Band),
                                                     attackReleaseRange,
                                                     50));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Attack_Mid_Band),
                                                     params.at(Names::Attack_Mid_Band),
                                                     attackReleaseRange,
                                                     50));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Attack_High_Band),
                                                     params.at(Names::Attack_High_Band),
                                                     attackReleaseRange,
                                                     50));
    
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Release_Low_Band),
                                                     params.at(Names::Release_Low_Band),
                                                     attackReleaseRange,
                                                     250));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Release_Mid_Band),
                                                     params.at(Names::Release_Mid_Band),
                                                     attackReleaseRange,
                                                     250));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Release_High_Band),
                                                     params.at(Names::Release_High_Band),
                                                     attackReleaseRange,
                                                     250));
    
    auto choices = std::vector<double>{ 1, 1.5, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20, 50, 100 };
    juce::StringArray sa;
    for( auto choice : choices )
    {
        sa.add( juce::String(choice, 1) );
    }
    
    layout.add(std::make_unique<AudioParameterChoice>(params.at(Names::Ratio_Low_Band),
                                                      params.at(Names::Ratio_Low_Band),
                                                      sa,
                                                      3));
    layout.add(std::make_unique<AudioParameterChoice>(params.at(Names::Ratio_Mid_Band),
                                                      params.at(Names::Ratio_Mid_Band),
                                                      sa,
                                                      3));
    layout.add(std::make_unique<AudioParameterChoice>(params.at(Names::Ratio_High_Band),
                                                      params.at(Names::Ratio_High_Band),
                                                      sa,
                                                      3));
    
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Bypassed_Low_Band),
                                                    params.at(Names::Bypassed_Low_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Bypassed_Mid_Band),
                                                    params.at(Names::Bypassed_Mid_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Bypassed_High_Band),
                                                    params.at(Names::Bypassed_High_Band),
                                                    false));
    
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Mute_Low_Band),
                                                    params.at(Names::Mute_Low_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Mute_Mid_Band),
                                                    params.at(Names::Mute_Mid_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Mute_High_Band),
                                                    params.at(Names::Mute_High_Band),
                                                    false));
    
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Solo_Low_Band),
                                                    params.at(Names::Solo_Low_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Solo_Mid_Band),
                                                    params.at(Names::Solo_Mid_Band),
                                                    false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Solo_High_Band),
                                                    params.at(Names::Solo_High_Band),
                                                    false));
    
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Low_Mid_Crossover_Freq),
                                                     params.at(Names::Low_Mid_Crossover_Freq),
                                                     NormalisableRange<float>(SimpleMBComp::MIN_FREQUENCY, 999, 1, 1),
                                                     400));
    
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Mid_High_Crossover_Freq),
                                                     params.at(Names::Mid_High_Crossover_Freq),
                                                     NormalisableRange<float>(1000, SimpleMBComp::MAX_FREQUENCY, 1, 1),
                                                     2000));
    
    return layout;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleMBCompAudioProcessor();
}
