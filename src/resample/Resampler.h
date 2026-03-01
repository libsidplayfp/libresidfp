/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <cmath>
#include <cassert>
#include <cstdint>

#include "siddefs-fp.h"

#include "fpm/fixed.hpp"
#include "fpm/math.hpp"

namespace reSIDfp
{

/**
 * Abstraction of a resampling process. Given enough input, produces output.
 * Constructors take additional arguments that configure these objects.
 */
class Resampler
{
using FP_17_15 = fpm::fixed<std::int32_t, std::int64_t, 15>;   // Q17.15 format

private:
    // Padé approximation of tanh
    static constexpr inline FP_17_15 sid_tanh(FP_17_15 x) noexcept
    {
        if (unlikely(x > FP_17_15(3)))
            return FP_17_15(1);

        const FP_17_15 x2 = x * x;
        const FP_17_15 num = x*(FP_17_15(945) + x2*(FP_17_15(105) + x2));
        const FP_17_15 den = FP_17_15(945) + x2*(FP_17_15(420) + x2*FP_17_15(15));
        return num/den;
    }

    template<int m>
    static inline int clipper(int x) noexcept
    {
        assert(x >= 0);

        // leave values below threshold untouched
        constexpr int threshold = 28000;
        if (likely(x < threshold))
            return x;

        // avoid overflows
        using L = std::numeric_limits<FP_17_15>;
        if (unlikely(x > static_cast<int>(L::max())))
            x = static_cast<int>(L::max());

        constexpr FP_17_15 max_val = FP_17_15(m);
        constexpr FP_17_15 t = FP_17_15(threshold) / max_val;
        constexpr FP_17_15 a = 1 - t;
        constexpr FP_17_15 b = 1 / a;

        FP_17_15 value = FP_17_15(x - threshold) / max_val;
        value = t + a * sid_tanh(b * value);
        return static_cast<int>(value * max_val);
    }

    /*
     * Soft Clipping implementation, splitted for test.
     */
    static inline int softClipImpl(int x) noexcept
    {
        return x < 0 ? -clipper<32768>(-x) : clipper<32767>(x);
    }

protected:
    /*
     * Soft Clipping into 16 bit range [-32768,32767]
     */
    static inline short softClip(int x) noexcept { return static_cast<short>(softClipImpl(x)); }

    virtual int output() const = 0;

    Resampler() {}

public:
    virtual ~Resampler() = default;

    /**
     * Input a sample into resampler. Output "true" when resampler is ready with new sample.
     *
     * @param sample input sample
     * @return true when a sample is ready
     */
    virtual bool input(int sample) = 0;

    /**
     * Output a sample from resampler.
     *
     * @return resampled sample
     */
    inline short getOutput(int scaleFactor) const
    {
        const int out = (scaleFactor * output()) / 2;
        return softClip(out);
    }

    virtual void reset() = 0;
};

} // namespace reSIDfp

#endif
