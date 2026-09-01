/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2026 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#ifndef ZEROORDER_RESAMPLER_H
#define ZEROORDER_RESAMPLER_H

#include "Resampler.h"

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * Return sample with linear interpolation.
 *
 * @author Antti Lankila
 */
class ZeroOrderResampler final : public Resampler
{
    friend class State;

private:
    /// Last sample
    float cachedSample;

    /// Number of cycles per sample
    const float cyclesPerSample;

    float sampleOffset;

private:
    ZeroOrderResampler(const ZeroOrderResampler&) = delete;
    ZeroOrderResampler& operator=(const ZeroOrderResampler&) = delete;

public:
    ZeroOrderResampler(double clockFrequency, double samplingFrequency) :
        cachedSample(0.f),
        cyclesPerSample(static_cast<float>(clockFrequency / samplingFrequency)),
        sampleOffset(0.f) {}

    bool input(float sample) override
    {
        bool ready = false;

        if (unlikely(sampleOffset < 1.f))
        {
            outputValue = cachedSample + (sampleOffset * (sample - cachedSample));
            ready = true;
            sampleOffset += cyclesPerSample;
        }

        sampleOffset -= 1.f;

        cachedSample = sample;

        return ready;
    }

    void reset() override
    {
        cachedSample = 0.f;
        sampleOffset = 0.f;
        outputValue = 0.f;
    }
};

} // namespace reSIDfp

#endif
