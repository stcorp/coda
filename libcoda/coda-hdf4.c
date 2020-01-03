/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "coda-hdf4-internal.h"

#include <assert.h>

static int init_GRImages(coda_hdf4_product *product)
{
    if (GRfileinfo(product->gr_id, &(product->num_images), &(product->num_gr_file_attributes)) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        return -1;
    }
    if (product->num_images > 0)
    {
        int i;

        product->gri = malloc(product->num_images * sizeof(coda_hdf4_GRImage *));
        if (product->gri == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)product->num_images * sizeof(coda_hdf4_GRImage *), __FILE__, __LINE__);
            return -1;
        }
        for (i = 0; i < product->num_images; i++)
        {
            product->gri[i] = NULL;
        }
        for (i = 0; i < product->num_images; i++)
        {
            product->gri[i] = coda_hdf4_GRImage_new(product, i);
            if (product->gri[i] == NULL)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int init_SDSs(coda_hdf4_product *product)
{
    if (SDfileinfo(product->sd_id, &(product->num_sds), &(product->num_sd_file_attributes)) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        return -1;
    }
    if (product->num_sds > 0)
    {
        int i;

        product->sds = malloc(product->num_sds * sizeof(coda_hdf4_SDS *));
        if (product->sds == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)product->num_sds * sizeof(coda_hdf4_SDS *), __FILE__, __LINE__);
            return -1;
        }
        for (i = 0; i < product->num_sds; i++)
        {
            product->sds[i] = NULL;
        }
        for (i = 0; i < product->num_sds; i++)
        {
            product->sds[i] = coda_hdf4_SDS_new(product, i);
            if (product->sds[i] == NULL)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int init_Vdatas(coda_hdf4_product *product)
{
    int32 vdata_ref;

    vdata_ref = VSgetid(product->file_id, -1);
    while (vdata_ref != -1)
    {
        if (product->num_vdata % BLOCK_SIZE == 0)
        {
            coda_hdf4_Vdata **vdata;
            int i;

            vdata = realloc(product->vdata, (product->num_vdata + BLOCK_SIZE) * sizeof(coda_hdf4_Vdata *));
            if (vdata == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)(product->num_vdata + BLOCK_SIZE) * sizeof(coda_hdf4_Vdata *), __FILE__, __LINE__);
                return -1;
            }
            product->vdata = vdata;
            for (i = product->num_vdata; i < product->num_vdata + BLOCK_SIZE; i++)
            {
                product->vdata[i] = NULL;
            }
        }
        product->num_vdata++;
        product->vdata[product->num_vdata - 1] = coda_hdf4_Vdata_new(product, vdata_ref);
        if (product->vdata[product->num_vdata - 1] == NULL)
        {
            return -1;
        }
        vdata_ref = VSgetid(product->file_id, vdata_ref);
    }

    return 0;
}

static int init_Vgroups(coda_hdf4_product *product)
{
    int32 vgroup_ref;
    int result;
    int i;

    vgroup_ref = Vgetid(product->file_id, -1);
    while (vgroup_ref != -1)
    {
        if (product->num_vgroup % BLOCK_SIZE == 0)
        {
            coda_hdf4_Vgroup **vgroup;
            int i;

            vgroup = realloc(product->vgroup, (product->num_vgroup + BLOCK_SIZE) * sizeof(coda_hdf4_Vgroup *));
            if (vgroup == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)(product->num_vgroup + BLOCK_SIZE) * sizeof(coda_hdf4_Vgroup *), __FILE__,
                               __LINE__);
                return -1;
            }
            product->vgroup = vgroup;
            for (i = product->num_vgroup; i < product->num_vgroup + BLOCK_SIZE; i++)
            {
                product->vgroup[i] = NULL;
            }
        }
        product->num_vgroup++;
        /* This will not yet create the links to the entries of the Vgroup */
        product->vgroup[product->num_vgroup - 1] = coda_hdf4_Vgroup_new(product, vgroup_ref);
        if (product->vgroup[product->num_vgroup - 1] == NULL)
        {
            return -1;
        }
        vgroup_ref = Vgetid(product->file_id, vgroup_ref);
    }

    /* Now for each Vgroup create the links to its entries */
    for (i = 0; i < product->num_vgroup; i++)
    {
        coda_hdf4_Vgroup *type;
        int32 num_entries;

        type = product->vgroup[i];
        num_entries = Vntagrefs(type->vgroup_id);
        if (num_entries < 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            return -1;
        }

        if (num_entries > 0 && !type->hide)
        {
            int32 *tags;
            int32 *refs;
            int j;

            type->entry = malloc(num_entries * sizeof(coda_hdf4_type *));
            if (type->entry == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)num_entries * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
                return -1;
            }
            tags = malloc(num_entries * sizeof(int32));
            if (tags == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)num_entries * sizeof(int32), __FILE__, __LINE__);
                return -1;
            }
            refs = malloc(num_entries * sizeof(int32));
            if (refs == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)num_entries * sizeof(int32), __FILE__, __LINE__);
                free(tags);
                return -1;
            }

            result = Vgettagrefs(type->vgroup_id, tags, refs, num_entries);
            if (result != num_entries)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                free(refs);
                free(tags);
                return -1;
            }

            for (j = 0; j < num_entries; j++)
            {
                int32 index;
                int k;

                switch (tags[j])
                {
                    case DFTAG_RIG:
                    case DFTAG_RI:
                    case DFTAG_RI8:
                        index = GRreftoindex(product->gr_id, (uint16)refs[j]);
                        if (index != -1)
                        {
                            for (k = 0; k < product->num_images; k++)
                            {
                                if (product->gri[k]->index == index)
                                {
                                    product->gri[k]->group_count++;
                                    if (coda_type_record_create_field(type->definition, product->gri[k]->gri_name,
                                                                      (coda_type *)product->gri[k]->definition) != 0)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    type->entry[type->definition->num_fields - 1] = (coda_hdf4_type *)product->gri[k];
                                    break;
                                }
                            }
                            /* if k == product->num_images then the Vgroup links to a non-existent GRImage and
                             * we ignore the entry */
                        }
                        /* if index == -1 then the Vgroup links to a non-existent GRImage and we ignore the entry */
                        break;
                    case DFTAG_SD:
                    case DFTAG_SDG:
                    case DFTAG_NDG:
                        index = SDreftoindex(product->sd_id, refs[j]);
                        if (index != -1)
                        {
                            for (k = 0; k < product->num_sds; k++)
                            {
                                if (product->sds[k]->index == index)
                                {
                                    product->sds[k]->group_count++;
                                    if (coda_type_record_create_field(type->definition, product->sds[k]->sds_name,
                                                                      (coda_type *)product->sds[k]->definition) != 0)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    type->entry[type->definition->num_fields - 1] = (coda_hdf4_type *)product->sds[k];
                                    break;
                                }
                            }
                            /* if k == product->num_sds then the Vgroup links to a non-existent SDS and
                             * we ignore the entry */
                        }
                        /* if index == -1 then the Vgroup links to a non-existent SDS and we ignore the entry */
                        break;
                    case DFTAG_VH:
                    case DFTAG_VS:
                        for (k = 0; k < product->num_vdata; k++)
                        {
                            if (product->vdata[k]->ref == refs[j])
                            {
                                if (!product->vdata[k]->hide)
                                {
                                    product->vdata[k]->group_count++;
                                    if (coda_type_record_create_field(type->definition, product->vdata[k]->vdata_name,
                                                                      (coda_type *)product->vdata[k]->definition) != 0)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    type->entry[type->definition->num_fields - 1] = (coda_hdf4_type *)product->vdata[k];
                                }
                                break;
                            }
                        }
                        /* if k == product->num_vdata then the Vgroup links to a non-existent Vdata and
                         * we ignore the entry */
                        break;
                    case DFTAG_VG:
                        for (k = 0; k < product->num_vgroup; k++)
                        {
                            if (product->vgroup[k]->ref == refs[j])
                            {
                                if (!product->vgroup[k]->hide)
                                {
                                    product->vgroup[k]->group_count++;
                                    if (coda_type_record_create_field(type->definition, product->vgroup[k]->vgroup_name,
                                                                      (coda_type *)product->vgroup[k]->definition) != 0)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    type->entry[type->definition->num_fields - 1] =
                                        (coda_hdf4_type *)product->vgroup[k];
                                }
                                break;
                            }
                        }
                        /* if k == product->num_vgroup then the Vgroup links to a non-existent Vgroup and
                         * we ignore the entry */
                        break;
                    default:
                        /* The Vgroup contains an unsupported item and we ignore the entry */
                        break;
                }
            }
            free(refs);
            free(tags);
        }
    }

    return 0;
}

int coda_hdf4_close(coda_product *product)
{
    coda_hdf4_product *product_file = (coda_hdf4_product *)product;
    int i;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
    }

    /* first remove everything that was not added to the root_type */
    if (product_file->vgroup != NULL)
    {
        for (i = 0; i < product_file->num_vgroup; i++)
        {
            if (product_file->vgroup[i] != NULL && (product_file->vgroup[i]->group_count != 0 ||
                                                    product_file->vgroup[i]->hide))
            {
                coda_dynamic_type_delete((coda_dynamic_type *)product_file->vgroup[i]);
            }
        }
        free(product_file->vgroup);
    }
    if (product_file->vdata != NULL)
    {
        for (i = 0; i < product_file->num_vdata; i++)
        {
            if (product_file->vdata[i] != NULL && (product_file->vdata[i]->group_count != 0 ||
                                                   product_file->vdata[i]->hide))
            {
                coda_dynamic_type_delete((coda_dynamic_type *)product_file->vdata[i]);
            }
        }
        free(product_file->vdata);
    }
    if (product_file->sds != NULL)
    {
        for (i = 0; i < product_file->num_sds; i++)
        {
            if (product_file->sds[i] != NULL && product_file->sds[i]->group_count != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)product_file->sds[i]);
            }
        }
        free(product_file->sds);
    }
    if (product_file->gri != NULL)
    {
        for (i = 0; i < product_file->num_images; i++)
        {
            if (product_file->gri[i] != NULL && product_file->gri[i]->group_count != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)product_file->gri[i]);
            }
        }
        free(product_file->gri);
    }

    /* then remove the root_type (which recursively removes its members) */
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)product_file->root_type);
    }


    if (product_file->sd_id != -1)
    {
        SDend(product_file->sd_id);
    }
    if (product_file->is_hdf)
    {
        if (product_file->gr_id != -1)
        {
            GRend(product_file->gr_id);
        }
        if (product_file->an_id != -1)
        {
            ANend(product_file->an_id);
        }
        if (product_file->file_id != -1)
        {
            Vend(product_file->file_id);
            Hclose(product_file->file_id);
        }
    }

    free(product_file);

    return 0;
}

int coda_hdf4_reopen(coda_product **product)
{
    coda_hdf4_product *product_file;

    product_file = (coda_hdf4_product *)malloc(sizeof(coda_hdf4_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_hdf4_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_hdf4;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;
    product_file->is_hdf = 0;
    product_file->file_id = -1;
    product_file->gr_id = -1;
    product_file->sd_id = -1;
    product_file->an_id = -1;
    product_file->num_gr_file_attributes = 0;
    product_file->num_sd_file_attributes = 0;
    product_file->num_sds = 0;
    product_file->sds = NULL;
    product_file->num_images = 0;
    product_file->gri = NULL;
    product_file->num_vgroup = 0;
    product_file->vgroup = NULL;
    product_file->num_vdata = 0;
    product_file->vdata = NULL;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_hdf4_close((coda_product *)product_file);
        coda_close(*product);
        return -1;
    }

    coda_close(*product);

    product_file->is_hdf = Hishdf(product_file->filename);      /* is this a real HDF4 file or a (net)CDF file */
    if (product_file->is_hdf)
    {
        product_file->file_id = Hopen(product_file->filename, DFACC_READ, 0);
        if (product_file->file_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        if (Vstart(product_file->file_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        product_file->gr_id = GRstart(product_file->file_id);
        if (product_file->gr_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        product_file->an_id = ANstart(product_file->file_id);
        if (product_file->an_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
    }
    product_file->sd_id = SDstart(product_file->filename, DFACC_READ);
    if (product_file->sd_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }
    product_file->root_type = NULL;

    if (init_SDSs(product_file) != 0)
    {
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }
    if (product_file->is_hdf)
    {
        if (init_GRImages(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        if (init_Vdatas(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        /* initialization of Vgroup entries should happen last, so we can build the structural tree */
        if (init_Vgroups(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
    }

    if (coda_hdf4_create_root(product_file) != 0)
    {
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

void coda_hdf4_add_error_message(void)
{
    int error = HEvalue(1);

    if (error != 0)
    {
        coda_add_error_message("[HDF4] %s", HEstring(error));
    }
}
