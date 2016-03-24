/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#ifndef CODA_SWAP2_H
#define CODA_SWAP2_H

static void swap2(void *value)
{
    unsigned char *v = (unsigned char *)value;

    /* use XOR swap algorithm to swap bytes 0/1 */
    v[0] = v[0] ^ v[1];
    v[1] = v[0] ^ v[1];
    v[0] = v[0] ^ v[1];
}

#define swap_int16 swap2
#define swap_uint16 swap2

#endif
