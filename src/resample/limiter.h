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

#ifndef CLIPPER_H
#define CLIPPER_H

#include <limits>

#include <cassert>
#include <cmath>
#include <cstdint>

#include "siddefs-fp.h"

class limiter
{
private:
    // Fixed point Q17.15
    static constexpr int DECIMAL_BITS = 15;

    static constexpr inline std::int32_t TO_FP_15(std::int32_t x) { return x << DECIMAL_BITS; }
    static constexpr inline std::int32_t FROM_FP_15(std::int32_t x) { return x >> DECIMAL_BITS; }
    static constexpr inline std::int32_t MAX_FP_15() { return std::numeric_limits<std::int32_t>::max(); }

    // Padé approximation of tanh
    static constexpr inline std::int32_t fp_tanh(std::int32_t x) noexcept
    {
        if (unlikely(x > TO_FP_15(3)))
            return TO_FP_15(1);

        const std::int32_t x2 = x * x;
        const std::int32_t num = x*(TO_FP_15(945) + x2*(TO_FP_15(105) + x2));
        const std::int32_t den = TO_FP_15(945) + x2*(TO_FP_15(420) + x2*TO_FP_15(15));
        return num/den;
    }

    template<int m>
    static inline int softClipper(int x) noexcept
    {
        static_assert(m > 0);
        assert(x >= 0);

        // leave values below threshold untouched
        constexpr int threshold = 28000;
        if (likely(x < threshold))
            return x;

        // avoid overflows
        constexpr int fpMax = FROM_FP_15(MAX_FP_15());
        if (unlikely(x > fpMax))
            x = fpMax;

        constexpr std::int32_t max_val = TO_FP_15(m);
        constexpr std::int32_t t = TO_FP_15(threshold) / max_val;
        constexpr std::int32_t a = TO_FP_15(1) - t;
        constexpr std::int32_t b = TO_FP_15(1) / a;

        std::int32_t value = TO_FP_15(x - threshold) / max_val;
        value = a * fp_tanh(b * value);
        return static_cast<int>(threshold + FROM_FP_15(value * max_val));
    }

    /*
     * Soft Clipping implementation, splitted for test.
     */
    static inline int softClipImpl(int x) noexcept
    {
        return x < 0 ? -softClipper<32768>(-x) : softClipper<32767>(x);
    }

public:
    /*
     * Soft Clipping into 16 bit range [-32768,32767]
     */
    static inline short softClip(int x) noexcept { return static_cast<short>(softClipImpl(x)); }
};

#endif
