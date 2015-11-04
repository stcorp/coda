/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

#ifndef CODA_XML_H
#define CODA_XML_H

#include "coda-internal.h"

void coda_xml_type_delete(coda_dynamic_type *type);
int coda_xml_type_update(coda_dynamic_type **type, coda_type *definition);

int coda_xml_recognize_file(const char *filename, int64_t size, coda_product_definition **definition);
int coda_xml_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                  coda_product **product);
int coda_xml_close(coda_product *product);
int coda_xml_cursor_set_product(coda_cursor *cursor, coda_product *product);

#endif
