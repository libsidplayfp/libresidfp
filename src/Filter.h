/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2025 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FILTER_H
#define FILTER_H

#include "FilterModelConfig.h"
#include "Voice.h"

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * SID filter base class
 */
class Filter
{
private:
    uint16_t* mixer;
    uint16_t* summer;
    uint16_t* resonance;
    uint16_t* volume;

    FilterModelConfig& fmc;

    /// Current filter/voice mixer setting.
    uint16_t* currentMixer = nullptr;

    /// Filter input summer setting.
    uint16_t* currentSummer = nullptr;

    /// Filter resonance value.
    uint16_t* currentResonance = nullptr;

    /// Current volume amplifier setting.
    uint16_t* currentVolume = nullptr;

protected:
    /// Filter highpass state.
    int32_t Vhp = 0;

    /// Filter bandpass state.
    int32_t Vbp = 0;

    /// Filter lowpass state.
    int32_t Vlp = 0;

private:
    /// Filter external input.
    int32_t Ve = 0;

    /// Filter cutoff frequency.
    uint16_t fc = 0;

    /// Routing to filter or outside filter
    //@{
    bool filt1 = false;
    bool filt2 = false;
    bool filt3 = false;
    bool filtE = false;
    //@}

    /// Switch voice 3 off.
    bool voice3off = false;

protected:
    /// Highpass, bandpass, and lowpass filter modes.
    //@{
    bool hp = false;
    bool bp = false;
    bool lp = false;
    //@}

    double leakMixer  = 0.00255;  // 6581: 0.00255  / 8580: 0.00119
    double leakFilter = 0.001725; // 6581: 0.001725 / 8580: 0.0008
    double leakV3     = 0.1125;   // 6581: 0.1125   / 8580: 0.0525

private:
    /// Current volume.
    uint8_t vol = 0;

    /// Filter enabled.
    bool enabled = true;

    /// Selects which inputs to route through filter.
    uint8_t filt = 0;


private:
    inline int32_t getNormalizedVoice(Voice& v) const
    {
        return fmc.getNormalizedVoice(v.output(), v.envelope()->output());
    }

protected:
    /**
     * Update filter cutoff frequency.
     */
    virtual void updateCenterFrequency() = 0;

    /**
     * Update filter resonance.
     *
     * @param res the new resonance value
     */
    void updateResonance(uint8_t res) { currentResonance = resonance + (res * (1<<16)); }

    /**
     * Mixing configuration modified (offsets change)
     */
    void updateMixing();

    /**
     * Get the filter cutoff register value
     */
    inline unsigned int getFC() const { return static_cast<unsigned int>(fc); }

    virtual int32_t solveIntegrators() = 0;

    static inline int32_t signalLeak(int32_t input, double leak)
    {
        int32_t leaked = static_cast<int32_t>(leak * (1 << 12));
        return ((input-32767) * leaked) >> 12;
    }

public:
    Filter(FilterModelConfig& fmc);

    virtual ~Filter() = default;

    /**
     * SID clocking - 1 cycle
     *
     * @param voice1 voice 1 in
     * @param voice2 voice 2 in
     * @param voice3 voice 3 in
     * @return filtered output, unsigned 16 bit
     */
    uint16_t clock(Voice& voice1, Voice& voice2, Voice& voice3);

    /**
     * Enable filter.
     *
     * @param enable
     */
    void enable(bool enable);

    /**
     * SID reset.
     */
    void reset();

    /**
     * Write Frequency Cutoff Low register.
     *
     * @param fc_lo Frequency Cutoff Low-Byte
     */
    void writeFC_LO(uint8_t fc_lo);

    /**
     * Write Frequency Cutoff High register.
     *
     * @param fc_hi Frequency Cutoff High-Byte
     */
    void writeFC_HI(uint8_t fc_hi);

    /**
     * Write Resonance/Filter register.
     *
     * @param res_filt Resonance/Filter
     */
    void writeRES_FILT(uint8_t res_filt);

    /**
     * Write filter Mode/Volume register.
     *
     * @param mode_vol Filter Mode/Volume
     */
    void writeMODE_VOL(uint8_t mode_vol);

    /**
     * Apply a signal to EXT-IN
     *
     * @param input a signed 16 bit sample
     */
    void input(int16_t input) { Ve = fmc.getNormalizedVoice(input/32768.f, 0); }
};

} // namespace reSIDfp

#if RESIDFP_INLINING || defined(FILTER_CPP)

namespace reSIDfp
{

RESIDFP_INLINE
uint16_t Filter::clock(Voice& voice1, Voice& voice2, Voice& voice3)
{
    const int32_t V1 = getNormalizedVoice(voice1);
    const int32_t V2 = getNormalizedVoice(voice2);
    const int32_t V3 = getNormalizedVoice(voice3);

    int32_t Vsum = 0;
    int32_t Vmix = 0;

    Vsum += filt1 ? V1 : signalLeak(V1, leakFilter);
    Vmix += filt1 ? signalLeak(V1, leakMixer) : V1;

    Vsum += filt2 ? V2 : signalLeak(V2, leakFilter);
    Vmix += filt2 ? signalLeak(V2, leakMixer) : V2;

    Vsum += filt3 ? V3 : signalLeak(V3, leakFilter);
    // Voice 3 is silenced by voice3off if it is not routed through the filter.
    Vmix += filt3 ? signalLeak(V3, leakMixer) : voice3off ? signalLeak(V3, leakV3) : V3;

    Vsum += filtE ? Ve : signalLeak(Ve, leakFilter);
    Vmix += filtE ? signalLeak(Ve, leakMixer) : Ve;

    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vsum];

    Vmix += solveIntegrators();

    return currentVolume[currentMixer[Vmix]];
}

} // namespace reSIDfp

#endif

#endif
