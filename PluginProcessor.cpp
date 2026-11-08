#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace solaris;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SolarisProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<P> (juce::ParameterID { "in", 1 },  "Input",  R (-24.f, 24.f, 0.1f), 0.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "out", 1 }, "Output", R (-24.f, 24.f, 0.1f), 0.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "drive", 1 }, "Drive", R (0.f, 100.f, 0.1f), 40.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",    R (0.f, 100.f, 0.1f), 100.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "warmth", 1 }, "Warmth", R (0.f, 100.f, 0.1f), 30.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "tone", 1 }, "Tone", R (-100.f, 100.f, 0.1f), 0.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "clarity", 1 }, "Clarity", R (0.f, 100.f, 0.1f), 50.f));
    p.push_back (std::make_unique<B> (juce::ParameterID { "clarityOn", 1 }, "Clarity On", false));
    p.push_back (std::make_unique<P> (juce::ParameterID { "air", 1 }, "Air", R (0.f, 100.f, 0.1f), 50.f));
    p.push_back (std::make_unique<B> (juce::ParameterID { "airOn", 1 }, "Air On", false));
    p.push_back (std::make_unique<B> (juce::ParameterID { "softclip", 1 }, "Soft Clip", false));
    p.push_back (std::make_unique<B> (juce::ParameterID { "multi", 1 }, "Multiband", false));

    for (int b = 0; b < kNumBands; ++b)
    {
        p.push_back (std::make_unique<P> (juce::ParameterID { "bd" + juce::String (b + 1), 1 },
                                          juce::String (kBandNames[b]) + " Drive", R (0.f, 100.f, 0.1f), 40.f));
        p.push_back (std::make_unique<P> (juce::ParameterID { "bl" + juce::String (b + 1), 1 },
                                          juce::String (kBandNames[b]) + " Level", R (-12.f, 12.f, 0.1f), 0.f));
    }
    return { p.begin(), p.end() };
}

SolarisProcessor::SolarisProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto ptr = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    pIn = ptr ("in"); pOut = ptr ("out"); pDrive = ptr ("drive"); pMix = ptr ("mix");
    pWarm = ptr ("warmth"); pTone = ptr ("tone");
    pClar = ptr ("clarity"); pClarOn = ptr ("clarityOn"); pAir = ptr ("air"); pAirOn = ptr ("airOn");
    pClip = ptr ("softclip"); pMulti = ptr ("multi");
    for (int b = 0; b < kNumBands; ++b)
    {
        pBandDrive[b] = ptr ("bd" + juce::String (b + 1));
        pBandLevel[b] = ptr ("bl" + juce::String (b + 1));
        bandMeter[b].store (0.f);
    }
    for (int c = 0; c < 2; ++c) { inLevel[c].store (0.f); outLevel[c].store (0.f); }
}

bool SolarisProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo())
           && o == l.getMainInputChannelSet();
}

//==============================================================================
void SolarisProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    sampleRateAtomic.store (fs);
    osFs = fs * 4.0;
    numCh = juce::jmax (1, juce::jmin (2, getTotalNumInputChannels()));

    os = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) samplesPerBlock);
    os->reset();

    const int latency = (int) std::lround (os->getLatencyInSamples());
    setLatencySamples (latency);

    juce::dsp::ProcessSpec spec { fs, (juce::uint32) samplesPerBlock, (juce::uint32) numCh };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuf.setSize (numCh, samplesPerBlock);
    monoIn.assign ((size_t) samplesPerBlock + 8, 0.0f);
    monoOut.assign ((size_t) samplesPerBlock + 8, 0.0f);

    compTab.build();

    // phase-compensated 5-band split
    const double q = 0.70710678118654752;
    for (int i = 0; i < 4; ++i) { xo[i].set (osFs, kCrossover[i]); xo[i].reset(); }
    ap[0].allPass (osFs, kCrossover[1], q); ap[1].allPass (osFs, kCrossover[2], q); ap[2].allPass (osFs, kCrossover[3], q);
    ap[3].allPass (osFs, kCrossover[2], q); ap[4].allPass (osFs, kCrossover[3], q);
    ap[5].allPass (osFs, kCrossover[3], q);
    for (auto& a : ap) a.reset();

    // Clarity band (2.4 - 7.5 kHz) and Air band (>6.5 kHz) pick-ups for the exciters
    clarHp.highPass (osFs, 2400.0, q); clarLp.lowPass (osFs, 7500.0, q);
    airHp.highPass (osFs, 6500.0, q);
    airShelf.highShelf (osFs, 10000.0, 0.4, 0.0);
    toneLo.lowShelf (osFs, 900.0, 0.5, 0.0); toneHi.highShelf (osFs, 900.0, 0.5, 0.0);
    clarHp.reset(); clarLp.reset(); airHp.reset(); airShelf.reset(); toneLo.reset(); toneHi.reset();
    lastToneDb = 0.f; airBlock = 0.f;
    prevClarActive = prevAirActive = false;

    atkC = 1.0 - std::exp (-1.0 / (0.0008 * osFs));
    for (int b = 0; b < 5; ++b) relC[b] = 1.0 - std::exp (-1.0 / (kBandRelease[b] * osFs));
    relClar = 1.0 - std::exp (-1.0 / (0.030 * osFs));
    relAir  = 1.0 - std::exp (-1.0 / (0.020 * osFs));
    heatC = 1.0 - std::exp (-1.0 / (0.35 * osFs));
    dcR = 1.0 - (6.283185307 * 10.0 / osFs);

    for (auto& row : bandSat) for (auto& s : row) s.reset();
    for (int c = 0; c < 2; ++c) { clarSat[c].reset(); airSat[c].reset(); dcX[c] = dcY[c] = 0.0; }

    auto initMul = [&] (Mul& s, float v, double rate) { s.reset (rate, 0.025); s.setCurrentAndTargetValue (v); };
    auto initLin = [&] (Lin& s, float v, double rate) { s.reset (rate, 0.025); s.setCurrentAndTargetValue (v); };
    initMul (smIn, 1.f, osFs); initMul (smOut, 1.f, osFs);
    initLin (smClar, 0.f, osFs); initLin (smAir, 0.f, osFs); initLin (smClip, 0.f, osFs);
    initLin (smWarm, pWarm->load() * 0.01f, osFs);
    initLin (smMix, 1.f, fs);
    for (auto& s : smDrive) initLin (s, 0.f, osFs);
    for (auto& s : smLevel) initMul (s, 1.f, osFs);
}

//==============================================================================
void SolarisProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (! os || n <= 0 || buffer.getNumChannels() < numCh) return;
    for (int c = numCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (dryBuf.getNumSamples() < n) dryBuf.setSize (numCh, n, false, false, true);
    if ((int) monoIn.size() < n)  { monoIn.resize ((size_t) n);  monoOut.resize ((size_t) n); }

    const float meterDecay = std::exp (-(float) n / (float) (fs * 0.30));

    // ---- input metering, input scope tap, latency-matched dry ----
    for (int c = 0; c < numCh; ++c)
    {
        const float* src = buffer.getReadPointer (c);
        float* d = dryBuf.getWritePointer (c);
        float pk = 0.f;
        for (int i = 0; i < n; ++i)
        {
            pk = juce::jmax (pk, std::abs (src[i]));
            dryDelay.pushSample (c, src[i]);
            d[i] = dryDelay.popSample (c);
        }
        inLevel[c].store (juce::jmax (pk, inLevel[c].load() * meterDecay));
    }
    if (numCh == 1) inLevel[1].store (inLevel[0].load());
    for (int i = 0; i < n; ++i)
    {
        float m = 0.f;
        for (int c = 0; c < numCh; ++c) m += buffer.getSample (c, i);
        monoIn[(size_t) i] = m / (float) numCh;
    }
    inScope.push (monoIn.data(), n);

    // ---- parameter targets ----
    const bool multi = pMulti->load() > 0.5f;
    const float master = pDrive->load() * 0.01f;
    for (int b = 0; b < kNumBands; ++b)
    {
        smDrive[b].setTargetValue (multi ? pBandDrive[b]->load() * 0.01f : juce::jmin (1.f, master * kVoiceDrive[b]));
        smLevel[b].setTargetValue (multi ? juce::Decibels::decibelsToGain (pBandLevel[b]->load()) : 1.f);
    }
    smIn.setTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    smOut.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    smWarm.setTargetValue (pWarm->load() * 0.01f);
    const float clarTarget = pClarOn->load() > 0.5f ? pClar->load() * 0.01f : 0.f;
    const float airTarget  = pAirOn->load()  > 0.5f ? pAir->load()  * 0.01f : 0.f;
    const float clipTarget = pClip->load() > 0.5f ? 1.f : 0.f;
    smClar.setTargetValue (clarTarget); smAir.setTargetValue (airTarget); smClip.setTargetValue (clipTarget);
    smMix.setTargetValue (pMix->load() * 0.01f);

    // tone tilt (+-4.5 dB shelves around 900 Hz) and air shelf coefficients (per block)
    const float toneDb = pTone->load() * 0.045f;
    if (std::abs (toneDb - lastToneDb) > 1.0e-4f)
    {
        toneLo.lowShelf (osFs, 900.0, 0.5, -toneDb);
        toneHi.highShelf (osFs, 900.0, 0.5, toneDb);
        lastToneDb = toneDb;
    }
    airBlock += (airTarget - airBlock) * 0.35f;
    if (airBlock > 1.0e-3f || airTarget > 0.f) airShelf.highShelf (osFs, 10000.0, 0.4, 4.5 * (double) airBlock);

    // ---- oversampled processing ----
    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh).getSubBlock (0, (size_t) n);
    auto up = os->processSamplesUp (sub);
    const int osN = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

    float bandPk[5] = { 0, 0, 0, 0, 0 };
    const bool clarOnNow = clarTarget > 0.f, airOnNow = airTarget > 0.f;

    for (int i = 0; i < osN; ++i)
    {
        const float inG = smIn.getNextValue(), outG = smOut.getNextValue();
        const float cl = smClar.getNextValue(), ai = smAir.getNextValue(), sc = smClip.getNextValue();
        const float warm = smWarm.getNextValue();

        double D[5], W[5], Bi[5];
        float lv[5];
        for (int s = 0; s < 5; ++s)
        {
            const double d = (double) smDrive[s].getNextValue();
            D[s]  = 0.35 + 4.0 * d * std::sqrt (d);        // saturation depth
            W[s]  = std::min (1.0, d * (2.0 - d));         // wet fraction of the harmonic part
            Bi[s] = 0.28 * (double) warm * (double) kVoiceWarm[s];
            lv[s] = smLevel[s].getNextValue();
        }

        const bool clarActive = cl > 1.0e-5f || clarOnNow;
        const bool airActive  = ai > 1.0e-5f || airOnNow || airBlock > 1.0e-3f;
        if (clarActive && ! prevClarActive) { clarHp.reset(); clarLp.reset(); clarSat[0].reset(); clarSat[1].reset(); }
        if (airActive && ! prevAirActive)   { airHp.reset(); airShelf.reset(); airSat[0].reset(); airSat[1].reset(); }
        prevClarActive = clarActive; prevAirActive = airActive;

        for (int c = 0; c < numCh; ++c)
        {
            const float x = ch[c][i] * inG;

            // 5-band phase-compensated split
            float b[5], r1, r2, r3, lo;
            xo[0].split (c, x,  lo, r1);
            b[0] = ap[2].process (c, ap[1].process (c, ap[0].process (c, lo)));
            xo[1].split (c, r1, lo, r2);
            b[1] = ap[4].process (c, ap[3].process (c, lo));
            xo[2].split (c, r2, lo, r3);
            b[2] = ap[5].process (c, lo);
            xo[3].split (c, r3, b[3], b[4]);

            float y = 0.f;
            for (int s = 0; s < 5; ++s)
            {
                float yb = b[s];
                if (W[s] > 1.0e-5)
                {
                    const float sat = bandSat[s][c].process (b[s], D[s], Bi[s], compTab, atkC, relC[s], heatC, W[s]);
                    yb = b[s] + (float) W[s] * (sat - b[s]);
                }
                bandPk[s] = juce::jmax (bandPk[s], std::abs (yb));
                y += yb * lv[s];
            }

            // DC blocker (bias-generated offset)
            const double yd = (double) y - dcX[c] + dcR * dcY[c];
            dcX[c] = (double) y; dcY[c] = yd; y = (float) yd;

            // Tone tilt
            y = toneHi.process (c, toneLo.process (c, y));

            // Clarity: level-normalised harmonic exciter on 2.4-7.5 kHz, added in phase
            if (clarActive)
            {
                const float bp = clarLp.process (c, clarHp.process (c, y));
                const float e = clarSat[c].process (bp, 0.8 + 2.2 * (double) cl, 0.06, compTab, atkC, relClar, heatC, 0.0);
                y += 0.75f * cl * e;
            }

            // Air: wide Maag-style shelf + exciter above 6.5 kHz (new harmonics land in 10-20 kHz)
            if (airActive)
            {
                const float hp = airHp.process (c, y);
                const float e = airSat[c].process (hp, 1.0 + 2.2 * (double) ai, 0.04, compTab, atkC, relAir, heatC, 0.0);
                y = airShelf.process (c, y) + 0.9f * ai * e;
            }

            y *= outG;
            if (sc > 0.f || clipTarget > 0.f) y += sc * (softClip (y) - y);
            ch[c][i] = y;
        }
    }

    os->processSamplesDown (sub);

    for (int s = 0; s < 5; ++s)
        bandMeter[s].store (juce::jmax (bandPk[s], bandMeter[s].load() * meterDecay));

    // ---- dry/wet, output meters, output scope tap ----
    float* wp[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float outPk[2] = { 0.f, 0.f };
    for (int i = 0; i < n; ++i)
    {
        const float m = smMix.getNextValue();
        float mono = 0.f;
        for (int c = 0; c < numCh; ++c)
        {
            const float o = dryBuf.getSample (c, i) * (1.f - m) + wp[c][i] * m;
            wp[c][i] = o;
            outPk[c] = juce::jmax (outPk[c], std::abs (o));
            mono += o;
        }
        monoOut[(size_t) i] = mono / (float) numCh;
    }
    for (int c = 0; c < numCh; ++c) outLevel[c].store (juce::jmax (outPk[c], outLevel[c].load() * meterDecay));
    if (numCh == 1) outLevel[1].store (outLevel[0].load());
    outScope.push (monoOut.data(), n);
}

//==============================================================================
juce::AudioProcessorEditor* SolarisProcessor::createEditor() { return new SolarisEditor (*this); }

void SolarisProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void SolarisProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SolarisProcessor(); }
