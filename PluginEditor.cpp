#include "PluginEditor.h"
#include <cstring>

using namespace juce;
using namespace solaris;

//==============================================================================
SolarisLnF::SolarisLnF()
{
    setColour (Slider::textBoxTextColourId, ui::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId, ui::dim);
}

void SolarisLnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float tw = jmax (3.f, r * 0.085f);
    const bool active = s.isMouseOverOrDragging();

    g.setColour (Colour (0x28ffffff));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.f;
        const float r0 = r + 1.f, r1 = r - tw * 0.9f;
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.f);
    }
    const float ar = r - tw * 1.8f;

    Path track; track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
    g.setColour (ui::edge);
    g.strokePath (track, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));

    Path val; val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, ang, true);
    g.setColour (ui::violet.withAlpha (active ? 0.34f : 0.20f));
    g.strokePath (val, PathStrokeType (tw * 2.8f, PathStrokeType::curved, PathStrokeType::rounded));
    ColourGradient arc (ui::cyan, c.x - r, c.y, ui::magenta, c.x + r, c.y, false);
    arc.addColour (0.5, ui::violet);
    g.setGradientFill (arc);
    g.strokePath (val, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));

    const float kr = ar - tw * 1.3f;
    g.setColour (Colour (0x70000000));
    g.fillEllipse (Rectangle<float> (kr * 2.1f, kr * 2.1f).withCentre ({ c.x, c.y + kr * 0.08f }));
    g.setGradientFill (ColourGradient (Colour (0xff3b4359), c.x - kr * 0.6f, c.y - kr, Colour (0xff0d1017), c.x + kr * 0.6f, c.y + kr, false));
    g.fillEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c));
    g.setGradientFill (ColourGradient (Colour (0x34ffffff), c.x, c.y - kr, Colour (0x00ffffff), c.x, c.y, false));
    g.fillEllipse (Rectangle<float> (kr * 1.7f, kr * 1.7f).withCentre ({ c.x, c.y - kr * 0.05f }));
    g.setColour (active ? ui::cyan.withAlpha (0.85f) : ui::edge.brighter (0.3f));
    g.drawEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c), 1.2f);

    Path p; p.addRoundedRectangle (-1.8f, -kr * 0.9f, 3.6f, kr * 0.36f, 1.6f);
    g.setColour (Colours::white);
    g.fillPath (p, AffineTransform::rotation (ang).translated (c));
}

void SolarisLnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                                   Slider::SliderStyle, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.f, 8.f);
    const float cx = b.getCentreX();
    const auto col = s.findColour (Slider::thumbColourId);

    g.setColour (Colour (0x22ffffff));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = b.getY() + b.getHeight() * (float) i / 10.f;
        g.drawLine (cx - 12.f, ty, cx - 7.f, ty, 1.f);
        g.drawLine (cx + 7.f, ty, cx + 12.f, ty, 1.f);
    }
    g.setColour (Colour (0xff0a0c12));
    g.fillRoundedRectangle (Rectangle<float> (7.f, b.getHeight()).withCentre (b.getCentre()), 3.5f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (Rectangle<float> (7.f, b.getHeight()).withCentre (b.getCentre()), 3.5f, 1.f);

    const float top = jlimit (b.getY(), b.getBottom(), sliderPos);
    g.setColour (col.withAlpha (0.25f));
    g.fillRoundedRectangle (Rectangle<float> (cx - 5.f, top, 10.f, b.getBottom() - top), 4.f);
    g.setGradientFill (ColourGradient (col, cx, top, col.withAlpha (0.15f), cx, b.getBottom(), false));
    g.fillRoundedRectangle (Rectangle<float> (cx - 2.5f, top, 5.f, b.getBottom() - top), 2.5f);

    auto thumb = Rectangle<float> (jmin (30.f, b.getWidth()), 15.f).withCentre ({ cx, top });
    g.setColour (col.withAlpha (0.3f));
    g.fillRoundedRectangle (thumb.expanded (3.f), 6.f);
    g.setGradientFill (ColourGradient (Colour (0xff434b63), thumb.getX(), thumb.getY(), Colour (0xff1a1f2c), thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.5f);
    g.setColour (col);
    g.fillRoundedRectangle (Rectangle<float> (thumb.getWidth() - 8.f, 2.f).withCentre (thumb.getCentre()), 1.f);
}

void SolarisLnF::drawToggleButton (Graphics& g, ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const bool on = b.getToggleState();
    const float rad = r.getHeight() * 0.5f;
    if (on)
    {
        g.setColour (ui::cyan.withAlpha (0.16f));
        g.fillRoundedRectangle (r.expanded (3.f), rad + 3.f);
        g.setGradientFill (ColourGradient (ui::cyan.withAlpha (0.38f), r.getX(), 0.f, ui::violet.withAlpha (0.38f), r.getRight(), 0.f, false));
    }
    else
        g.setColour (hl ? Colour (0xff1a2030) : ui::panel);
    g.fillRoundedRectangle (r, rad);
    g.setColour (on ? ui::cyan : ui::edge);
    g.drawRoundedRectangle (r, rad, 1.4f);

    const float led = r.getHeight() * 0.28f;
    auto ledR = Rectangle<float> (led, led).withCentre ({ r.getX() + rad, r.getCentreY() });
    g.setColour (on ? ui::cyan : Colour (0xff2a3144));
    g.fillEllipse (ledR);
    g.setColour (on ? Colours::white : ui::dim);
    g.setFont (jmax (10.f, r.getHeight() * 0.42f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (rad + led), Justification::centred);
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& a, const String& id, const String& name, const String& suffix, int decimals)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (12.f)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (a, id, slider);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (18));
    slider.setBounds (b);
}

AuraKnob::AuraKnob (SolarisProcessor& p, const String& id, const String& name, const String& suffix)
    : Knob (p.apvts, id, name, suffix, 1), proc (p)
{
    label.setFont (Font (FontOptions (14.f)));
    label.setColour (Label::textColourId, ui::text);
    startTimerHz (30);
}

void AuraKnob::timerCallback()
{
    const float l = jmax (proc.outLevel[0].load(), proc.outLevel[1].load());
    pulse += (jlimit (0.f, 1.f, l) - pulse) * 0.35f;
    repaint();
}

void AuraKnob::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto c = b.getCentre();
    c.y += 9.f;
    const float d = (float) slider.valueToProportionOfLength (slider.getValue());
    const float rad = jmin (b.getWidth(), b.getHeight()) * (0.52f + 0.22f * d + 0.12f * pulse);
    ColourGradient aura (ui::violet.withAlpha (0.10f + 0.40f * d + 0.12f * pulse), c.x, c.y,
                         Colours::transparentBlack, c.x + rad, c.y, true);
    aura.addColour (0.55, ui::cyan.withAlpha (0.05f + 0.16f * d));
    g.setGradientFill (aura);
    g.fillEllipse (Rectangle<float> (rad * 2.f, rad * 2.f).withCentre (c));
}

VFader::VFader (AudioProcessorValueTreeState& a, const String& id, const String& name, const String& suffix,
                int decimals, Colour c)
{
    slider.setSliderStyle (Slider::LinearVertical);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 70, 20);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (Slider::thumbColourId, c);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (11.f)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (a, id, slider);
}

void VFader::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (16));
    slider.setBounds (b);
}

BandStrip::BandStrip (SolarisProcessor& p, int band)
    : proc (p), index (band),
      drive (p.apvts, "bd" + String (band + 1), "DRIVE", "%", 0, ui::bandColours[band]),
      lvl (p.apvts, "bl" + String (band + 1), "LEVEL", " dB", 1, ui::bandColours[band])
{
    addAndMakeVisible (drive);
    addAndMakeVisible (lvl);
    startTimerHz (30);
}

void BandStrip::timerCallback()
{
    level += (proc.bandMeter[index].load() - level) * 0.45f;
    repaint (meterRect.toNearestInt().expanded (2));
}

void BandStrip::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.f);
    const auto col = ui::bandColours[index];
    g.setGradientFill (ColourGradient (Colour (0xff151a26), 0.f, b.getY(), Colour (0xff0b0e15), 0.f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 9.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (b, 9.f, 1.f);
    g.setGradientFill (ColourGradient (col.withAlpha (0.9f), b.getX(), 0.f, col.withAlpha (0.2f), b.getRight(), 0.f, false));
    g.fillRoundedRectangle (Rectangle<float> (b.getX() + 8.f, b.getY() + 1.f, b.getWidth() - 16.f, 3.f), 1.5f);
    g.setColour (ui::text);
    g.setFont (13.f);
    g.drawText (kBandNames[index], getLocalBounds().removeFromTop (34).withTrimmedTop (8), Justification::centred);
    g.setColour (ui::dim);
    g.setFont (10.f);
    g.drawText (kBandRanges[index], getLocalBounds().removeFromTop (48).withTrimmedTop (30), Justification::centred);

    // LED band meter
    g.setColour (Colour (0xff07090e));
    g.fillRoundedRectangle (meterRect, 3.f);
    const float db = Decibels::gainToDecibels (level, -60.f);
    const float fill = jlimit (0.f, 1.f, (db + 48.f) / 48.f);
    const int segs = 18;
    for (int i = 0; i < segs; ++i)
    {
        auto seg = Rectangle<float> (meterRect.getX() + 1.f, meterRect.getBottom() - (float) (i + 1) * meterRect.getHeight() / (float) segs + 1.f,
                                     meterRect.getWidth() - 2.f, meterRect.getHeight() / (float) segs - 2.f);
        const bool lit = (float) (i + 1) / (float) segs <= fill + 0.001f;
        g.setColour (lit ? (i > segs - 4 ? ui::amber : col) : Colour (0xff141824));
        g.fillRoundedRectangle (seg, 1.2f);
    }
}

void BandStrip::resized()
{
    auto b = getLocalBounds().reduced (6);
    b.removeFromTop (48);
    auto left = b.removeFromLeft ((int) (b.getWidth() * 0.42f));
    auto right = b.removeFromRight ((int) (b.getWidth() * 0.72f));
    drive.setBounds (left);
    lvl.setBounds (right);
    meterRect = b.reduced (2, 22).withTrimmedBottom (20).toFloat();
}

//==============================================================================
LevelMeter::LevelMeter (SolarisProcessor& p, bool isInput) : proc (p), input (isInput) { startTimerHz (30); }

void LevelMeter::timerCallback()
{
    for (int c = 0; c < 2; ++c)
    {
        const float v = (input ? proc.inLevel[c] : proc.outLevel[c]).load();
        lv[c] += (v - lv[c]) * 0.5f;
        hold[c] = jmax (v, hold[c] * 0.985f);
    }
    repaint();
}

void LevelMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff090b11));
    g.fillRoundedRectangle (b, 6.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);
    g.setColour (ui::dim);
    g.setFont (9.5f);
    g.drawText (input ? "IN" : "OUT", b.removeFromBottom (16.f), Justification::centred);
    b = b.reduced (5.f, 6.f);
    const float bw = (b.getWidth() - 3.f) / 2.f;
    for (int c = 0; c < 2; ++c)
    {
        auto bar = Rectangle<float> (b.getX() + (float) c * (bw + 3.f), b.getY(), bw, b.getHeight());
        g.setColour (Colour (0xff12161f));
        g.fillRoundedRectangle (bar, 2.f);
        const float f = jlimit (0.f, 1.f, (Decibels::gainToDecibels (lv[c], -60.f) + 60.f) / 60.f);
        auto fill = bar.withTop (bar.getBottom() - bar.getHeight() * f);
        ColourGradient grad (ui::amber, 0.f, bar.getY(), ui::cyan, 0.f, bar.getBottom(), false);
        grad.addColour (0.12, ui::magenta);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 2.f);
        const float hf = jlimit (0.f, 1.f, (Decibels::gainToDecibels (hold[c], -60.f) + 60.f) / 60.f);
        g.setColour (Colours::white.withAlpha (0.85f));
        g.fillRect (bar.getX(), bar.getBottom() - bar.getHeight() * hf - 1.f, bar.getWidth(), 1.5f);
    }
}

//==============================================================================
void ModeSwitch::setMulti (bool m, bool animate)
{
    target = m ? 1.f : 0.f;
    if (! animate) { pos = target; repaint(); return; }
    startTimerHz (60);
}

void ModeSwitch::timerCallback()
{
    pos += (target - pos) * 0.3f;
    if (std::abs (target - pos) < 0.003f) { pos = target; stopTimer(); }
    repaint();
}

void ModeSwitch::mouseDown (const MouseEvent& e)
{
    const bool m = e.position.x > (float) getWidth() * 0.5f;
    if (m != (target > 0.5f)) { setMulti (m, true); if (onChange) onChange (m); }
}

void ModeSwitch::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.f);
    const float rad = b.getHeight() * 0.5f;
    g.setColour (Colour (0xff0a0d14));
    g.fillRoundedRectangle (b, rad);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (b, rad, 1.2f);

    auto half = b.reduced (3.f);
    half.setWidth (half.getWidth() * 0.5f);
    half.setX (half.getX() + pos * half.getWidth());
    g.setColour (ui::violet.withAlpha (0.28f));
    g.fillRoundedRectangle (half.expanded (3.f), rad);
    g.setGradientFill (ColourGradient (ui::cyan.withAlpha (0.85f), half.getX(), 0.f, ui::violet.withAlpha (0.85f), half.getRight(), 0.f, false));
    g.fillRoundedRectangle (half, rad - 3.f);

    g.setFont (jmax (11.f, b.getHeight() * 0.36f));
    auto l = b.withWidth (b.getWidth() * 0.5f), r = l.translated (b.getWidth() * 0.5f, 0.f);
    g.setColour (pos < 0.5f ? Colours::black : ui::dim);
    g.drawText ("SINGLE", l, Justification::centred);
    g.setColour (pos >= 0.5f ? Colours::black : ui::dim);
    g.drawText ("MULTIBAND", r, Justification::centred);
}

//==============================================================================
SpectrumView::SpectrumView (SolarisProcessor& p)
    : proc (p), fft (fftOrder), window ((size_t) fftSize, dsp::WindowingFunction<float>::hann)
{
    pMulti = p.apvts.getRawParameterValue ("multi");
    pDrive = p.apvts.getRawParameterValue ("drive");
    for (int i = 0; i < 5; ++i) pBd[i] = p.apvts.getRawParameterValue ("bd" + String (i + 1));
    smoothOut.fill (-120.f); smoothIn.fill (-120.f); frozenOut.fill (-120.f); frozenIn.fill (-120.f);
    startTimerHz (30);
}

float SpectrumView::xForFreq (float f) const { return plot.getX() + plot.getWidth() * std::log (jmax (20.f, f) / 20.f) / std::log (1000.f); }
float SpectrumView::freqForX (float x) const { return 20.f * std::pow (1000.f, (x - plot.getX()) / plot.getWidth()); }
float SpectrumView::yForDb (float db) const   { return plot.getY() + plot.getHeight() * (6.f - jlimit (-90.f, 6.f, db)) / 96.f; }

float SpectrumView::dbAt (const Spec& s, float f0, float f1) const
{
    const float binHz = (float) proc.getSampleRateSafe() / (float) fftSize;
    const float b0 = f0 / binHz, b1 = f1 / binHz;
    const int i0 = jlimit (1, numBins - 3, (int) std::floor (b0));
    const int i1 = jlimit (i0, numBins - 2, (int) std::ceil (b1));
    if (b1 - b0 < 1.f)
    {
        const float bm = 0.5f * (b0 + b1);
        const int k = jlimit (1, numBins - 3, (int) bm);
        return s[(size_t) k] + (s[(size_t) k + 1] - s[(size_t) k]) * (bm - (float) k);
    }
    float m = -200.f;
    for (int k = i0; k <= i1; ++k) m = jmax (m, s[(size_t) k]);
    return m;
}

Path SpectrumView::makePath (const Spec& s, float step) const
{
    Path p;
    bool started = false;
    const float nyq = (float) proc.getSampleRateSafe() * 0.5f;
    for (float px = 0.f; px <= plot.getWidth(); px += step)
    {
        const float x = plot.getX() + px;
        const float f0 = freqForX (x), f1 = freqForX (x + step);
        if (f1 > nyq) break;
        const float y = yForDb (dbAt (s, f0, f1));
        if (! started) { p.startNewSubPath (x, y); started = true; }
        else p.lineTo (x, y);
    }
    return p;
}

std::vector<SpectrumView::Peak> SpectrumView::findPeaks (const Spec& s, float fLo, float fHi, int maxN) const
{
    const float binHz = (float) proc.getSampleRateSafe() / (float) fftSize;
    const int kLo = jmax (2, (int) std::ceil (fLo / binHz));
    const int kHi = jmin (numBins - 3, (int) std::floor (fHi / binHz));
    std::vector<Peak> all;
    for (int k = kLo; k <= kHi; ++k)
    {
        const float a = s[(size_t) k - 1], b = s[(size_t) k], c = s[(size_t) k + 1];
        if (b > a && b >= c && b > -80.f)
        {
            const float den = a - 2.f * b + c;
            const float dl = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
            all.push_back ({ ((float) k + dl) * binHz, b - 0.25f * (a - c) * dl });
        }
    }
    std::sort (all.begin(), all.end(), [] (const Peak& l, const Peak& r) { return l.db > r.db; });
    std::vector<Peak> out;
    for (auto& p : all)
    {
        bool ok = true;
        for (auto& q : out) if (std::abs (std::log2 (p.f / q.f)) < 0.25f) { ok = false; break; }
        if (ok) out.push_back (p);
        if ((int) out.size() >= maxN) break;
    }
    return out;
}

int SpectrumView::rangeForX (float x) const
{
    const float f = freqForX (x);
    for (int i = 0; i < 5; ++i) if (f >= (float) kEdges[i] && f < (float) kEdges[i + 1]) return i;
    return -1;
}

String SpectrumView::fmtFreq (float f)
{
    return f >= 1000.f ? String (f / 1000.f, 2) + " kHz" : String ((int) std::round (f)) + " Hz";
}

void SpectrumView::feed (ScopeFifo& f, Win& w)
{
    float tmp[2048];
    for (;;)
    {
        const int n = f.pull (tmp, 2048);
        if (n <= 0) break;
        std::memmove (w.data(), w.data() + n, sizeof (float) * (size_t) (fftSize - n));
        std::copy (tmp, tmp + n, w.begin() + (fftSize - n));
    }
}

void SpectrumView::analyse (const Win& w, Spec& out)
{
    std::copy (w.begin(), w.end(), fftData.begin());
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    const float norm = (float) fftSize * 0.25f;
    for (int i = 0; i < numBins; ++i)
    {
        const float db = Decibels::gainToDecibels (fftData[(size_t) i] / norm, -120.f);
        float& s = out[(size_t) i];
        s += (db - s) * (db > s ? 0.6f : 0.15f);
    }
}

void SpectrumView::timerCallback()
{
    feed (proc.outScope, winOut);
    feed (proc.inScope, winIn);
    if (! frozen)
    {
        analyse (winOut, smoothOut);
        analyse (winIn, smoothIn);
    }
    repaint();
}

void SpectrumView::mouseMove (const MouseEvent& e)
{
    hovered = plot.contains (e.position) ? rangeForX (e.position.x) : -1;
    if (hovered >= 0 && ! frozen && ! suppress)
    {
        frozenOut = smoothOut;
        frozenIn = smoothIn;
        frozen = true;
    }
}

void SpectrumView::mouseDown (const MouseEvent&) { frozen = false; suppress = true; }
void SpectrumView::mouseExit (const MouseEvent&) { suppress = false; if (! frozen) hovered = -1; }

void SpectrumView::paint (Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff0d111a), 0.f, 0.f, Colour (0xff06080d), 0.f, full.getBottom(), false));
    g.fillRoundedRectangle (full, 12.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (full.reduced (0.5f), 12.f, 1.f);

    plot = full.reduced (36.f, 26.f);

    const bool multi = pMulti->load() > 0.5f;
    const float master = pDrive->load() * 0.01f;

    // range shading + live drive "heat map"
    for (int i = 0; i < 5; ++i)
    {
        auto r = Rectangle<float> (xForFreq ((float) kEdges[i]), plot.getY(),
                                   xForFreq ((float) kEdges[i + 1]) - xForFreq ((float) kEdges[i]), plot.getHeight());
        g.setColour (ui::bandColours[i].withAlpha (hovered == i ? 0.14f : 0.04f));
        g.fillRect (r);
        const float d = multi ? pBd[i]->load() * 0.01f : jmin (1.f, master * kVoiceDrive[i]);
        if (d > 0.002f)
        {
            const float hgt = r.getHeight() * (0.20f + 0.60f * d);
            g.setGradientFill (ColourGradient (ui::bandColours[i].withAlpha (0.05f + 0.28f * d), r.getX(), r.getBottom(),
                                               Colours::transparentBlack, r.getX(), r.getBottom() - hgt, false));
            g.fillRect (r.withTop (r.getBottom() - hgt));
        }
    }

    // grid
    g.setFont (10.f);
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xForFreq (f);
        g.setColour (Colour (0x12ffffff));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (ui::dim);
        g.drawText (f >= 1000.f ? String (f / 1000.f, 0) + "k" : String ((int) f),
                    Rectangle<float> (x - 20, plot.getBottom() + 4, 40, 14), Justification::centred);
    }
    for (int db = 0; db >= -84; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (Colour (0x10ffffff));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (ui::dim);
        g.drawText (String (db), Rectangle<float> (2, y - 7, 30, 14), Justification::right);
    }

    const Spec& specOut = frozen ? frozenOut : smoothOut;
    const Spec& specIn  = frozen ? frozenIn  : smoothIn;

    // input ghost
    {
        Path in = makePath (specIn, 2.f);
        if (! in.isEmpty())
        {
            Path fill (in);
            fill.lineTo (fill.getCurrentPosition().x, plot.getBottom());
            fill.lineTo (plot.getX(), plot.getBottom());
            fill.closeSubPath();
            g.setColour (Colour (0x0cffffff));
            g.fillPath (fill);
            g.setColour (Colour (0x55c9d1e0));
            g.strokePath (in, PathStrokeType (1.1f, PathStrokeType::curved));
        }
    }

    // output curve
    Path line = makePath (specOut, 1.5f);
    if (! line.isEmpty())
    {
        Path fill (line);
        fill.lineTo (fill.getCurrentPosition().x, plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        const Colour c = frozen ? ui::amber : ui::cyan;
        g.setGradientFill (ColourGradient (c.withAlpha (0.34f), 0.f, plot.getY(), c.withAlpha (0.02f), 0.f, plot.getBottom(), false));
        g.fillPath (fill);
        g.setColour (c.withAlpha (0.18f));
        g.strokePath (line, PathStrokeType (5.f, PathStrokeType::curved));
        if (frozen)
            g.setColour (c);
        else
        {
            ColourGradient cg (ui::bandColours[0], plot.getX(), 0.f, ui::bandColours[4], plot.getRight(), 0.f, false);
            for (int i = 0; i < 5; ++i)
            {
                const float mid = 0.5f * (xForFreq ((float) kEdges[i]) + xForFreq ((float) kEdges[i + 1]));
                cg.addColour (jlimit (0.0, 1.0, (double) ((mid - plot.getX()) / plot.getWidth())), ui::bandColours[i]);
            }
            g.setGradientFill (cg);
        }
        g.strokePath (line, PathStrokeType (1.8f, PathStrokeType::curved));
    }

    // dividers + headers
    for (int i = 0; i < 5; ++i)
    {
        const float xa = xForFreq ((float) kEdges[i]), xb = xForFreq ((float) kEdges[i + 1]);
        if (i > 0)
        {
            g.setColour (Colour (0x2effffff));
            g.drawVerticalLine ((int) xa, plot.getY(), plot.getBottom());
        }
        auto pk = findPeaks (specOut, (float) kEdges[i], (float) kEdges[i + 1], 1);
        g.setColour (ui::bandColours[i]);
        g.setFont (11.f);
        g.drawText (kBandNames[i], Rectangle<float> (xa, 5.f, xb - xa, 16.f), Justification::centred);
        if (! pk.empty())
        {
            g.setColour (ui::dim);
            g.setFont (9.5f);
            g.drawText (String (pk[0].db, 1) + " dB", Rectangle<float> (xa, plot.getY() + 2.f, xb - xa, 12.f), Justification::centred);
        }
    }

    g.setFont (9.5f);
    g.setColour (Colour (0x88c9d1e0));
    g.drawText ("INPUT", Rectangle<float> (plot.getRight() - 120.f, plot.getBottom() - 16.f, 56.f, 12.f), Justification::centredRight);
    g.setColour (ui::cyan);
    g.drawText ("OUTPUT", Rectangle<float> (plot.getRight() - 60.f, plot.getBottom() - 16.f, 56.f, 12.f), Justification::centredRight);

    // frozen: list the peaks of the hovered range
    if (frozen && hovered >= 0)
    {
        auto peaks = findPeaks (frozenOut, (float) kEdges[hovered], (float) kEdges[hovered + 1], 4);
        Rectangle<float> lastBox;
        for (auto& p : peaks)
        {
            const float x = xForFreq (p.f), y = yForDb (p.db);
            g.setColour (Colours::white);
            g.fillEllipse (x - 3.5f, y - 3.5f, 7.f, 7.f);
            g.setColour (ui::amber);
            g.drawEllipse (x - 5.f, y - 5.f, 10.f, 10.f, 1.2f);

            auto box = Rectangle<float> (120.f, 18.f).withCentre ({ x, y - 22.f });
            if (box.getX() < plot.getX()) box.setX (plot.getX());
            if (box.getRight() > plot.getRight()) box.setRight (plot.getRight());
            if (box.getY() < plot.getY() + 14.f) box.setY (plot.getY() + 14.f);
            if (! lastBox.isEmpty() && box.intersects (lastBox)) box.setY (lastBox.getBottom() + 2.f);
            lastBox = box;

            g.setColour (Colour (0xe8101520));
            g.fillRoundedRectangle (box, 6.f);
            g.setColour (ui::amber.withAlpha (0.85f));
            g.drawRoundedRectangle (box, 6.f, 1.f);
            g.setColour (Colours::white);
            g.setFont (11.f);
            g.drawText (fmtFreq (p.f) + "   " + String (p.db, 1) + " dB", box, Justification::centred);
        }
        g.setColour (ui::amber);
        g.setFont (10.5f);
        g.drawText ("FROZEN  -  click to resume live monitoring",
                    Rectangle<float> (plot.getX(), plot.getY() + 14.f, plot.getWidth() - 4.f, 14.f), Justification::centredRight);
    }
}

//==============================================================================
SolarisEditor::SolarisEditor (SolarisProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), inMeter (p, true), outMeter (p, false),
      driveKnob (p, "drive", "D R I V E", " %"),
      inKnob (p.apvts, "in", "INPUT", " dB"),
      outKnob (p.apvts, "out", "OUTPUT", " dB"),
      warmKnob (p.apvts, "warmth", "WARMTH", " %", 0),
      toneKnob (p.apvts, "tone", "TONE", "", 0),
      mixKnob (p.apvts, "mix", "MIX", " %", 0),
      clarKnob (p.apvts, "clarity", "AMOUNT", " %", 0),
      airKnob (p.apvts, "air", "AMOUNT", " %", 0)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);
    addAndMakeVisible (modeSwitch);
    for (auto* k : { (Component*) &driveKnob, (Component*) &inKnob, (Component*) &outKnob, (Component*) &warmKnob,
                     (Component*) &toneKnob, (Component*) &mixKnob, (Component*) &clarKnob, (Component*) &airKnob })
        addAndMakeVisible (*k);
    for (auto* b : { &clarBtn, &airBtn, &clipBtn }) addAndMakeVisible (*b);
    clarAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "clarityOn", clarBtn);
    airAtt  = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "airOn", airBtn);
    clipAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "softclip", clipBtn);

    for (int i = 0; i < 5; ++i)
    {
        strips[i] = std::make_unique<BandStrip> (p, i);
        addChildComponent (*strips[i]);
    }

    modeSwitch.onChange = [this] (bool m) { setMulti (m); };
    multiShown = proc.apvts.getRawParameterValue ("multi")->load() > 0.5f;
    modeSwitch.setMulti (multiShown, false);
    updateMode();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1100.0 / 700.0);
    setResizeLimits (880, 560, 1760, 1120);
    setSize (1100, 700);
    startTimerHz (15);
}

SolarisEditor::~SolarisEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SolarisEditor::setMulti (bool m)
{
    if (auto* prm = proc.apvts.getParameter ("multi"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (m ? 1.f : 0.f);
        prm->endChangeGesture();
    }
    multiShown = m;
    updateMode();
}

void SolarisEditor::timerCallback()
{
    const bool m = proc.apvts.getRawParameterValue ("multi")->load() > 0.5f;
    if (m != multiShown) { multiShown = m; modeSwitch.setMulti (m, true); updateMode(); }
}

void SolarisEditor::updateMode()
{
    driveKnob.setVisible (! multiShown);
    for (auto& s : strips) s->setVisible (multiShown);
    resized();
    repaint();
}

static void drawPanel (Graphics& g, Rectangle<int> r, const String& title, Colour accent)
{
    auto f = r.toFloat();
    g.setColour (accent.withAlpha (0.05f));
    g.fillRoundedRectangle (f.expanded (2.f), 14.f);
    g.setGradientFill (ColourGradient (Colour (0xff161a25), 0.f, f.getY(), Colour (0xff0b0e15), 0.f, f.getBottom(), false));
    g.fillRoundedRectangle (f, 12.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (f.reduced (0.5f), 12.f, 1.f);
    g.setGradientFill (ColourGradient (accent, f.getX() + 14.f, 0.f, accent.withAlpha (0.f), f.getX() + 90.f, 0.f, false));
    g.fillRoundedRectangle (Rectangle<float> (f.getX() + 14.f, f.getY() + 10.f, 76.f, 2.f), 1.f);
    g.setColour (ui::text);
    g.setFont (11.5f);
    g.drawText (title, Rectangle<float> (f.getX() + 14.f, f.getY() + 13.f, f.getWidth() - 28.f, 16.f), Justification::centredLeft);
}

void SolarisEditor::paint (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    g.setGradientFill (ColourGradient (Colour (0xff0d111a), 0.f, 0.f, ui::bg, 0.f, H, false));
    g.fillAll();
    g.setGradientFill (ColourGradient (ui::violet.withAlpha (0.10f), W * 0.15f, 0.f, Colours::transparentBlack, W * 0.15f, H * 0.7f, true));
    g.fillAll();
    g.setGradientFill (ColourGradient (ui::cyan.withAlpha (0.07f), W * 0.9f, H, Colours::transparentBlack, W * 0.9f, H * 0.3f, true));
    g.fillAll();

    auto top = getLocalBounds().removeFromTop (proportionOfHeight (0.075f)).reduced (20, 8);
    auto logo = top.removeFromLeft (top.getHeight()).toFloat().reduced (2.f);
    g.setColour (ui::amber.withAlpha (0.22f));
    g.fillEllipse (logo.expanded (5.f));
    g.setGradientFill (ColourGradient (ui::amber, logo.getX(), logo.getY(), ui::magenta, logo.getRight(), logo.getBottom(), false));
    g.fillEllipse (logo);
    g.setColour (ui::bg);
    g.fillEllipse (logo.reduced (logo.getWidth() * 0.30f));
    g.setGradientFill (ColourGradient (ui::cyan, logo.getCentreX(), logo.getCentreY(), ui::violet, logo.getRight(), logo.getBottom(), false));
    g.fillEllipse (logo.reduced (logo.getWidth() * 0.40f));

    top.removeFromLeft (12);
    g.setColour (Colours::white);
    g.setFont ((float) top.getHeight() * 0.66f);
    g.drawText ("SOLARIS", top.removeFromLeft (proportionOfWidth (0.125f)), Justification::centredLeft);
    g.setColour (ui::magenta);
    g.drawText ("2", top.removeFromLeft (proportionOfWidth (0.03f)), Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont ((float) top.getHeight() * 0.34f);
    g.drawText ("HARMONIC SATURATOR  |  by FIBONACCI", top.removeFromLeft (proportionOfWidth (0.30f)), Justification::centredLeft);

    drawPanel (g, leftPanel, "GAIN & COLOUR", ui::cyan);
    drawPanel (g, centrePanel, multiShown ? "MULTIBAND SATURATION  -  5 BANDS" : "SATURATION", ui::amber);
    drawPanel (g, rightPanel, "ENHANCE  /  OUTPUT", ui::bandColours[2]);
}

void SolarisEditor::resized()
{
    auto b = getLocalBounds();
    auto top = b.removeFromTop (proportionOfHeight (0.075f)).reduced (20, 10);
    modeSwitch.setBounds (top.removeFromRight (proportionOfWidth (0.24f)));

    auto specArea = b.removeFromTop (proportionOfHeight (0.41f)).reduced (12, 2);
    const int mw = jmax (26, proportionOfWidth (0.028f));
    outMeter.setBounds (specArea.removeFromRight (mw));
    specArea.removeFromRight (6);
    inMeter.setBounds (specArea.removeFromRight (mw));
    specArea.removeFromRight (6);
    spectrum.setBounds (specArea);

    auto area = b.reduced (12, 6);
    leftPanel = area.removeFromLeft (proportionOfWidth (0.22f));   area.removeFromLeft (8);
    rightPanel = area.removeFromRight (proportionOfWidth (0.30f)); area.removeFromRight (8);
    centrePanel = area;

    auto left = leftPanel.reduced (8, 0).withTrimmedTop (30).withTrimmedBottom (6);
    auto lrow1 = left.removeFromTop (left.getHeight() / 2);
    inKnob.setBounds (lrow1.removeFromLeft (lrow1.getWidth() / 2));
    outKnob.setBounds (lrow1);
    warmKnob.setBounds (left.removeFromLeft (left.getWidth() / 2));
    toneKnob.setBounds (left);

    auto right = rightPanel.reduced (8, 0).withTrimmedTop (30).withTrimmedBottom (6);
    auto row1 = right.removeFromTop (right.getHeight() / 2);
    auto cell = [] (Rectangle<int> r, Knob* k, Component* btn)
    {
        auto btnArea = r.removeFromBottom ((int) ((float) r.getHeight() * 0.24f));
        if (k) k->setBounds (r);
        if (btn) btn->setBounds (btnArea.reduced (btnArea.getWidth() / 8, 2));
    };
    cell (row1.removeFromLeft (row1.getWidth() / 2), &clarKnob, &clarBtn);
    cell (row1, &airKnob, &airBtn);
    mixKnob.setBounds (right.removeFromLeft (right.getWidth() / 2));
    clipBtn.setBounds (right.withSizeKeepingCentre (right.getWidth() * 4 / 5, jmax (30, right.getHeight() / 4)));

    auto c = centrePanel.reduced (10, 0).withTrimmedTop (30).withTrimmedBottom (8);
    driveKnob.setBounds (c.withSizeKeepingCentre (jmin (c.getWidth(), (int) ((float) c.getHeight() * 1.25f)), c.getHeight()));
    const int sw = c.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
        strips[i]->setBounds (c.removeFromLeft (sw));
}
