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

#include "EnvelopeGenerator.h"
#include "residfp/residfp_defs.h"

#include <cstdint>

namespace reSIDfp
{

struct State
{
    State(); // FIXME remove?

    // SID
    uint8_t bus_value;
    int bus_value_ttl;
    unsigned int nextVoiceSync;
    ChipModel model;
    CombinedWaveforms cws;

    // Waveform
    uint32_t pw[3];
    uint32_t shift_register[3];
    uint32_t shift_latch[3];
    uint32_t ring_msb_mask[3];
    uint32_t no_noise[3];
    uint32_t noise_output[3];
    uint32_t no_noise_or_noise_output[3];
    uint32_t no_pulse[3];
    uint32_t pulse_output[3];
    uint32_t waveform_output[3];
    uint32_t accumulator[3];
    uint32_t freq[3];
    uint32_t tri_saw_pipeline[3];
    uint32_t osc3[3];
    int shift_pipeline[3];
    unsigned int shift_register_reset[3];
    unsigned int floating_output_ttl[3];
    uint8_t waveform[3];
    bool test[3];
    bool sync[3];
    bool test_or_reset[3];
    bool msb_rising[3];

    // Envelope
    uint16_t lfsr[3];
    uint16_t rate[3];
    unsigned int exponential_counter[3];
    unsigned int exponential_counter_period[3];
    unsigned int new_exponential_counter_period[3];
    unsigned int state_pipeline[3];
    unsigned int envelope_pipeline[3];
    unsigned int exponential_pipeline[3];
    EnvelopeGenerator::State state[3];
    EnvelopeGenerator::State next_state[3];
    bool counter_enabled[3];
    bool gate[3];
    bool resetLfsr[3];
    uint8_t envelope_counter[3];
    uint8_t attack[3];
    uint8_t decay[3];
    uint8_t sustain[3];
    uint8_t release[3];
    uint8_t env3[3];
};

} // namespace reSIDfp

#endif
