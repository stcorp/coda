/*
 * Copyright (C) 2007-2021 S[&]T, The Netherlands.
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

#include "coda-sp3.h"
#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-mem-internal.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1000

enum
{
    sp3_pos_vel,
    sp3_datetime_start_string,
    sp3_datetime_start,
    sp3_num_epochs,
    sp3_data_used,
    sp3_coordinate_sys,
    sp3_orbit_type,
    sp3_agency,
    sp3_gps_week,
    sp3_sec_of_week,
    sp3_epoch_interval,
    sp3_mjd_start,
    sp3_frac_day,
    sp3_num_satellites,
    sp3_sat_id,
    sp3_sat_id_array,
    sp3_sat_accuracy,
    sp3_sat_accuracy_array,
    sp3_file_type,
    sp3_time_system,
    sp3_base_pos_vel,
    sp3_base_clk_rate,

    sp3_vehicle_id,
    sp3_P_x_coordinate,
    sp3_P_y_coordinate,
    sp3_P_z_coordinate,
    sp3_P_clock,
    sp3_P_x_sdev,
    sp3_P_y_sdev,
    sp3_P_z_sdev,
    sp3_P_clock_sdev,
    sp3_P_clock_event_flag,
    sp3_P_clock_pred_flag,
    sp3_P_maneuver_flag,
    sp3_P_orbit_pred_flag,
    sp3_P_corr,

    sp3_EP_x_sdev,
    sp3_EP_y_sdev,
    sp3_EP_z_sdev,
    sp3_EP_clock_sdev,
    sp3_EP_xy_corr,
    sp3_EP_xz_corr,
    sp3_EP_xc_corr,
    sp3_EP_yz_corr,
    sp3_EP_yc_corr,
    sp3_EP_zc_corr,

    sp3_V_x_velocity,
    sp3_V_y_velocity,
    sp3_V_z_velocity,
    sp3_V_clock_rate,
    sp3_V_xvel_sdev,
    sp3_V_yvel_sdev,
    sp3_V_zvel_sdev,
    sp3_V_clkrate_sdev,
    sp3_V_corr,

    sp3_EV_xvel_sdev,
    sp3_EV_yvel_sdev,
    sp3_EV_zvel_sdev,
    sp3_EV_clkrate_sdev,
    sp3_EV_xy_corr,
    sp3_EV_xz_corr,
    sp3_EV_xc_corr,
    sp3_EV_yz_corr,
    sp3_EV_yc_corr,
    sp3_EV_zc_corr,

    sp3_epoch_string,
    sp3_epoch,
    sp3_pos_clk,
    sp3_pos_clk_array,
    sp3_vel_rate,
    sp3_vel_rate_array,

    sp3_header,
    sp3_records,
    sp3_record,
    sp3_file,

    num_sp3_types
};

static THREAD_LOCAL coda_type **sp3_type = NULL;

typedef struct ingest_info_struct
{
    FILE *f;
    coda_product *product;
    coda_mem_record *header;    /* actual data for /header */
    coda_mem_array *records;    /* actual data for /record */
    coda_mem_record *record;    /* actual data for /record[] */
    coda_mem_array *pos_clk_array;      /* actual data for /record[]/pos_clk */
    coda_mem_record *pos_clk;   /* actual data for /record[]/pos_clk[] */
    coda_mem_array *vel_rate_array;     /* actual data for /record[]/vel_rate */
    coda_mem_record *vel_rate;  /* actual data for /record[]/vel_rate[] */
    coda_mem_record *corr;      /* actual data for /record[]/...[]/corr */
    int num_satellites;
    char posvel;
    long linenumber;
    long offset;
} ingest_info;

static void ingest_info_cleanup(ingest_info *info)
{
    if (info->f != NULL)
    {
        fclose(info->f);
    }
    if (info->header != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->header);
    }
    if (info->records != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->records);
    }
    if (info->record != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->record);
    }
    if (info->pos_clk_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->pos_clk_array);
    }
    if (info->pos_clk != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->pos_clk);
    }
    if (info->vel_rate_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->vel_rate_array);
    }
    if (info->vel_rate != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->vel_rate);
    }
    if (info->corr != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->corr);
    }
}

static void ingest_info_init(ingest_info *info)
{
    info->f = NULL;
    info->header = NULL;
    info->records = NULL;
    info->record = NULL;
    info->pos_clk_array = NULL;
    info->pos_clk = NULL;
    info->vel_rate_array = NULL;
    info->vel_rate = NULL;
    info->corr = NULL;
    info->num_satellites = 0;
    info->linenumber = 0;
    info->offset = 0;
}

static int sp3_init(void)
{
    coda_endianness endianness;
    coda_type_record_field *field;
    coda_expression *expr;
    int i;

    if (sp3_type != NULL)
    {
        return 0;
    }

#ifdef WORDS_BIGENDIAN
    endianness = coda_big_endian;
#else
    endianness = coda_little_endian;
#endif

    sp3_type = malloc(num_sp3_types * sizeof(coda_type *));
    if (sp3_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_sp3_types * sizeof(coda_type *), __FILE__, __LINE__);
        return -1;
    }
    for (i = 0; i < num_sp3_types; i++)
    {
        sp3_type[i] = NULL;
    }

    sp3_type[sp3_pos_vel] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_pos_vel], 1);
    coda_type_set_description(sp3_type[sp3_pos_vel], "Position/Velocity Flag 'P' = no velocities are included, "
                              "'V' = at each epoch, for each satellite, an additional satellite velocity and clock "
                              "rate-of-change has been computed");

    sp3_type[sp3_datetime_start_string] = (coda_type *)coda_type_text_new(coda_format_sp3);

    expr = NULL;
    coda_expression_from_string("time(str(.),\"yyyy MM dd HH mm ss*.SSSSSSSS|yyyy MM* dd* HH* mm* ss*.SSSSSSSS\")",
                                &expr);
    sp3_type[sp3_datetime_start] = (coda_type *)coda_type_time_new(coda_format_sp3, expr);
    coda_type_time_set_base_type((coda_type_special *)sp3_type[sp3_datetime_start],
                                 sp3_type[sp3_datetime_start_string]);
    coda_type_set_description(sp3_type[sp3_datetime_start], "Start time");

    sp3_type[sp3_num_epochs] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_num_epochs], endianness);
    coda_type_set_read_type(sp3_type[sp3_num_epochs], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_num_epochs], 32);
    coda_type_set_description(sp3_type[sp3_num_epochs], "Number of Epochs");

    sp3_type[sp3_data_used] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_data_used], 5);
    coda_type_set_description(sp3_type[sp3_data_used], "Data Used");

    sp3_type[sp3_coordinate_sys] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_coordinate_sys], 5);
    coda_type_set_description(sp3_type[sp3_coordinate_sys], "Coordinate System");

    sp3_type[sp3_orbit_type] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_orbit_type], 3);
    coda_type_set_description(sp3_type[sp3_orbit_type], "Orbit Type");

    sp3_type[sp3_agency] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_agency], 4);
    coda_type_set_description(sp3_type[sp3_agency], "Agency");

    sp3_type[sp3_gps_week] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_gps_week], endianness);
    coda_type_set_read_type(sp3_type[sp3_gps_week], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_gps_week], 16);
    coda_type_set_description(sp3_type[sp3_gps_week], "GPS Week");

    sp3_type[sp3_sec_of_week] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_sec_of_week], endianness);
    coda_type_set_bit_size(sp3_type[sp3_sec_of_week], 64);
    coda_type_set_description(sp3_type[sp3_sec_of_week], "Seconds of Week");

    sp3_type[sp3_epoch_interval] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_epoch_interval], endianness);
    coda_type_set_bit_size(sp3_type[sp3_epoch_interval], 64);
    coda_type_set_description(sp3_type[sp3_epoch_interval], "Epoch Interval");

    sp3_type[sp3_mjd_start] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_mjd_start], endianness);
    coda_type_set_read_type(sp3_type[sp3_mjd_start], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_mjd_start], 32);
    coda_type_set_description(sp3_type[sp3_mjd_start], "Modified Julian Day Start");

    sp3_type[sp3_frac_day] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_frac_day], endianness);
    coda_type_set_bit_size(sp3_type[sp3_frac_day], 64);
    coda_type_set_description(sp3_type[sp3_frac_day], "Fractional Day");

    sp3_type[sp3_num_satellites] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_num_satellites], endianness);
    coda_type_set_read_type(sp3_type[sp3_num_satellites], coda_native_type_uint8);
    coda_type_set_bit_size(sp3_type[sp3_num_satellites], 8);
    coda_type_set_description(sp3_type[sp3_num_satellites], "Number of Satellites");

    sp3_type[sp3_sat_id] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_sat_id], 3);
    coda_type_set_description(sp3_type[sp3_sat_id], "Satellite Id");

    sp3_type[sp3_sat_accuracy] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_sat_accuracy], endianness);
    coda_type_set_read_type(sp3_type[sp3_sat_accuracy], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_sat_accuracy], 16);
    coda_type_set_description(sp3_type[sp3_sat_accuracy], "Satellite Accuracy");

    sp3_type[sp3_file_type] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_file_type], 2);
    coda_type_set_description(sp3_type[sp3_file_type], "File Type");

    sp3_type[sp3_time_system] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_time_system], 3);
    coda_type_set_description(sp3_type[sp3_time_system], "Time System");

    sp3_type[sp3_base_pos_vel] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_base_pos_vel], endianness);
    coda_type_set_bit_size(sp3_type[sp3_base_pos_vel], 64);
    coda_type_set_description(sp3_type[sp3_base_pos_vel], "Base for Pos/Vel (mm or 10**-4 mm/sec)");

    sp3_type[sp3_base_clk_rate] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_base_clk_rate], endianness);
    coda_type_set_bit_size(sp3_type[sp3_base_clk_rate], 64);
    coda_type_set_description(sp3_type[sp3_base_clk_rate], "Base for Clk/Rate (psec or 10**-4 psec/sec)");

    sp3_type[sp3_epoch_string] = (coda_type *)coda_type_text_new(coda_format_sp3);

    expr = NULL;
    coda_expression_from_string("time(str(.),\"yyyy MM dd HH mm ss*.SSSSSSSS|yyyy MM* dd* HH* mm* ss*.SSSSSSSS\")",
                                &expr);
    sp3_type[sp3_epoch] = (coda_type *)coda_type_time_new(coda_format_sp3, expr);
    coda_type_time_set_base_type((coda_type_special *)sp3_type[sp3_epoch], sp3_type[sp3_epoch_string]);
    coda_type_set_description(sp3_type[sp3_epoch], "Epoch Start");

    sp3_type[sp3_vehicle_id] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_vehicle_id], 3);
    coda_type_set_description(sp3_type[sp3_vehicle_id], "Vehicle Id");

    sp3_type[sp3_P_x_coordinate] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_x_coordinate], endianness);
    coda_type_set_bit_size(sp3_type[sp3_P_x_coordinate], 64);
    coda_type_set_description(sp3_type[sp3_P_x_coordinate], "x coordinate");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_P_x_coordinate], "km");

    sp3_type[sp3_P_y_coordinate] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_y_coordinate], endianness);
    coda_type_set_bit_size(sp3_type[sp3_P_y_coordinate], 64);
    coda_type_set_description(sp3_type[sp3_P_y_coordinate], "y coordinate");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_P_y_coordinate], "km");

    sp3_type[sp3_P_z_coordinate] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_z_coordinate], endianness);
    coda_type_set_bit_size(sp3_type[sp3_P_z_coordinate], 64);
    coda_type_set_description(sp3_type[sp3_P_z_coordinate], "z coordinate");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_P_z_coordinate], "km");

    sp3_type[sp3_P_clock] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_clock], endianness);
    coda_type_set_bit_size(sp3_type[sp3_P_clock], 64);
    coda_type_set_description(sp3_type[sp3_P_clock], "clock");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_P_clock], "1e-6 s");

    sp3_type[sp3_P_x_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_x_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_P_x_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_P_x_sdev], 8);
    coda_type_set_description(sp3_type[sp3_P_x_sdev], "x sdev (b**n mm)");

    sp3_type[sp3_P_y_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_y_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_P_y_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_P_y_sdev], 8);
    coda_type_set_description(sp3_type[sp3_P_y_sdev], "y sdev (b**n mm)");

    sp3_type[sp3_P_z_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_z_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_P_z_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_P_z_sdev], 8);
    coda_type_set_description(sp3_type[sp3_P_z_sdev], "z sdev (b**n mm)");

    sp3_type[sp3_P_clock_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_P_clock_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_P_clock_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_P_clock_sdev], 16);
    coda_type_set_description(sp3_type[sp3_P_clock_sdev], "clock sdev (b**n psec)");

    sp3_type[sp3_P_clock_event_flag] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_P_clock_event_flag], 1);
    coda_type_set_description(sp3_type[sp3_P_clock_event_flag], "Clock Event Flag");

    sp3_type[sp3_P_clock_pred_flag] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_P_clock_pred_flag], 1);
    coda_type_set_description(sp3_type[sp3_P_clock_pred_flag], "Clock Pred. Flag");

    sp3_type[sp3_P_maneuver_flag] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_P_maneuver_flag], 1);
    coda_type_set_description(sp3_type[sp3_P_maneuver_flag], "Maneuver Flag");

    sp3_type[sp3_P_orbit_pred_flag] = (coda_type *)coda_type_text_new(coda_format_sp3);
    coda_type_set_byte_size(sp3_type[sp3_P_orbit_pred_flag], 1);
    coda_type_set_description(sp3_type[sp3_P_orbit_pred_flag], "Orbit Pred. Flag");

    sp3_type[sp3_EP_x_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_x_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_x_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EP_x_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EP_x_sdev], "x sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EP_x_sdev], "mm");

    sp3_type[sp3_EP_y_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_y_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_y_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EP_y_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EP_y_sdev], "y sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EP_y_sdev], "mm");

    sp3_type[sp3_EP_z_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_z_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_z_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EP_z_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EP_z_sdev], "z sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EP_z_sdev], "mm");

    sp3_type[sp3_EP_clock_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_clock_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_clock_sdev], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_clock_sdev], 32);
    coda_type_set_description(sp3_type[sp3_EP_clock_sdev], "clock sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EP_clock_sdev], "ps");

    sp3_type[sp3_EP_xy_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_xy_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_xy_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_xy_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_xy_corr], "xy correlation");

    sp3_type[sp3_EP_xz_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_xz_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_xz_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_xz_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_xz_corr], "xz correlation");

    sp3_type[sp3_EP_xc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_xc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_xc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_xc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_xc_corr], "xc correlation");

    sp3_type[sp3_EP_yz_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_yz_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_yz_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_yz_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_yz_corr], "yz correlation");

    sp3_type[sp3_EP_yc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_yc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_yc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_yc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_yc_corr], "yc correlation");

    sp3_type[sp3_EP_zc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EP_zc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EP_zc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EP_zc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EP_zc_corr], "zc correlation");

    sp3_type[sp3_V_x_velocity] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_x_velocity], endianness);
    coda_type_set_bit_size(sp3_type[sp3_V_x_velocity], 64);
    coda_type_set_description(sp3_type[sp3_V_x_velocity], "x velocity");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_V_x_velocity], "dm/s");

    sp3_type[sp3_V_y_velocity] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_y_velocity], endianness);
    coda_type_set_bit_size(sp3_type[sp3_V_y_velocity], 64);
    coda_type_set_description(sp3_type[sp3_V_y_velocity], "y velocity");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_V_y_velocity], "dm/s");

    sp3_type[sp3_V_z_velocity] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_z_velocity], endianness);
    coda_type_set_bit_size(sp3_type[sp3_V_z_velocity], 64);
    coda_type_set_description(sp3_type[sp3_V_z_velocity], "z velocity");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_V_z_velocity], "dm/s");

    sp3_type[sp3_V_clock_rate] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_clock_rate], endianness);
    coda_type_set_bit_size(sp3_type[sp3_V_clock_rate], 64);
    coda_type_set_description(sp3_type[sp3_V_clock_rate], "clock rate change");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_V_clock_rate], "1e-10 s/s");

    sp3_type[sp3_V_xvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_xvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_V_xvel_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_V_xvel_sdev], 8);
    coda_type_set_description(sp3_type[sp3_V_xvel_sdev], "xvel sdev (b**n 1e-4 mm/sec)");

    sp3_type[sp3_V_yvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_yvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_V_yvel_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_V_yvel_sdev], 8);
    coda_type_set_description(sp3_type[sp3_V_yvel_sdev], "yvel sdev (b**n 1e-4 mm/sec)");

    sp3_type[sp3_V_zvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_zvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_V_zvel_sdev], coda_native_type_int8);
    coda_type_set_bit_size(sp3_type[sp3_V_zvel_sdev], 8);
    coda_type_set_description(sp3_type[sp3_V_zvel_sdev], "zvel sdev (b**n 1e-4 mm/sec)");

    sp3_type[sp3_V_clkrate_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_V_clkrate_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_V_clkrate_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_V_clkrate_sdev], 16);
    coda_type_set_description(sp3_type[sp3_V_clkrate_sdev], "clock rate sdev (b**n 1e-4 psec/sec)");

    sp3_type[sp3_EV_xvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_xvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_xvel_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EV_xvel_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EV_xvel_sdev], "xvel sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EV_xvel_sdev], "1e-4 mm/s)");

    sp3_type[sp3_EV_yvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_yvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_yvel_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EV_yvel_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EV_yvel_sdev], "yvel sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EV_yvel_sdev], "1e-4 mm/s)");

    sp3_type[sp3_EV_zvel_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_zvel_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_zvel_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EV_zvel_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EV_zvel_sdev], "zvel sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EV_zvel_sdev], "1e-4 mm/s)");

    sp3_type[sp3_EV_clkrate_sdev] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_clkrate_sdev], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_clkrate_sdev], coda_native_type_int16);
    coda_type_set_bit_size(sp3_type[sp3_EV_clkrate_sdev], 16);
    coda_type_set_description(sp3_type[sp3_EV_clkrate_sdev], "clock rate sdev");
    coda_type_number_set_unit((coda_type_number *)sp3_type[sp3_EV_clkrate_sdev], "1e-4 ps/s");

    sp3_type[sp3_EV_xy_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_xy_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_xy_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_xy_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_xy_corr], "xy correlation");

    sp3_type[sp3_EV_xz_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_xz_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_xz_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_xz_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_xz_corr], "xz correlation");

    sp3_type[sp3_EV_xc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_xc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_xc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_xc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_xc_corr], "xc correlation");

    sp3_type[sp3_EV_yz_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_yz_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_yz_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_yz_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_yz_corr], "yz correlation");

    sp3_type[sp3_EV_yc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_yc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_yc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_yc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_yc_corr], "yc correlation");

    sp3_type[sp3_EV_zc_corr] = (coda_type *)coda_type_number_new(coda_format_sp3, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)sp3_type[sp3_EV_zc_corr], endianness);
    coda_type_set_read_type(sp3_type[sp3_EV_zc_corr], coda_native_type_int32);
    coda_type_set_bit_size(sp3_type[sp3_EV_zc_corr], 32);
    coda_type_set_description(sp3_type[sp3_EV_zc_corr], "zc correlation");

    sp3_type[sp3_sat_id_array] = (coda_type *)coda_type_array_new(coda_format_sp3);
    coda_type_array_add_variable_dimension((coda_type_array *)sp3_type[sp3_sat_id_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)sp3_type[sp3_sat_id_array], sp3_type[sp3_sat_id]);

    sp3_type[sp3_sat_accuracy_array] = (coda_type *)coda_type_array_new(coda_format_sp3);
    coda_type_array_add_variable_dimension((coda_type_array *)sp3_type[sp3_sat_accuracy_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)sp3_type[sp3_sat_accuracy_array], sp3_type[sp3_sat_accuracy]);

    sp3_type[sp3_header] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("pos_vel");
    coda_type_record_field_set_type(field, sp3_type[sp3_pos_vel]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("datetime_start");
    coda_type_record_field_set_type(field, sp3_type[sp3_datetime_start]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("num_epochs");
    coda_type_record_field_set_type(field, sp3_type[sp3_num_epochs]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("data_used");
    coda_type_record_field_set_type(field, sp3_type[sp3_data_used]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("coordinate_sys");
    coda_type_record_field_set_type(field, sp3_type[sp3_coordinate_sys]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("orbit_type");
    coda_type_record_field_set_type(field, sp3_type[sp3_orbit_type]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("agency");
    coda_type_record_field_set_type(field, sp3_type[sp3_agency]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("gps_week");
    coda_type_record_field_set_type(field, sp3_type[sp3_gps_week]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("sec_of_week");
    coda_type_record_field_set_type(field, sp3_type[sp3_sec_of_week]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("epoch_interval");
    coda_type_record_field_set_type(field, sp3_type[sp3_epoch_interval]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("mjd_start");
    coda_type_record_field_set_type(field, sp3_type[sp3_mjd_start]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("frac_day");
    coda_type_record_field_set_type(field, sp3_type[sp3_frac_day]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("num_satellites");
    coda_type_record_field_set_type(field, sp3_type[sp3_num_satellites]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("sat_id");
    coda_type_record_field_set_type(field, sp3_type[sp3_sat_id_array]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("sat_accuracy");
    coda_type_record_field_set_type(field, sp3_type[sp3_sat_accuracy_array]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("file_type");
    coda_type_record_field_set_type(field, sp3_type[sp3_file_type]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("time_system");
    coda_type_record_field_set_type(field, sp3_type[sp3_time_system]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("base_pos_vel");
    coda_type_record_field_set_type(field, sp3_type[sp3_base_pos_vel]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);
    field = coda_type_record_field_new("base_clk_rate");
    coda_type_record_field_set_type(field, sp3_type[sp3_base_clk_rate]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_header], field);

    sp3_type[sp3_P_corr] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("x_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_x_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("y_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_y_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("z_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_z_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("clock_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_clock_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("xy_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_xy_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("xz_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_xz_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("xc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_xc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("yz_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_yz_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("yc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_yc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);
    field = coda_type_record_field_new("zc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EP_zc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_P_corr], field);

    sp3_type[sp3_pos_clk] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("vehicle_id");
    coda_type_record_field_set_type(field, sp3_type[sp3_vehicle_id]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("x_coordinate");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_x_coordinate]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("y_coordinate");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_y_coordinate]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("z_coordinate");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_z_coordinate]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("clock");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_clock]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("x_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_x_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("y_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_y_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("z_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_z_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("clock_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_clock_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("clock_event_flag");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_clock_event_flag]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("clock_pred_flag");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_clock_pred_flag]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("maneuver_flag");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_maneuver_flag]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("orbit_pred_flag");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_orbit_pred_flag]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);
    field = coda_type_record_field_new("corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_P_corr]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_pos_clk], field);

    sp3_type[sp3_pos_clk_array] = (coda_type *)coda_type_array_new(coda_format_sp3);
    coda_type_array_add_variable_dimension((coda_type_array *)sp3_type[sp3_pos_clk_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)sp3_type[sp3_pos_clk_array], sp3_type[sp3_pos_clk]);

    sp3_type[sp3_V_corr] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("xvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_xvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("yvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_yvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("zvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_zvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("clkrate_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_clkrate_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("xy_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_xy_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("xz_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_xz_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("xc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_xc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("yz_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_yz_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("yc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_yc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);
    field = coda_type_record_field_new("zc_corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_EV_zc_corr]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_V_corr], field);

    sp3_type[sp3_vel_rate] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("vehicle_id");
    coda_type_record_field_set_type(field, sp3_type[sp3_vehicle_id]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("x_velocity");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_x_velocity]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("y_velocity");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_y_velocity]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("z_velocity");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_z_velocity]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("clock_rate");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_clock_rate]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("xvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_xvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("yvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_yvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("zvel_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_zvel_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("clkrate_sdev");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_clkrate_sdev]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);
    field = coda_type_record_field_new("corr");
    coda_type_record_field_set_type(field, sp3_type[sp3_V_corr]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_vel_rate], field);

    sp3_type[sp3_vel_rate_array] = (coda_type *)coda_type_array_new(coda_format_sp3);
    coda_type_array_add_variable_dimension((coda_type_array *)sp3_type[sp3_vel_rate_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)sp3_type[sp3_vel_rate_array], sp3_type[sp3_vel_rate]);

    sp3_type[sp3_record] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, sp3_type[sp3_epoch]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_record], field);
    field = coda_type_record_field_new("pos_clk");
    coda_type_record_field_set_type(field, sp3_type[sp3_pos_clk_array]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_record], field);
    field = coda_type_record_field_new("vel_rate");
    coda_type_record_field_set_type(field, sp3_type[sp3_vel_rate_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_record], field);

    sp3_type[sp3_records] = (coda_type *)coda_type_array_new(coda_format_sp3);
    coda_type_array_add_variable_dimension((coda_type_array *)sp3_type[sp3_records], NULL);
    coda_type_array_set_base_type((coda_type_array *)sp3_type[sp3_records], sp3_type[sp3_record]);

    sp3_type[sp3_file] = (coda_type *)coda_type_record_new(coda_format_sp3);
    field = coda_type_record_field_new("header");
    coda_type_record_field_set_type(field, sp3_type[sp3_header]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_file], field);
    field = coda_type_record_field_new("record");
    coda_type_record_field_set_type(field, sp3_type[sp3_records]);
    coda_type_record_add_field((coda_type_record *)sp3_type[sp3_file], field);

    return 0;
}

void coda_sp3_done(void)
{
    int i;

    if (sp3_type == NULL)
    {
        return;
    }
    for (i = 0; i < num_sp3_types; i++)
    {
        if (sp3_type[i] != NULL)
        {
            coda_type_release(sp3_type[i]);
            sp3_type[i] = NULL;
        }
    }
    free(sp3_type);
    sp3_type = NULL;
}

static int get_line(FILE *f, char *line)
{
    long length;

    if (fgets(line, MAX_LINE_LENGTH, f) == NULL)
    {
        if (ferror(f))
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
            return -1;
        }
        /* end of file -> return empty line  */
        line[0] = '\0';
        return 0;
    }
    length = (long)strlen(line);

    /* remove 'linefeed' character if available */
    if (length > 0 && line[length - 1] == '\n')
    {
        line[length - 1] = '\0';
        length--;
    }

    /* remove 'carriage return' character if available */
    if (length > 0 && line[length - 1] == '\r')
    {
        line[length - 1] = '\0';
        length--;
    }

    return length;
}

static int read_header(ingest_info *info)
{
    coda_dynamic_type *base_type;
    coda_dynamic_type *value;
    coda_dynamic_type *array;
    char line[MAX_LINE_LENGTH];
    double double_value;
    int64_t int_value;
    char str[61];
    long linelength;
    int i;

    /* First Line */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    /* we already verified the first three characters as part of coda_open() */
    info->posvel = line[2];
    str[0] = line[2];
    str[1] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_pos_vel], NULL, info->product, str);
    coda_mem_record_add_field(info->header, "pos_vel", value, 0);

    memcpy(str, &line[3], 28);
    str[28] = '\0';
    base_type = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_datetime_start_string], NULL,
                                                         info->product, str);
    value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)sp3_type[sp3_datetime_start], NULL, base_type);
    coda_mem_record_add_field(info->header, "datetime_start", value, 0);

    if (coda_ascii_parse_int64(&line[32], 7, &int_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 32);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_num_epochs], NULL, info->product,
                                                    (int32_t)int_value);
    coda_mem_record_add_field(info->header, "num_epochs", value, 0);

    memcpy(str, &line[40], 5);
    str[5] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_data_used], NULL, info->product,
                                                     str);
    coda_mem_record_add_field(info->header, "data_used", value, 0);

    memcpy(str, &line[46], 5);
    str[5] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_coordinate_sys], NULL,
                                                     info->product, str);
    coda_mem_record_add_field(info->header, "coordinate_sys", value, 0);

    memcpy(str, &line[52], 3);
    str[3] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_orbit_type], NULL, info->product,
                                                     str);
    coda_mem_record_add_field(info->header, "orbit_type", value, 0);

    memcpy(str, &line[56], 4);
    str[4] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_agency], NULL, info->product, str);
    coda_mem_record_add_field(info->header, "agency", value, 0);

    /* Line Two */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (memcmp(line, "## ", 3) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset);
        return -1;
    }

    if (coda_ascii_parse_int64(&line[3], 4, &int_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 3);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_gps_week], NULL, info->product,
                                                    (int16_t)int_value);
    coda_mem_record_add_field(info->header, "gps_week", value, 0);

    if (coda_ascii_parse_double(&line[8], 15, &double_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 8);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_sec_of_week], NULL, info->product,
                                                     double_value);
    coda_mem_record_add_field(info->header, "sec_of_week", value, 0);

    if (coda_ascii_parse_double(&line[24], 14, &double_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 24);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_epoch_interval], NULL,
                                                     info->product, double_value);
    coda_mem_record_add_field(info->header, "epoch_interval", value, 0);

    if (coda_ascii_parse_int64(&line[39], 5, &int_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 39);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_mjd_start], NULL, info->product,
                                                    (int32_t)int_value);
    coda_mem_record_add_field(info->header, "mjd_start", value, 0);

    if (coda_ascii_parse_double(&line[45], 15, &double_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 45);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_frac_day], NULL, info->product,
                                                     double_value);
    coda_mem_record_add_field(info->header, "frac_day", value, 0);

    /* Line Three to Seven */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (memcmp(line, "+   ", 4) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset);
        return -1;
    }

    if (coda_ascii_parse_int64(&line[4], 2, &int_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)sp3_type[sp3_num_satellites], NULL,
                                                    info->product, (uint8_t)int_value);
    coda_mem_record_add_field(info->header, "num_satellites", value, 0);
    info->num_satellites = (int)int_value;

    array = (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)sp3_type[sp3_sat_id_array], NULL);
    for (i = 0; i < 5 * 17; i++)
    {
        if (i % 17 == 0 && i > 0)
        {
            /* read next line */
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                coda_dynamic_type_delete(array);
                return -1;
            }
            if (linelength < 60)
            {
                coda_dynamic_type_delete(array);
                coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
            if (memcmp(line, "+        ", 9) != 0)
            {
                coda_dynamic_type_delete(array);
                coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset);
                return -1;
            }
        }
        if (i < info->num_satellites)
        {
            memcpy(str, &line[9 + (i % 17) * 3], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_sat_id], NULL,
                                                             info->product, str);
            coda_mem_array_add_element((coda_mem_array *)array, value);
        }
    }
    coda_mem_record_add_field(info->header, "sat_id", array, 0);

    /* Line Eight to Twelve */
    array = (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)sp3_type[sp3_sat_accuracy_array], NULL);
    for (i = 0; i < 5 * 17; i++)
    {
        if (i % 17 == 0)
        {
            /* read next line */
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                coda_dynamic_type_delete(array);
                return -1;
            }
            if (linelength < 60)
            {
                coda_dynamic_type_delete(array);
                coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
            if (memcmp(line, "++       ", 9) != 0)
            {
                coda_dynamic_type_delete(array);
                coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset);
                return -1;
            }
        }
        if (i < info->num_satellites)
        {
            if (coda_ascii_parse_int64(&line[9 + (i % 17) * 3], 3, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 9 +
                                       (i % 17) * 3);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_sat_accuracy], NULL,
                                                            info->product, (int16_t)int_value);
            coda_mem_array_add_element((coda_mem_array *)array, value);
        }
    }
    coda_mem_record_add_field(info->header, "sat_accuracy", array, 0);

    /* Line Thirteen */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (memcmp(line, "%c ", 3) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset);
        return -1;
    }

    memcpy(str, &line[3], 2);
    str[2] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_file_type], NULL, info->product,
                                                     str);
    coda_mem_record_add_field(info->header, "file_type", value, 0);

    memcpy(str, &line[9], 3);
    str[3] = '\0';
    value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_time_system], NULL, info->product,
                                                     str);
    coda_mem_record_add_field(info->header, "time_system", value, 0);

    /* Line Fourteen */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (memcmp(line, "%c ", 3) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset);
        return -1;
    }

    /* Line Fifteen */
    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 60)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (memcmp(line, "%f ", 3) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid lead characters for line (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset);
        return -1;
    }

    if (coda_ascii_parse_double(&line[3], 10, &double_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 3);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_base_pos_vel], NULL,
                                                     info->product, double_value);
    coda_mem_record_add_field(info->header, "base_pos_vel", value, 0);

    if (coda_ascii_parse_double(&line[14], 12, &double_value, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 14);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_base_clk_rate], NULL,
                                                     info->product, double_value);
    coda_mem_record_add_field(info->header, "base_clk_rate", value, 0);

    /* Line Sixteen to Twenty two */
    for (i = 0; i < 7; i++)
    {
        info->offset = ftell(info->f);
        info->linenumber++;
        linelength = get_line(info->f, line);
        if (linelength < 0)
        {
            return -1;
        }
    }

    return 0;
}

static int read_records(ingest_info *info)
{
    coda_dynamic_type *base_type;
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    double double_value;
    int64_t int_value;
    char str[61];
    long linelength;

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (memcmp(line, "EOF", 3) != 0)
    {
        if (line[0] == '*')
        {
            if (info->record != NULL)
            {
                coda_mem_record_add_field(info->record, "pos_clk", (coda_dynamic_type *)info->pos_clk_array, 0);
                info->pos_clk_array = NULL;
                if (info->vel_rate_array != NULL)
                {
                    coda_mem_record_add_field(info->record, "vel_rate", (coda_dynamic_type *)info->vel_rate_array, 0);
                    info->vel_rate_array = NULL;
                }
                coda_mem_array_add_element(info->records, (coda_dynamic_type *)info->record);
                info->record = NULL;
            }
            info->pos_clk_array = coda_mem_array_new((coda_type_array *)sp3_type[sp3_pos_clk_array], NULL);
            if (info->posvel == 'V')
            {
                info->vel_rate_array = coda_mem_array_new((coda_type_array *)sp3_type[sp3_vel_rate_array], NULL);
            }
            info->record = coda_mem_record_new((coda_type_record *)sp3_type[sp3_record], NULL);
            if (linelength < 31)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
            memcpy(str, &line[3], 28);
            str[28] = '\0';
            base_type = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_epoch_string], NULL,
                                                                 info->product, str);
            value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)sp3_type[sp3_epoch], NULL, base_type);
            coda_mem_record_add_field(info->record, "epoch", value, 0);
        }
        else if (line[0] == 'P')
        {
            if (info->pos_clk_array == NULL)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "Position and Clock Record without Epoch Header Record "
                               "(line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            info->pos_clk = coda_mem_record_new((coda_type_record *)sp3_type[sp3_pos_clk], NULL);

            if (linelength < 60)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }

            memcpy(str, &line[1], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_vehicle_id], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->pos_clk, "vehicle_id", value, 0);

            if (coda_ascii_parse_double(&line[4], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_P_x_coordinate], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->pos_clk, "x_coordinate", value, 0);

            if (coda_ascii_parse_double(&line[18], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 18);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_P_y_coordinate], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->pos_clk, "y_coordinate", value, 0);

            if (coda_ascii_parse_double(&line[32], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 32);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_P_z_coordinate], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->pos_clk, "z_coordinate", value, 0);

            if (coda_ascii_parse_double(&line[46], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 46);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_P_clock], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->pos_clk, "clock", value, 0);

            if (linelength < 64 || memcmp(&line[61], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[61], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 61);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_P_x_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->pos_clk, "x_sdev", value, 0);

            if (linelength < 66 || memcmp(&line[64], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[64], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 64);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_P_y_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->pos_clk, "y_sdev", value, 0);

            if (linelength < 69 || memcmp(&line[67], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[67], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 67);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_P_z_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->pos_clk, "z_sdev", value, 0);

            if (linelength < 73 || memcmp(&line[70], "   ", 3) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[70], 3, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 70);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_P_clock_sdev], NULL,
                                                            info->product, (int16_t)int_value);
            coda_mem_record_add_field(info->pos_clk, "clock_sdev", value, 0);

            str[0] = linelength < 75 ? ' ' : line[74];
            str[1] = '\0';
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_P_clock_event_flag], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->pos_clk, "clock_event_flag", value, 0);

            str[0] = linelength < 76 ? ' ' : line[75];
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_P_clock_pred_flag], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->pos_clk, "clock_pred_flag", value, 0);

            str[0] = linelength < 79 ? ' ' : line[78];
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_P_maneuver_flag], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->pos_clk, "maneuver_flag", value, 0);

            str[0] = linelength < 80 ? ' ' : line[79];
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_P_orbit_pred_flag], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->pos_clk, "orbit_pred_flag", value, 0);
        }
        else if (line[0] == 'V')
        {
            if (info->posvel != 'V')
            {
                coda_set_error(CODA_ERROR_FILE_READ, "Velocity and Rate Record not allowed due to header Position/"
                               "Velocity Flag value (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            if (info->vel_rate_array == NULL)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "Velocity and Rate Record without Epoch Header Record "
                               "(line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            info->vel_rate = coda_mem_record_new((coda_type_record *)sp3_type[sp3_vel_rate], NULL);

            if (linelength < 60)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }

            memcpy(str, &line[1], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)sp3_type[sp3_vehicle_id], NULL,
                                                             info->product, str);
            coda_mem_record_add_field(info->vel_rate, "vehicle_id", value, 0);

            if (coda_ascii_parse_double(&line[4], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_V_x_velocity], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->vel_rate, "x_velocity", value, 0);

            if (coda_ascii_parse_double(&line[18], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 18);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_V_y_velocity], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->vel_rate, "y_velocity", value, 0);

            if (coda_ascii_parse_double(&line[32], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 32);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_V_z_velocity], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->vel_rate, "z_velocity", value, 0);

            if (coda_ascii_parse_double(&line[46], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 46);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)sp3_type[sp3_V_clock_rate], NULL,
                                                             info->product, double_value);
            coda_mem_record_add_field(info->vel_rate, "clock_rate", value, 0);

            if (linelength < 63 || memcmp(&line[61], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[61], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 61);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_V_xvel_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->vel_rate, "xvel_sdev", value, 0);

            if (linelength < 66 || memcmp(&line[64], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[64], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 64);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_V_yvel_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->vel_rate, "yvel_sdev", value, 0);

            if (linelength < 69 || memcmp(&line[67], "  ", 2) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[67], 2, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 67);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)sp3_type[sp3_V_zvel_sdev], NULL,
                                                           info->product, (int8_t)int_value);
            coda_mem_record_add_field(info->vel_rate, "zvel_sdev", value, 0);

            if (linelength < 73 || memcmp(&line[70], "   ", 3) == 0)
            {
                int_value = 0;
            }
            else if (coda_ascii_parse_int64(&line[70], 3, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 70);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_V_clkrate_sdev], NULL,
                                                            info->product, (int16_t)int_value);
            coda_mem_record_add_field(info->vel_rate, "clkrate_sdev", value, 0);
        }
        else
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid line (line: %ld, byte offset: %ld)", info->linenumber,
                           info->offset);
            return -1;
        }

        info->offset = ftell(info->f);
        info->linenumber++;
        linelength = get_line(info->f, line);
        if (linelength < 0)
        {
            return -1;
        }

        if (line[0] == 'E' && (line[1] == 'P' || line[1] == 'V'))
        {
            if (line[1] == 'P')
            {
                if (info->pos_clk == NULL)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "Position and Clock Correlation Record without Position and "
                                   "Clock Record (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                info->corr = coda_mem_record_new((coda_type_record *)sp3_type[sp3_P_corr], NULL);

                if (linelength < 8 || memcmp(&line[4], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[4], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EP_x_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "x_sdev", value, 0);

                if (linelength < 13 || memcmp(&line[9], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[9], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 9);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EP_y_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "y_sdev", value, 0);

                if (linelength < 18 || memcmp(&line[14], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[14], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 14);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EP_z_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "z_sdev", value, 0);

                if (linelength < 26 || memcmp(&line[19], "       ", 7) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[19], 7, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 19);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_clock_sdev], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "clock_sdev", value, 0);

                if (linelength < 35 || memcmp(&line[27], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[27], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 27);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_xy_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xy_corr", value, 0);

                if (linelength < 44 || memcmp(&line[36], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[36], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 36);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_xz_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xz_corr", value, 0);

                if (linelength < 53 || memcmp(&line[45], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[45], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 45);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_xc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xc_corr", value, 0);

                if (linelength < 62 || memcmp(&line[54], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[54], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 54);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_yz_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "yz_corr", value, 0);

                if (linelength < 71 || memcmp(&line[63], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[63], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 63);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_yc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "yc_corr", value, 0);

                if (linelength < 80 || memcmp(&line[72], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[72], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 72);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EP_zc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "zc_corr", value, 0);

                coda_mem_record_add_field(info->pos_clk, "corr", (coda_dynamic_type *)info->corr, 0);
                info->corr = NULL;
            }
            else
            {
                if (info->vel_rate == NULL)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "Velocity and Rate Correlation Record without Velocity and "
                                   "Rate Record (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                info->corr = coda_mem_record_new((coda_type_record *)sp3_type[sp3_V_corr], NULL);

                if (linelength < 8 || memcmp(&line[4], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[4], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EV_xvel_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "xvel_sdev", value, 0);

                if (linelength < 13 || memcmp(&line[9], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[9], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 9);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EV_yvel_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "yvel_sdev", value, 0);

                if (linelength < 18 || memcmp(&line[14], "    ", 4) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[14], 4, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 14);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EV_zvel_sdev], NULL,
                                                                info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "zvel_sdev", value, 0);

                if (linelength < 28 || memcmp(&line[19], "       ", 7) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[19], 7, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 19);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)sp3_type[sp3_EV_clkrate_sdev],
                                                                NULL, info->product, (int16_t)int_value);
                coda_mem_record_add_field(info->corr, "clkrate_sdev", value, 0);

                if (linelength < 35 || memcmp(&line[27], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[27], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 27);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_xy_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xy_corr", value, 0);

                if (linelength < 44 || memcmp(&line[36], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[36], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 36);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_xz_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xz_corr", value, 0);

                if (linelength < 53 || memcmp(&line[45], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[45], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 45);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_xc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "xc_corr", value, 0);

                if (linelength < 62 || memcmp(&line[54], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[54], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 54);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_yz_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "yz_corr", value, 0);

                if (linelength < 71 || memcmp(&line[63], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[63], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 63);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_yc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "yc_corr", value, 0);

                if (linelength < 80 || memcmp(&line[72], "        ", 8) == 0)
                {
                    int_value = 0;
                }
                else if (coda_ascii_parse_int64(&line[72], 8, &int_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 72);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)sp3_type[sp3_EV_zc_corr], NULL,
                                                                info->product, (int32_t)int_value);
                coda_mem_record_add_field(info->corr, "zc_corr", value, 0);

                coda_mem_record_add_field(info->vel_rate, "corr", (coda_dynamic_type *)info->corr, 0);
                info->corr = NULL;
            }
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                return -1;
            }
        }

        if (info->pos_clk != NULL)
        {
            coda_mem_array_add_element(info->pos_clk_array, (coda_dynamic_type *)info->pos_clk);
            info->pos_clk = NULL;
        }
        if (info->vel_rate != NULL)
        {
            coda_mem_array_add_element(info->vel_rate_array, (coda_dynamic_type *)info->vel_rate);
            info->vel_rate = NULL;
        }
    }

    if (info->record != NULL)
    {
        coda_mem_record_add_field(info->record, "pos_clk", (coda_dynamic_type *)info->pos_clk_array, 0);
        info->pos_clk_array = NULL;
        coda_mem_record_add_field(info->record, "vel_rate", (coda_dynamic_type *)info->vel_rate_array, 0);
        info->vel_rate_array = NULL;
        coda_mem_array_add_element(info->records, (coda_dynamic_type *)info->record);
        info->record = NULL;
    }

    return 0;
}

static int read_file(coda_product *product)
{
    coda_mem_record *root_type = NULL;
    ingest_info info;

    ingest_info_init(&info);
    info.product = product;

    info.f = fopen(product->filename, "r");
    if (info.f == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", product->filename);
        return -1;
    }

    info.header = coda_mem_record_new((coda_type_record *)sp3_type[sp3_header], NULL);
    info.records = coda_mem_array_new((coda_type_array *)sp3_type[sp3_records], NULL);

    if (read_header(&info) != 0)
    {
        ingest_info_cleanup(&info);
        return -1;
    }

    if (read_records(&info) != 0)
    {
        ingest_info_cleanup(&info);
        return -1;
    }

    /* create root record */
    root_type = coda_mem_record_new((coda_type_record *)sp3_type[sp3_file], NULL);
    coda_mem_record_add_field(root_type, "header", (coda_dynamic_type *)info.header, 0);
    info.header = NULL;
    coda_mem_record_add_field(root_type, "record", (coda_dynamic_type *)info.records, 0);
    info.records = NULL;

    product->root_type = (coda_dynamic_type *)root_type;

    ingest_info_cleanup(&info);

    return 0;
}

int coda_sp3_reopen(coda_product **product)
{
    coda_product *product_file;

    if (sp3_init() != 0)
    {
        coda_close(*product);
        return -1;
    }

    product_file = (coda_product *)malloc(sizeof(coda_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_sp3;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_close(product_file);
        coda_close(*product);
        return -1;
    }

    coda_close(*product);

    /* create root type */
    if (read_file(product_file) != 0)
    {
        coda_close(product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_sp3_close(coda_product *product)
{
    if (product->root_type != NULL)
    {
        coda_dynamic_type_delete(product->root_type);
    }
    if (product->filename != NULL)
    {
        free(product->filename);
    }
    if (product->mem_ptr != NULL)
    {
        free(product->mem_ptr);
    }

    free(product);

    return 0;
}

int coda_sp3_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for memory backend */

    return 0;
}
