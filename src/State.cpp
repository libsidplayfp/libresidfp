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

#include "State.h"

namespace reSIDfp
{

State::State()
{
/*
    for (int i = 0; i < 0x20; i++)
    {
        registers[i] = 0;
    }
*/
    bus_value = 0u;
    bus_value_ttl = 0;
    nextVoiceSync = 0u;

    for (int i = 0; i < 3; i++)
    {
        accumulator[i] = 0u;
        shift_register[i] = 0x7fffffu;
        shift_latch[i] = 0u;
        shift_register_reset[i] = 0u;
        shift_pipeline[i] = 0;
        pulse_output[i] = 0u;
        floating_output_ttl[i] = 0u;

        lfsr[i] = 0x7fffu;
        rate[i] = 0x007fu;
        exponential_counter[i] = 0u;
        exponential_counter_period[i] = 1u;
        envelope_counter[i] = 0u;
        state[i] = EnvelopeGenerator::State::RELEASE;
        counter_enabled[i] = true;
        envelope_pipeline[i] = 0u;
        exponential_pipeline[i] = 0u;
    }
}

} // namespace reSIDfp
