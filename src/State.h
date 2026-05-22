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

#ifndef STATE_H
#define STATE_H

#include <cstdint>

namespace reSIDfp
{

struct State
{
    uint8_t registers[0x20];

    // SID
    uint8_t bus_value;
    int bus_value_ttl;
    unsigned int nextVoiceSync;

    // Waveform
    uint32_t accumulator[3];
    uint32_t shift_register[3];
    uint32_t shift_latch[3];
    unsigned int shift_register_reset[3];
    int shift_pipeline[3];
    uint32_t pulse_output[3];
    unsigned int floating_output_ttl[3];

    // Envelope
    uint16_t lfsr[3];
    uint16_t rate[3];
    unsigned int exponential_counter[3];
    unsigned int exponential_counter_period[3];
    uint8_t envelope_counter[3];
    //EnvelopeGenerator::State envelope_state[3];
    bool counter_enabled[3];
    unsigned int envelope_pipeline[3];
    unsigned int exponential_pipeline[3];
};

} // namespace reSIDfp

#endif
