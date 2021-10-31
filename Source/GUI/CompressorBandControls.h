/*
  ==============================================================================

    CompressorBandControls.h
    Created: 30 Oct 2021 1:03:55am
    Author:  matkatmusic

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "RotarySliderWithLabels.h"

struct CompressorBandControls : juce::Component, juce::Button::Listener
{
    CompressorBandControls(juce::AudioProcessorValueTreeState& apvts);
    ~CompressorBandControls() override;
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    void buttonClicked(juce::Button* button) override;
    
    void toggleAllBands(bool shouldBeBypassed);
private:
    juce::AudioProcessorValueTreeState& apvts;
    
    RotarySliderWithLabels attackSlider, releaseSlider, thresholdSlider/*, ratioSlider*/;
    RatioSlider ratioSlider;
    
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> attackSliderAttachment,
                                releaseSliderAttachment,
                                thresholdSliderAttachment,
                                ratioSliderAttachment;
    
    juce::ToggleButton bypassButton, soloButton, muteButton, lowBand, midBand, highBand;
    
    using BtnAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttachment>  bypassButtonAttachment,
                                    soloButtonAttachment,
                                    muteButtonAttachment;
    
    juce::Component::SafePointer<CompressorBandControls> safePtr {this};
    
    juce::ToggleButton* activeBand = &lowBand;
    
    void updateAttachments();
    void updateSliderEnablements();
    void updateSoloMuteBypassToggleStates(juce::Button& clickedButton);
    void updateActiveBandFillColors(juce::Button& clickedButton);
    
    void resetActiveBandColors();
    static void refreshBandButtonColors(juce::Button& band,
                                        juce::Button& colorSource);
    
    void updateBandSelectButtonStates();
    
};
