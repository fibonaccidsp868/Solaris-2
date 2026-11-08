#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "DSP.h"

// lock-free mono tap for the GUI analyser
struct ScopeFifo
{
    juce::AbstractFifo fifo { 16384 };
    std::vector<float> data = std::vector<float> (16384, 0.0f);

    void push (const float* src, int n)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        if (n1 > 0) std::copy (src, src + n1, data.begin() + s1);
        if (n2 > 0) std::copy (src + n1, src + n1 + n2, data.begin() + s2);
        fifo.finishedWrite (n1 + n2);
    }
    int pull (float* dst, int maxN)
    {
        const int n = juce::jmin (maxN, fifo.getNumReady());
        if (n <= 0) return 0;
        int s1, n1, s2, n2;
        fifo.prepareToRead (n, s1, n1, s2, n2);
        if (n1 > 0) std::copy (data.begin() + s1, data.begin() + s1 + n1, dst);
        if (n2 > 0) std::copy (data.begin() + s2, data.begin() + s2 + n2, dst + n1);
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }
};

class SolarisProcessor : public juce::AudioProcessor
{
public:
    SolarisProcessor();
    ~SolarisProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Solaris 2"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    double getSampleRateSafe() const { return sampleRateAtomic.load(); }

    // ---- shared with the GUI ----
    juce::AudioProcessorValueTreeState apvts;
    ScopeFifo inScope, outScope;
    std::atomic<float> inLevel[2], outLevel[2], bandMeter[5];

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float> *pIn, *pOut, *pDrive, *pMix, *pWarm, *pTone, *pClar, *pClarOn, *pAir, *pAirOn, *pClip, *pMulti;
    std::atomic<float> *pBandDrive[5], *pBandLevel[5];

    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuf;
    std::vector<float> monoIn, monoOut;
    int numCh = 2;
    double fs = 44100.0, osFs = 176400.0;
    double atkC = 0, heatC = 0, dcR = 0.999, relC[5] = {}, relClar = 0, relAir = 0;

    using Lin = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    using Mul = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;
    Mul smIn, smOut, smLevel[5];
    Lin smClar, smAir, smClip, smWarm, smMix /* base rate */, smDrive[5];

    solaris::CompTable compTab;
    solaris::NormSat bandSat[5][2], clarSat[2], airSat[2];
    solaris::LR4 xo[4];
    solaris::Biquad ap[6];
    solaris::Biquad toneLo, toneHi, clarHp, clarLp, airHp, airShelf;
    double dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 };
    float lastToneDb = 1000.f, airBlock = 0.f;
    bool prevClarActive = false, prevAirActive = false;

    std::atomic<double> sampleRateAtomic { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolarisProcessor)
};
