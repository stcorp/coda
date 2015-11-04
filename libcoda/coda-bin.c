/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#include "coda-bin-internal.h"

#include "coda-definition.h"
#include "coda-bin-definition.h"

int coda_bin_close(coda_ProductFile *pf)
{
    return coda_ascbin_close(pf);
}

int coda_bin_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type)
{
    *type = (coda_Type *)dynamic_type;
    return 0;
}
