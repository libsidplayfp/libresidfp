/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2026 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef LIMITER_H
#define LIMITER_H

#include <cmath>
#include <cassert>
#include <cstdint>

#include "siddefs-fp.h"

class Limiter
{
private:
    static constexpr float threshold = 28000.f;

    template<int m>
    static inline float clipper(float x)
    {
        static_assert(m > 0, "Clipper range must be a positive value");
        assert(!std::signbit(x));
        if (likely(x < threshold))
            return x;

        constexpr float max_val = static_cast<float>(m);
        constexpr float max_val_inv = 1.f / max_val;
        constexpr float t = threshold / max_val;
        constexpr float a = 1. - t;
        constexpr float b = 1. / a;

        float value = (x - threshold) * max_val_inv;
        value = a * std::tanh(b * value);
        return threshold + (value * max_val);
    }

    /*
     * Soft Clipping implementation, splitted for test.
     */
    static inline float softClipImpl(float x)
    {
        return std::signbit(x) ? -clipper<32768>(-x) : clipper<32767>(x);
    }

public:
    /*
     * Soft Clipping into 16 bit range [-32768,32767]
     */
    static inline int16_t softClip(float x) { return static_cast<int16_t>(softClipImpl(x)); }
};

#endif
