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

#ifndef NOISE_H
#define NOISE_H

namespace reSIDfp
{

class Noise
{
private:
    mutable uint32_t m_wnoise = 34653463u;
    mutable int32_t m_hpnoise = 0;

    const uint32_t m_mask;
    const int32_t m_middle;

public:
    Noise(int bits) :
        m_mask((1u << bits) - 1),
        m_middle(1 << (bits - 1))
    {}

    inline int32_t output() const
    {
        // white noise
        m_wnoise = m_wnoise * 1664525u + 1013904223u;

        // Reduce to 8bit signed
        int32_t n = (int32_t)((m_wnoise >> 20) & m_mask) - m_middle;

        // high-passed noise
        m_hpnoise = m_hpnoise + ((n - m_hpnoise) / 8);

        return n - m_hpnoise;
    }
};

} // namespace reSIDfp

#endif
