/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "utpp/utpp.h"

#include "../src/residfp/residfp.h"

using namespace UnitTest;
using namespace reSIDfp;

SUITE(SID)
{

#define BUF_SIZE 1024
#define CYCLES 21333

TEST(TestCycles)
{
    residfp s;

    short buf[BUF_SIZE];
    s.setSamplingParameters(1000000, DECIMATE, 48000);
    int c = s.clock(buf, BUF_SIZE);
    CHECK(c == CYCLES);
}

TEST(TestBufsize)
{
    residfp s;

    short buf[BUF_SIZE];
    s.setSamplingParameters(1000000, DECIMATE, 48000);
    int b = s.clock(CYCLES, buf);
    CHECK(b == BUF_SIZE);
}

}
