#pragma once
#include "PluginProcessor.h"
#include <functional>

namespace ui
{
const juce::Colour bg      { 0xff06070c };
const juce::Colour panel   { 0xff0f121a };
const juce::Colour edge    { 0xff232a3b };
const juce::Colour text    { 0xffd7deec };
const juce::Colour dim     { 0xff6b7690 };
const juce::Colour cyan    { 0xff3df2ff };
const juce::Colour violet  { 0xff8a6bff };
const juce::Colour magenta { 0xffff5fd0 };
const juce::Colour amber   { 0xffffb35c };
const juce::Colour bandColours[5] { juce::Colour (0xff4d7cff), juce::Colour (0xff3df2ff), juce::Colour (0xff52f2a8),
                                    juce::Colour (0xffffd35c), juce::Colour (0xffff6f8a) };
}

//==============================================================================
class SolarisLnF : public juce::LookAndFeel_V4
{
public:
    SolarisLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
          const juce::String& suffix, int decimals = 1);
    void resized() override;
    juce::Slider slider;
protected:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// the hero Drive knob: an aura behind it grows with the drive amount and pulses with the output level
class AuraKnob : public Knob, private juce::Timer
{
public:
    AuraKnob (SolarisProcessor&, const juce::String& id, const juce::String& name, const juce::String& suffix);
    ~AuraKnob() override { stopTimer(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    SolarisProcessor& proc;
    float pulse = 0.f;
};

class VFader : public juce::Component
{
public:
    VFader (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
            const juce::String& suffix, int decimals, juce::Colour c);
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class BandStrip : public juce::Component, private juce::Timer
{
public:
    BandStrip (SolarisProcessor&, int band);
    ~BandStrip() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    SolarisProcessor& proc;
    int index;
    float level = 0.f;
    juce::Rectangle<float> meterRect;
    VFader drive, lvl;
};

class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (SolarisProcessor&, bool isInput);
    ~LevelMeter() override { stopTimer(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    SolarisProcessor& proc;
    bool input;
    float lv[2] = { 0, 0 }, hold[2] = { 0, 0 };
};

class ModeSwitch : public juce::Component, private juce::Timer
{
public:
    std::function<void (bool)> onChange;
    ~ModeSwitch() override { stopTimer(); }
    void setMulti (bool m, bool animate);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    float pos = 0.f, target = 0.f;
};

//==============================================================================
// Real-time in/out spectrum with the 5 saturation ranges.  Hover a range -> display freezes and the
// strongest peaks of that range are listed; click to resume live monitoring.
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumView (SolarisProcessor&);
    ~SpectrumView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder, numBins = fftSize / 2;
    using Spec = std::array<float, (size_t) numBins>;
    using Win  = std::array<float, (size_t) fftSize>;

private:
    struct Peak { float f, db; };

    void timerCallback() override;
    void feed (ScopeFifo&, Win&);
    void analyse (const Win&, Spec&);
    juce::Path makePath (const Spec&, float step) const;
    float xForFreq (float f) const;
    float freqForX (float x) const;
    float yForDb (float db) const;
    float dbAt (const Spec&, float f0, float f1) const;
    std::vector<Peak> findPeaks (const Spec&, float fLo, float fHi, int maxN) const;
    int rangeForX (float x) const;
    static juce::String fmtFreq (float f);

    SolarisProcessor& proc;
    std::atomic<float> *pMulti, *pDrive, *pBd[5];
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    Win winOut {}, winIn {};
    std::array<float, (size_t) fftSize * 2> fftData {};
    Spec smoothOut {}, smoothIn {}, frozenOut {}, frozenIn {};
    bool frozen = false, suppress = false;
    int hovered = -1;
    juce::Rectangle<float> plot;
};

//==============================================================================
class SolarisEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SolarisEditor (SolarisProcessor&);
    ~SolarisEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setMulti (bool);
    void updateMode();

    SolarisProcessor& proc;
    SolarisLnF lnf;
    SpectrumView spectrum;
    LevelMeter inMeter, outMeter;
    ModeSwitch modeSwitch;

    AuraKnob driveKnob;
    Knob inKnob, outKnob, warmKnob, toneKnob, mixKnob, clarKnob, airKnob;
    juce::ToggleButton clarBtn { "CLARITY" }, airBtn { "AIR" }, clipBtn { "SOFT CLIP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clarAtt, airAtt, clipAtt;
    std::unique_ptr<BandStrip> strips[5];
    bool multiShown = false;
    juce::Rectangle<int> leftPanel, centrePanel, rightPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolarisEditor)
};
