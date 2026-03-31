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

#include <iostream>
#include <iomanip>
#include <vector>

#include <ctime>
#include <cmath>

#include "limiter.h"

// g++ $CXXFLAGS -I../../build/src test_limiter.cpp

class float_limiter
{
private:
    template<int m>
    static inline int softClipper(int x) noexcept
    {
        assert(x >= 0);
        constexpr int threshold = 28000;
        if (likely(x < threshold))
            return x;

        constexpr double max_val = static_cast<double>(m);
        constexpr double t = threshold / max_val;
        constexpr double a = 1. - t;
        constexpr double b = 1. / a;

        double value = static_cast<double>(x - threshold) / max_val;
        value = t + a * std::tanh(b * value);
        return static_cast<int>(value * max_val);
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

int main(int, const char*[])
{
    // first run is slower
    std::vector<short> results_discard;
    {
        //clock_t start = clock();

        for (int val = -38000; val < 38000; val++)
        {
            results_discard.push_back(limiter::softClip(val));
        }

        //clock_t end = clock();

        //std::cout << "Time floating point: " << (end - start) * 1000. / CLOCKS_PER_SEC << " ms" << std::endl;
    }

    std::vector<short> results;

    {
        clock_t start = clock();
        for (int val = -38000000; val < 38000000; val++)
        {
            results.push_back(limiter::softClip(val));
        }

        clock_t end = clock();

        std::cout << "Time fixed point: " << (end - start) * 1000. / CLOCKS_PER_SEC << " ms" << std::endl;
    }

    std::vector<short> results_float;
    {
        clock_t start = clock();

        for (int val = -38000000; val < 38000000; val++)
        {
            results_float.push_back(limiter::softClip(val));
        }

        clock_t end = clock();

        std::cout << "Time floating point: " << (end - start) * 1000. / CLOCKS_PER_SEC << " ms" << std::endl;
    }

    for (size_t i = 0; i < results.size(); i++)
    {
        int diff = results[i] - results_float[i];
        if (diff) {
            std::cout << (i-38000000) << " -> " << diff << std::endl;
        }
    }
}

