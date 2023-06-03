/*
  ==============================================================================

    CompressorBandControls.cpp
    Created: 30 Oct 2021 1:03:55am
    Author:  matkatmusic

  ==============================================================================
*/

#include "CompressorBandControls.h"
#include "Utilities.h"
#include "../DSP/Params.h"
#include "LookAndFeel.h"

//==============================================================================
CompressorBandControls::CompressorBandControls(juce::AudioProcessorValueTreeState& apv) :
apvts(apv),
attackSlider(nullptr, "ms", "ATTACK"),
releaseSlider(nullptr, "ms", "RELEASE"),
thresholdSlider(nullptr, "dB", "THRESH"),
ratioSlider(nullptr, "")
{
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(thresholdSlider);
    addAndMakeVisible(ratioSlider);
    
    bypassButton.addListener(this);
    soloButton.addListener(this);
    muteButton.addListener(this);
    
    bypassButton.setName("X");
    bypassButton.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                           juce::Colours::yellow);
    bypassButton.setColour(juce::TextButton::ColourIds::buttonColourId,
                           juce::Colours::black);
    
    soloButton.setName("S");
    soloButton.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                         juce::Colours::limegreen);
    soloButton.setColour(juce::TextButton::ColourIds::buttonColourId,
                         juce::Colours::black);
    
    muteButton.setName("M");
    muteButton.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                         juce::Colours::red);
    muteButton.setColour(juce::TextButton::ColourIds::buttonColourId,
                         juce::Colours::black);
    
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(soloButton);
    addAndMakeVisible(muteButton);
    
    lowBand.setName("Low");
    lowBand.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                      ColorScheme::getSliderBorderColor());
    lowBand.setColour(juce::TextButton::ColourIds::buttonColourId,
                      juce::Colours::black);
    
    midBand.setName("Mid");
    midBand.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                      ColorScheme::getSliderBorderColor());
    midBand.setColour(juce::TextButton::ColourIds::buttonColourId,
                      juce::Colours::black);
    
    highBand.setName("High");
    highBand.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                       ColorScheme::getSliderBorderColor());
    highBand.setColour(juce::TextButton::ColourIds::buttonColourId,
                      juce::Colours::black);
    
    lowBand.setRadioGroupId(1);
    midBand.setRadioGroupId(1);
    highBand.setRadioGroupId(1);
    
    auto buttonSwitcher = [safePtr = this->safePtr]()
    {
        if( auto* c = safePtr.getComponent() )
        {
            c->updateAttachments();
        }
    };
    
    lowBand.onClick = buttonSwitcher;
    midBand.onClick = buttonSwitcher;
    highBand.onClick = buttonSwitcher;
    
    lowBand.setToggleState(true, juce::NotificationType::dontSendNotification);
    
    updateAttachments();
    updateSliderEnablements();
    updateBandSelectButtonStates();
    
    addAndMakeVisible(lowBand);
    addAndMakeVisible(midBand);
    addAndMakeVisible(highBand);
}

CompressorBandControls::~CompressorBandControls()
{
    bypassButton.removeListener(this);
    soloButton.removeListener(this);
    muteButton.removeListener(this);
}

void CompressorBandControls::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    using namespace juce;
    
    auto createBandButtonControlBox = [](std::vector<Component*> comps)
    {
        FlexBox flexBox;
        flexBox.flexDirection = FlexBox::Direction::column;
        flexBox.flexWrap = FlexBox::Wrap::noWrap;
        
        auto spacer = FlexItem().withHeight(2);
        
        for( auto* comp : comps )
        {
            flexBox.items.add(spacer);
            flexBox.items.add(FlexItem(*comp).withFlex(1.f));
        }
        
        flexBox.items.add(spacer);
        
        return flexBox;
    };
    
    auto bandButtonControlBox = createBandButtonControlBox({&bypassButton, &soloButton, &muteButton});
    auto bandSelectControlBox = createBandButtonControlBox({&lowBand, &midBand, &highBand});
    
    FlexBox flexBox;
    flexBox.flexDirection = FlexBox::Direction::row;
    flexBox.flexWrap = FlexBox::Wrap::noWrap;
    
    auto spacer = FlexItem().withWidth(4);
//    auto endCap = FlexItem().withWidth(6);
    
//    flexBox.items.add(endCap);
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(bandSelectControlBox).withWidth(50));
    flexBox.items.add(spacer);
    
    flexBox.items.add(FlexItem(attackSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(releaseSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(thresholdSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(ratioSlider).withFlex(1.f));
//    flexBox.items.add(endCap);
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(bandButtonControlBox).withWidth(30));
    
    flexBox.performLayout(bounds);
}



void CompressorBandControls::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    SimpleMBComp::drawModuleBackground(g, bounds);
}

void CompressorBandControls::buttonClicked(juce::Button *button)
{
    updateSliderEnablements();
    updateSoloMuteBypassToggleStates(*button);
    updateActiveBandFillColors(*button);
}

void CompressorBandControls::toggleAllBands(bool shouldBeBypassed)
{
    std::vector<Component*> bands { &lowBand, &midBand, &highBand};
    for( auto* band : bands )
    {
        band->setColour(juce::TextButton::ColourIds::buttonOnColourId,
                        shouldBeBypassed ? bypassButton.findColour(juce::TextButton::ColourIds::buttonOnColourId) :
                        ColorScheme::getSliderBorderColor());

        band->setColour(juce::TextButton::ColourIds::buttonColourId,
                        shouldBeBypassed ? bypassButton.findColour(juce::TextButton::ColourIds::buttonOnColourId) :
                        juce::Colours::black);
        band->repaint();
    }
}

void CompressorBandControls::updateActiveBandFillColors(juce::Button &clickedButton)
{
    jassert(activeBand != nullptr);
    DBG( "active band: " << activeBand->getName() );
    
    if( clickedButton.getToggleState() == false )
    {
        resetActiveBandColors();
    }
    else
    {
        refreshBandButtonColors(*activeBand, clickedButton);
    }
}

void CompressorBandControls::refreshBandButtonColors(juce::Button &band, juce::Button &colorSource)
{
    band.setColour(juce::TextButton::ColourIds::buttonOnColourId,
                   colorSource.findColour(juce::TextButton::ColourIds::buttonOnColourId));
    band.setColour(juce::TextButton::ColourIds::buttonColourId,
                   colorSource.findColour(juce::TextButton::ColourIds::buttonOnColourId));
    band.repaint();
}


void CompressorBandControls::resetActiveBandColors()
{
    activeBand->setColour(juce::TextButton::ColourIds::buttonOnColourId,
                          ColorScheme::getSliderBorderColor());
    activeBand->setColour(juce::TextButton::ColourIds::buttonColourId,
                          juce::Colours::black);
    activeBand->repaint();
}

void CompressorBandControls::updateBandSelectButtonStates()
{
    using namespace Params;
    
    std::vector<std::array<Names, 3>> paramsToCheck
    {
        {Names::Solo_Low_Band, Names::Mute_Low_Band, Names::Bypassed_Low_Band},
        {Names::Solo_Mid_Band, Names::Mute_Mid_Band, Names::Bypassed_Mid_Band},
        {Names::Solo_High_Band, Names::Mute_High_Band, Names::Bypassed_High_Band}
    };
    
    const auto& params = GetParams();
    auto paramHelper = [&params, this](const auto& name)
    {
        return dynamic_cast<juce::AudioParameterBool*>(&SimpleMBComp::getParam(apvts, params, name));
    };
    
    for( size_t i = 0; i < paramsToCheck.size(); ++i )
    {
        auto& list = paramsToCheck[i];
        
        auto* bandButton = (i == 0) ? &lowBand :
                           (i == 1) ? &midBand :
                                      &highBand;
        
        if( auto* solo = paramHelper(list[0]);
           solo->get() )
        {
            refreshBandButtonColors(*bandButton, soloButton);
        }
        else if( auto* mute = paramHelper(list[1]);
                mute->get() )
        {
            refreshBandButtonColors(*bandButton, muteButton);
        }
        else if( auto* byp = paramHelper(list[2]);
                byp->get() )
        {
            refreshBandButtonColors(*bandButton, bypassButton);
        }
        
    }
}

void CompressorBandControls::updateSliderEnablements()
{
    auto disabled = muteButton.getToggleState() || bypassButton.getToggleState();
    attackSlider.setEnabled( !disabled );
    releaseSlider.setEnabled( !disabled );
    thresholdSlider.setEnabled( !disabled );
    ratioSlider.setEnabled( !disabled );
}

void CompressorBandControls::updateSoloMuteBypassToggleStates(juce::Button &clickedButton)
{
    if( &clickedButton == &soloButton && soloButton.getToggleState() )
    {
        bypassButton.setToggleState(false, juce::NotificationType::sendNotification);
        muteButton.setToggleState(false, juce::NotificationType::sendNotification);
    }
    else if( &clickedButton == &muteButton && muteButton.getToggleState() )
    {
        bypassButton.setToggleState(false, juce::NotificationType::sendNotification);
        soloButton.setToggleState(false, juce::NotificationType::sendNotification);
    }
    else if( &clickedButton == &bypassButton && bypassButton.getToggleState() )
    {
        muteButton.setToggleState(false, juce::NotificationType::sendNotification);
        soloButton.setToggleState(false, juce::NotificationType::sendNotification);
    }
}

void CompressorBandControls::updateAttachments()
{
    enum BandType
    {
        Low,
        Mid,
        High
    };
    
    BandType bandType = [this]()
    {
        if( lowBand.getToggleState() )
            return BandType::Low;
        if( midBand.getToggleState() )
            return BandType::Mid;
        
        return BandType::High;
    }();
    
    using namespace Params;
    std::vector<Names> names;
    
    switch (bandType)
    {
        case Low:
        {
            names = std::vector<Names>
            {
                Names::Attack_Low_Band,
                Names::Release_Low_Band,
                Names::Threshold_Low_Band,
                Names::Ratio_Low_Band,
                Names::Mute_Low_Band,
                Names::Solo_Low_Band,
                Names::Bypassed_Low_Band
            };
            
            activeBand = &lowBand;
            break;
        }
        case Mid:
        {
            names = std::vector<Names>
            {
                Names::Attack_Mid_Band,
                Names::Release_Mid_Band,
                Names::Threshold_Mid_Band,
                Names::Ratio_Mid_Band,
                Names::Mute_Mid_Band,
                Names::Solo_Mid_Band,
                Names::Bypassed_Mid_Band
            };
            
            activeBand = &midBand;
            break;
        }
        case High:
        {
            names = std::vector<Names>
            {
                Names::Attack_High_Band,
                Names::Release_High_Band,
                Names::Threshold_High_Band,
                Names::Ratio_High_Band,
                Names::Mute_High_Band,
                Names::Solo_High_Band,
                Names::Bypassed_High_Band
            };
            
            activeBand = &highBand;
            break;
        }
    }
    
    enum Pos
    {
        Attack,
        Release,
        Threshold,
        Ratio,
        Mute,
        Solo,
        Bypass
    };
    
    const auto& params = GetParams();
    
    auto getParamHelper = [&params, &apvts = this->apvts, &names](const auto& pos) -> auto&
    {
        return SimpleMBComp::getParam(apvts, params, names.at(pos));
    };
    
    attackSliderAttachment.reset();
    releaseSliderAttachment.reset();
    thresholdSliderAttachment.reset();
    ratioSliderAttachment.reset();
    bypassButtonAttachment.reset();
    soloButtonAttachment.reset();
    muteButtonAttachment.reset();
    
    auto& attackParam = getParamHelper(Pos::Attack);
    SimpleMBComp::addLabelPairs(attackSlider.labels, attackParam, "ms");
    attackSlider.changeParam(&attackParam);
    
    auto& releaseParam = getParamHelper(Pos::Release);
    SimpleMBComp::addLabelPairs(releaseSlider.labels, releaseParam, "ms");
    releaseSlider.changeParam(&releaseParam);
    
    auto& threshParam = getParamHelper(Pos::Threshold);
    SimpleMBComp::addLabelPairs(thresholdSlider.labels, threshParam, "dB");
    thresholdSlider.changeParam(&threshParam);
    
    auto& ratioParamRap = getParamHelper(Pos::Ratio);
    ratioSlider.labels.clear();
    ratioSlider.labels.add({0.f, "1:1"});
    auto ratioParam = dynamic_cast<juce::AudioParameterChoice*>(&ratioParamRap);
    ratioSlider.labels.add({1.0f,
        juce::String(ratioParam->choices.getReference(ratioParam->choices.size() - 1).getIntValue()) + ":1" });
    ratioSlider.changeParam(ratioParam);
    
    auto makeAttachmentHelper = [&params, &apvts = this->apvts](auto& attachment,
                                                                const auto& name,
                                                                auto& slider)
    {
        SimpleMBComp::makeAttachment(attachment, apvts, params, name, slider);
    };
    
    makeAttachmentHelper(attackSliderAttachment, names[Pos::Attack], attackSlider);
    makeAttachmentHelper(releaseSliderAttachment, names[Pos::Release], releaseSlider);
    makeAttachmentHelper(thresholdSliderAttachment, names[Pos::Threshold], thresholdSlider);
    makeAttachmentHelper(ratioSliderAttachment, names[Pos::Ratio], ratioSlider);
    makeAttachmentHelper(bypassButtonAttachment, names[Pos::Bypass], bypassButton);
    makeAttachmentHelper(soloButtonAttachment, names[Pos::Solo], soloButton);
    makeAttachmentHelper(muteButtonAttachment, names[Pos::Mute], muteButton);
}
