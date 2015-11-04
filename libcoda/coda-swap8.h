/*
 * Copyright (C) 2007-2014 S[&]T, The Netherlands.
 *
 * This file is part of CODA.
 *
 * CODA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CODA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CODA; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CODA_SWAP8_H
#define CODA_SWAP8_H

static void swap8(void *value)
{
    unsigned char *v = (unsigned char *)value;

    /* use XOR swap algorithm to swap bytes 0/7, 1/6, 2/5, and 3/4 */
    v[0] = v[0] ^ v[7];
    v[7] = v[0] ^ v[7];
    v[0] = v[0] ^ v[7];
    v[1] = v[1] ^ v[6];
    v[6] = v[1] ^ v[6];
    v[1] = v[1] ^ v[6];
    v[2] = v[2] ^ v[5];
    v[5] = v[2] ^ v[5];
    v[2] = v[2] ^ v[5];
    v[3] = v[3] ^ v[4];
    v[4] = v[3] ^ v[4];
    v[3] = v[3] ^ v[4];
}

#define swap_int64 swap8
#define swap_uint64 swap8
#define swap_double swap8

#endif
