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

#include "coda-rinex.h"
#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-mem-internal.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1000

enum
{
    rinex_format_version,
    rinex_file_type,
    rinex_satellite_system,

    rinex_program,
    rinex_run_by,
    rinex_datetime,
    rinex_datetime_string,
    rinex_datetime_time_zone,
    rinex_marker_name,
    rinex_marker_number,
    rinex_marker_type,
    rinex_observer,
    rinex_agency,
    rinex_receiver_number,
    rinex_receiver_type,
    rinex_receiver_version,
    rinex_antenna_number,
    rinex_antenna_type,
    rinex_approx_position_x,
    rinex_approx_position_y,
    rinex_approx_position_z,
    rinex_antenna_delta_h,
    rinex_antenna_delta_e,
    rinex_antenna_delta_n,
    rinex_antenna_delta_x,
    rinex_antenna_delta_y,
    rinex_antenna_delta_z,
    rinex_sys_code,
    rinex_sys_num_obs_types,
    rinex_sys_descriptor,
    rinex_sys_descriptor_array,
    rinex_sys,
    rinex_sys_array,
    rinex_signal_strength_unit,
    rinex_obs_interval,
    rinex_time_of_first_obs,
    rinex_time_of_first_obs_string,
    rinex_time_of_last_obs,
    rinex_time_of_last_obs_string,
    rinex_time_of_obs_time_zone,
    rinex_rcv_clock_offs_appl,
    rinex_leap_seconds,
    rinex_num_satellites,
    rinex_time_system_id,

    rinex_epoch_string,
    rinex_obs_epoch,
    rinex_obs_epoch_flag,
    rinex_receiver_clock_offset,
    rinex_satellite_number,
    rinex_observation,
    rinex_lli,
    rinex_signal_strength,
    rinex_observation_record,

    rinex_obs_header,

    rinex_ionospheric_corr_type,
    rinex_ionospheric_corr_parameter,
    rinex_ionospheric_corr_parameter_array,
    rinex_ionospheric_corr,
    rinex_ionospheric_corr_array,
    rinex_time_system_corr_type,
    rinex_time_system_corr_a0,
    rinex_time_system_corr_a1,
    rinex_time_system_corr_t,
    rinex_time_system_corr_w,
    rinex_time_system_corr_s,
    rinex_time_system_corr_u,
    rinex_time_system_corr,
    rinex_time_system_corr_array,

    rinex_nav_epoch,
    rinex_nav_sv_clock_bias,
    rinex_nav_sv_clock_drift,
    rinex_nav_sv_clock_drift_rate,
    rinex_nav_iode,
    rinex_nav_crs,
    rinex_nav_delta_n,
    rinex_nav_m0,
    rinex_nav_cuc,
    rinex_nav_e,
    rinex_nav_cus,
    rinex_nav_sqrt_a,
    rinex_nav_toe,
    rinex_nav_cic,
    rinex_nav_omega0,
    rinex_nav_cis,
    rinex_nav_i0,
    rinex_nav_crc,
    rinex_nav_omega,
    rinex_nav_omega_dot,
    rinex_nav_idot,
    rinex_nav_l2_codes,
    rinex_nav_gps_week,
    rinex_nav_l2_p_data_flag,
    rinex_nav_sv_accuracy,
    rinex_nav_sv_health_gps,
    rinex_nav_tgd,
    rinex_nav_iodc,
    rinex_nav_transmission_time_gps,
    rinex_nav_fit_interval,

    rinex_nav_iodnav,
    rinex_nav_data_sources,
    rinex_nav_gal_week,
    rinex_nav_sisa,
    rinex_nav_sv_health_galileo,
    rinex_nav_bgd_e5a_e1,
    rinex_nav_bgd_e5b_e1,
    rinex_nav_transmission_time_galileo,

    rinex_nav_sv_rel_freq_bias,
    rinex_nav_msg_frame_time,
    rinex_nav_sat_pos_x,
    rinex_nav_sat_pos_y,
    rinex_nav_sat_pos_z,
    rinex_nav_sat_vel_x,
    rinex_nav_sat_vel_y,
    rinex_nav_sat_vel_z,
    rinex_nav_sat_acc_x,
    rinex_nav_sat_acc_y,
    rinex_nav_sat_acc_z,
    rinex_nav_sat_health,
    rinex_nav_sat_frequency_number,
    rinex_nav_age_of_oper_info,

    rinex_nav_transmission_time_sbas,
    rinex_nav_sat_accuracy_code,
    rinex_nav_iodn,

    rinex_nav_gps_record,
    rinex_nav_glonass_record,
    rinex_nav_galileo_record,
    rinex_nav_sbas_record,

    rinex_nav_header,
    rinex_nav_gps_array,
    rinex_nav_glonass_array,
    rinex_nav_galileo_array,
    rinex_nav_sbas_array,
    rinex_nav_file,

    rinex_clk_type,
    rinex_clk_name,
    rinex_clk_epoch,
    rinex_clk_bias,
    rinex_clk_bias_sigma,
    rinex_clk_rate,
    rinex_clk_rate_sigma,
    rinex_clk_acceleration,
    rinex_clk_acceleration_sigma,

    rinex_clk_header,
    rinex_clk_record,

    num_rinex_types
};

static coda_type **rinex_type = NULL;

typedef struct satellite_info_struct
{
    /* Observation specific */
    int num_observables;
    char **observable;
    coda_type_record *sat_obs_definition;       /* definition for /record[]/<sys>[] */
    coda_type_array *sat_obs_array_definition;  /* definition for /record[]/<sys> */
    coda_mem_array *sat_obs_array;      /* actual data for /record[]/<sys> */
    /* Navigation specific */
    coda_mem_array *records;    /* actual data for /<sys> */
} satellite_info;

typedef struct ingest_info_struct
{
    FILE *f;
    coda_mem_record *header;    /* actual data for /header */
    satellite_info gps;
    satellite_info glonass;
    satellite_info galileo;
    satellite_info sbas;
    double format_version;
    char file_type;
    char satellite_system;
    long linenumber;
    long offset;
    /* Observation specific */
    coda_type_record *epoch_record_definition;  /* definition for /record[] */
    coda_mem_record *epoch_record;      /* actual data for /record[] */
    /* Observation and Clock specific */
    coda_mem_array *sys_array;  /* actuall data for /header/sys */
    coda_mem_array *records;    /* actual data for /record */
    /* Navigation specific */
    coda_mem_array *ionospheric_corr_array;     /* actuall data for /header/ionospheric_corr */
    coda_mem_array *time_system_corr_array;     /* actuall data for /header/time_system_corr */
} ingest_info;

static void satellite_info_cleanup(satellite_info *info)
{
    int i;

    if (info->observable != NULL)
    {
        for (i = 0; i < info->num_observables; i++)
        {
            if (info->observable[i] != NULL)
            {
                free(info->observable[i]);
            }
        }
        free(info->observable);
    }
    if (info->sat_obs_definition != NULL)
    {
        coda_type_release((coda_type *)info->sat_obs_definition);
    }
    if (info->sat_obs_array_definition != NULL)
    {
        coda_type_release((coda_type *)info->sat_obs_array_definition);
    }
    if (info->sat_obs_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->sat_obs_array);
    }
    if (info->records != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->records);
    }
}

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
    satellite_info_cleanup(&info->gps);
    satellite_info_cleanup(&info->glonass);
    satellite_info_cleanup(&info->galileo);
    satellite_info_cleanup(&info->sbas);
    if (info->epoch_record_definition != NULL)
    {
        coda_type_release((coda_type *)info->epoch_record_definition);
    }
    if (info->sys_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->sys_array);
    }
    if (info->records != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->records);
    }
    if (info->epoch_record != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->epoch_record);
    }
    if (info->ionospheric_corr_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->ionospheric_corr_array);
    }
    if (info->time_system_corr_array != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)info->time_system_corr_array);
    }
}

static void satellite_info_init(satellite_info *info)
{
    info->num_observables = 0;
    info->observable = NULL;
    info->sat_obs_definition = NULL;
    info->sat_obs_array_definition = NULL;
    info->sat_obs_array = NULL;
    info->records = NULL;
}

static void ingest_info_init(ingest_info *info)
{
    info->f = NULL;
    info->header = NULL;
    satellite_info_init(&info->gps);
    satellite_info_init(&info->glonass);
    satellite_info_init(&info->galileo);
    satellite_info_init(&info->sbas);
    info->format_version = 0;
    info->file_type = ' ';
    info->satellite_system = ' ';
    info->linenumber = 0;
    info->offset = 0;
    info->epoch_record_definition = NULL;
    info->sys_array = NULL;
    info->records = NULL;
    info->epoch_record = NULL;
    info->ionospheric_corr_array = NULL;
    info->time_system_corr_array = NULL;
}

static int rinex_init(void)
{
    coda_type_record_field *field;
    int i;

    if (rinex_type != NULL)
    {
        return 0;
    }

    rinex_type = malloc(num_rinex_types * sizeof(coda_type *));
    if (rinex_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_rinex_types * sizeof(coda_type *), __FILE__, __LINE__);
        return -1;
    }
    for (i = 0; i < num_rinex_types; i++)
    {
        rinex_type[i] = NULL;
    }

    rinex_type[rinex_format_version] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_format_version], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_format_version], "Format version");

    rinex_type[rinex_file_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_byte_size(rinex_type[rinex_file_type], 1);
    coda_type_set_read_type(rinex_type[rinex_file_type], coda_native_type_char);
    coda_type_set_description(rinex_type[rinex_file_type], "File type: O for Observation Data, N for Navigation Data, "
                              "C for Clock Data, M for Meteorological Data");

    rinex_type[rinex_satellite_system] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_byte_size(rinex_type[rinex_satellite_system], 1);
    coda_type_set_read_type(rinex_type[rinex_satellite_system], coda_native_type_char);
    coda_type_set_description(rinex_type[rinex_satellite_system],
                              "Satellite System: G = GPS, R = GLONASS, E = Galileo, S = SBAS, M = Mixed");

    rinex_type[rinex_program] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_program], "Name of program creating current file");

    rinex_type[rinex_run_by] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_run_by], "Name of agency creating current file");

    rinex_type[rinex_datetime_string] = (coda_type *)coda_type_text_new(coda_format_rinex);

    rinex_type[rinex_datetime] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_datetime], rinex_type[rinex_datetime_string]);
    coda_type_set_description(rinex_type[rinex_datetime], "Date/time of file creation");

    rinex_type[rinex_datetime_time_zone] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_datetime_time_zone], "Code for file creation timezone: UTC recommended, "
                              "LCL = local time with unknown local time system code");

    rinex_type[rinex_marker_name] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_marker_name], "Name of antenna marker");

    rinex_type[rinex_marker_number] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_marker_number], "Number of antenna marker");

    rinex_type[rinex_marker_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_marker_type], "Type of the marker");

    rinex_type[rinex_observer] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_observer], "Name of observer");

    rinex_type[rinex_agency] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_agency], "Name of agency of observer");

    rinex_type[rinex_receiver_number] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_receiver_number], "Receiver number");

    rinex_type[rinex_receiver_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_receiver_type], "Receiver type");

    rinex_type[rinex_receiver_version] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_receiver_version], "Receiver version (e.g. Internal Software Version)");

    rinex_type[rinex_antenna_number] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_antenna_number], "Antenna number");

    rinex_type[rinex_antenna_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_antenna_type], "Antenna type");

    rinex_type[rinex_approx_position_x] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_approx_position_x], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_approx_position_x], "Geocentric approximate marker position - X");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_approx_position_x], "m");

    rinex_type[rinex_approx_position_y] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_approx_position_y], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_approx_position_y], "Geocentric approximate marker position - Y");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_approx_position_y], "m");

    rinex_type[rinex_approx_position_z] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_approx_position_z], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_approx_position_z], "Geocentric approximate marker position - Z");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_approx_position_z], "m");

    rinex_type[rinex_antenna_delta_h] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_antenna_delta_h], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_antenna_delta_h],
                              "Height of the antenna reference point (ARP) above the marker");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_antenna_delta_h], "m");

    rinex_type[rinex_antenna_delta_e] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_antenna_delta_e], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_antenna_delta_e],
                              "Horizontal eccentricity of ARP relative to the marker (east)");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_antenna_delta_e], "m");

    rinex_type[rinex_antenna_delta_n] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_antenna_delta_n], coda_native_type_float);
    coda_type_set_description(rinex_type[rinex_antenna_delta_n],
                              "Horizontal eccentricity of ARP relative to the marker (north)");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_antenna_delta_n], "m");

    rinex_type[rinex_sys_code] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_byte_size(rinex_type[rinex_sys_code], 1);
    coda_type_set_read_type(rinex_type[rinex_sys_code], coda_native_type_char);
    coda_type_set_description(rinex_type[rinex_sys_code], "Satellite system code (G/R/E/S)");

    rinex_type[rinex_sys_num_obs_types] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_sys_num_obs_types], coda_native_type_int16);
    coda_type_set_description(rinex_type[rinex_sys_num_obs_types],
                              "Number of different observation types for the specified satellite system");

    rinex_type[rinex_sys_descriptor] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_sys_descriptor],
                              "The following observation descriptors are defined in RINEX Version 3.00: "
                              "Type: C = Code / Pseudorange, L = Phase, D = Doppler, S = Raw signal strength, "
                              "I = Ionosphere phase delay, X = Receiver channel numbers. Band: 1 = L1 (GPS,SBAS), "
                              "G1 (GLO), E2-L1-E1 (GAL), 2 = L2 (GPS), G2 (GLO), 5 = L5 (GPS,SBAS), E5a (GAL), "
                              "6 = E6 (GAL), 7 = E5b (GAL), 8 = E5a+b (GAL), 0 for type X (all). Attribute: "
                              "P = P code-based (GPS,GLO), C = C code-based (SBAS,GPS,GLO), Y = Y code-based (GPS), "
                              "M = M code-based (GPS), N = codeless (GPS), A = A channel (GAL), B = B channel (GAL), "
                              "C = C channel (GAL), I = I channel (GPS,GAL), Q = Q channel (GPS,GAL), "
                              "S = M channel (L2C GPS), L = L channel (L2C GPS), X = B+C channels (GAL), "
                              "I+Q channels (GPS,GAL), M+L channels (GPS), W = based on Z-tracking (GPS), "
                              "Z = A+B+C channels (GAL), blank : for types I and X or unknown tracking mode (all)");

    rinex_type[rinex_sys_descriptor_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_sys_descriptor_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_sys_descriptor_array],
                                  rinex_type[rinex_sys_descriptor]);

    rinex_type[rinex_sys] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("code");
    coda_type_record_field_set_type(field, rinex_type[rinex_sys_code]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_sys], field);
    field = coda_type_record_field_new("num_obs_types");
    coda_type_record_field_set_type(field, rinex_type[rinex_sys_num_obs_types]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_sys], field);
    field = coda_type_record_field_new("descriptor");
    coda_type_record_field_set_type(field, rinex_type[rinex_sys_descriptor_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_sys], field);

    rinex_type[rinex_sys_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_sys_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_sys_array], rinex_type[rinex_sys]);

    rinex_type[rinex_signal_strength_unit] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_signal_strength_unit], "Unit of the signal strength observations Snn "
                              "(if present). e.g. DBHZ: S/N given in dbHz");

    rinex_type[rinex_obs_interval] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_obs_interval], "Observation interval in seconds");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_obs_interval], "s");

    rinex_type[rinex_time_of_first_obs_string] = (coda_type *)coda_type_text_new(coda_format_rinex);

    rinex_type[rinex_time_of_first_obs] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_time_of_first_obs],
                                 rinex_type[rinex_time_of_first_obs_string]);
    coda_type_set_description(rinex_type[rinex_time_of_first_obs], "Time of first observation record");

    rinex_type[rinex_time_of_last_obs_string] = (coda_type *)coda_type_text_new(coda_format_rinex);

    rinex_type[rinex_time_of_last_obs] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_time_of_last_obs],
                                 rinex_type[rinex_time_of_last_obs_string]);
    coda_type_set_description(rinex_type[rinex_time_of_last_obs], "Time of last observation record");

    rinex_type[rinex_time_of_obs_time_zone] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_time_of_obs_time_zone], "Time system: GPS (=GPS time system), "
                              "GLO (=UTC time system), GAL (=Galileo System Time)");

    rinex_type[rinex_rcv_clock_offs_appl] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_rcv_clock_offs_appl], coda_native_type_uint8);
    coda_type_set_description(rinex_type[rinex_rcv_clock_offs_appl], "Epoch, code, and phase are corrected by "
                              "applying the realtime-derived receiver clock offset: 1=yes, 0=no; default: 0=no. "
                              "Record required if clock offsets are reported in the EPOCH/SAT records");

    rinex_type[rinex_leap_seconds] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_leap_seconds], coda_native_type_int32);
    coda_type_set_description(rinex_type[rinex_leap_seconds], "Number of leap seconds since 6-Jan-1980 as transmitted "
                              "by the GPS almanac. Recommended for mixed GLONASS files");

    rinex_type[rinex_num_satellites] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_num_satellites], coda_native_type_uint16);
    coda_type_set_description(rinex_type[rinex_num_satellites],
                              "Number of satellites, for which observations are stored in the file");

    rinex_type[rinex_time_system_id] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_time_system_id], "Time system used for time tags: "
                              "'GPS' = GPS system time -> steered to (TAI - 19 s), "
                              "'GLO' = GLONASS system time -> steered to UTC, "
                              "'GAL' = Galileo system time -> steered to GPS time, "
                              "'UTC' = Coordinated Universal Time, 'TAI' = International Atomic Time. "
                              "Defaults: 'GPS' for pure GPS files, 'GLO' for pure GLONASS files, "
                              "'GAL' for pure Galileo files");

    rinex_type[rinex_epoch_string] = (coda_type *)coda_type_text_new(coda_format_rinex);

    rinex_type[rinex_obs_epoch] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_obs_epoch], rinex_type[rinex_epoch_string]);
    coda_type_set_description(rinex_type[rinex_obs_epoch], "Epoch of observation");

    rinex_type[rinex_obs_epoch_flag] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_byte_size(rinex_type[rinex_obs_epoch_flag], 1);
    coda_type_set_read_type(rinex_type[rinex_obs_epoch_flag], coda_native_type_char);
    coda_type_set_description(rinex_type[rinex_obs_epoch_flag], "0: OK, 1: power failure between previous and current "
                              "epoch, >1: Special event");

    rinex_type[rinex_receiver_clock_offset] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_receiver_clock_offset], "Receiver clock offset");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_receiver_clock_offset], "s");

    rinex_type[rinex_satellite_number] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_satellite_number], coda_native_type_uint8);
    coda_type_set_description(rinex_type[rinex_satellite_number],
                              "Satellite number (for the applicable satellite system)");

    rinex_type[rinex_observation] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_observation],
                              "Observations: Definition see /header/obs_type/descriptor. Missing observations are "
                              "written as 0.0 or blanks. Phase values overflowing the fixed format have to be clipped "
                              "into the valid interval (e.g add or subtract 10**9), set LLI indicator.");

    rinex_type[rinex_lli] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_lli], coda_native_type_uint8);
    coda_type_set_description(rinex_type[rinex_lli], "Loss of lock indicator (LLI). 0 or blank: OK or not known. "
                              "Bit 0 set : Lost lock between previous and current observation: Cycle slip possible. "
                              "For phase observations only. Bit 1 set : Half-cycle ambiguity/slip possible. Software "
                              "not capable of handling half cycles should skip this observation. Valid for the current "
                              "epoch only.");

    rinex_type[rinex_signal_strength] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_signal_strength], coda_native_type_uint8);
    coda_type_set_description(rinex_type[rinex_signal_strength], "Signal strength projected into interval 1-9: "
                              "1: minimum possible signal strength. 5: average S/N ratio. "
                              "9: maximum possible signal strength. 0 or blank: not known, don't care");

    rinex_type[rinex_observation_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("observation");
    coda_type_record_field_set_type(field, rinex_type[rinex_observation]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_observation_record], field);
    field = coda_type_record_field_new("lli");
    coda_type_record_field_set_type(field, rinex_type[rinex_lli]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_observation_record], field);
    field = coda_type_record_field_new("signal_strength");
    coda_type_record_field_set_type(field, rinex_type[rinex_signal_strength]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_observation_record], field);

    rinex_type[rinex_obs_header] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("format_version");
    coda_type_record_field_set_type(field, rinex_type[rinex_format_version]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("file_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_file_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("satellite_system");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_system]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("program");
    coda_type_record_field_set_type(field, rinex_type[rinex_program]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("run_by");
    coda_type_record_field_set_type(field, rinex_type[rinex_run_by]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("datetime");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("datetime_time_zone");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime_time_zone]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("marker_name");
    coda_type_record_field_set_type(field, rinex_type[rinex_marker_name]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("marker_number");
    coda_type_record_field_set_type(field, rinex_type[rinex_marker_number]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("marker_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_marker_type]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("observer");
    coda_type_record_field_set_type(field, rinex_type[rinex_observer]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("agency");
    coda_type_record_field_set_type(field, rinex_type[rinex_agency]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("receiver_number");
    coda_type_record_field_set_type(field, rinex_type[rinex_receiver_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("receiver_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_receiver_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("receiver_version");
    coda_type_record_field_set_type(field, rinex_type[rinex_receiver_version]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("antenna_number");
    coda_type_record_field_set_type(field, rinex_type[rinex_antenna_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("antenna_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_antenna_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("approx_position_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_approx_position_x]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("approx_position_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_approx_position_y]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("approx_position_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_approx_position_z]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("antenna_delta_h");
    coda_type_record_field_set_type(field, rinex_type[rinex_antenna_delta_h]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("antenna_delta_e");
    coda_type_record_field_set_type(field, rinex_type[rinex_antenna_delta_e]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("antenna_delta_n");
    coda_type_record_field_set_type(field, rinex_type[rinex_antenna_delta_n]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("sys");
    coda_type_record_field_set_type(field, rinex_type[rinex_sys_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("signal_strength_unit");
    coda_type_record_field_set_type(field, rinex_type[rinex_signal_strength_unit]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("obs_interval");
    coda_type_record_field_set_type(field, rinex_type[rinex_obs_interval]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("time_of_first_obs");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_of_first_obs]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("time_of_first_obs_time_zone");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_of_obs_time_zone]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("time_of_last_obs");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_of_last_obs]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("time_of_last_obs_time_zone");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_of_obs_time_zone]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("rcv_clock_offs_appl");
    coda_type_record_field_set_type(field, rinex_type[rinex_rcv_clock_offs_appl]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("leap_seconds");
    coda_type_record_field_set_type(field, rinex_type[rinex_leap_seconds]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);
    field = coda_type_record_field_new("num_satellites");
    coda_type_record_field_set_type(field, rinex_type[rinex_num_satellites]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_obs_header], field);


    rinex_type[rinex_ionospheric_corr_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_ionospheric_corr_type], "Correction type. GAL = Galileo: ai0 - ai2, "
                              "GPSA = GPS: alpha0 - alpha3, GPSB = GPS: beta0 - beta3");

    rinex_type[rinex_ionospheric_corr_parameter] = (coda_type *)coda_type_number_new(coda_format_rinex,
                                                                                     coda_real_class);

    rinex_type[rinex_ionospheric_corr_parameter_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_fixed_dimension((coda_type_array *)rinex_type[rinex_ionospheric_corr_parameter_array], 4);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_ionospheric_corr_parameter_array],
                                  rinex_type[rinex_ionospheric_corr_parameter]);
    coda_type_set_description(rinex_type[rinex_ionospheric_corr_parameter_array],
                              "GPS: alpha0-alpha3 or beta0-beta3, GAL: ai0, ai1, ai2, zero");

    rinex_type[rinex_ionospheric_corr] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("type");
    coda_type_record_field_set_type(field, rinex_type[rinex_ionospheric_corr_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_ionospheric_corr], field);
    field = coda_type_record_field_new("parameter");
    coda_type_record_field_set_type(field, rinex_type[rinex_ionospheric_corr_parameter_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_ionospheric_corr], field);

    rinex_type[rinex_ionospheric_corr_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_ionospheric_corr_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_ionospheric_corr_array],
                                  rinex_type[rinex_ionospheric_corr]);

    rinex_type[rinex_time_system_corr_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_time_system_corr_type], "Correction type. GAUT = GAL: to UTC a0, a1, "
                              "GPUT = GPS: to UTC a0, a1, SBUT = SBAS: to UTC a0, a1, GLUT = GLO: to UTC a0=TauC, "
                              "a1=zero, GPGA = GPS: to GAL a0=A0G, a1=A1G, GLGP = GLO: to GPS a0=TauGPS, a1=zero");

    rinex_type[rinex_time_system_corr_a0] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_time_system_corr_a0], "s");
    coda_type_set_description(rinex_type[rinex_time_system_corr_a0], "CORR(s) = a0 + a1 * DELTAT");

    rinex_type[rinex_time_system_corr_a1] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_time_system_corr_a1], "s/s");
    coda_type_set_description(rinex_type[rinex_time_system_corr_a1], "CORR(s) = a0 + a1 * DELTAT");

    rinex_type[rinex_time_system_corr_t] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_time_system_corr_t], coda_native_type_int32);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_time_system_corr_t], "s");
    coda_type_set_description(rinex_type[rinex_time_system_corr_t], "Reference time for polynomial");

    rinex_type[rinex_time_system_corr_w] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_time_system_corr_w], coda_native_type_int16);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_time_system_corr_w], "week");
    coda_type_set_description(rinex_type[rinex_time_system_corr_w], "Reference week number");

    rinex_type[rinex_time_system_corr_s] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_time_system_corr_s], "SBAS only. EGNOS, WAAS, or MSAS. "
                              "Derived from MT17 service provider. If not known: Use Snn with nn = PRN-100 of "
                              "satellite broadcasting the MT12");

    rinex_type[rinex_time_system_corr_u] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_time_system_corr_u], coda_native_type_uint8);
    coda_type_set_description(rinex_type[rinex_time_system_corr_u], "SBAS only. UTC Identifier (0 if unknown). "
                              "1=UTC(NIST), 2=UTC(USNO), 3=UTC(SU), 4=UTC(BIPM), 5=UTC(Europe Lab), 6=UTC(CRL), "
                              ">6 = not assigned yet");

    rinex_type[rinex_time_system_corr] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("type");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("a0");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_a0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("a1");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_a1]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("T");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_t]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("W");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_w]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("S");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_s]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);
    field = coda_type_record_field_new("U");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_u]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_time_system_corr], field);

    rinex_type[rinex_time_system_corr_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_time_system_corr_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_time_system_corr_array],
                                  rinex_type[rinex_time_system_corr]);

    rinex_type[rinex_nav_epoch] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_nav_epoch], rinex_type[rinex_epoch_string]);
    coda_type_set_description(rinex_type[rinex_nav_epoch], "Toc - Time of Clock (UTC)");

    rinex_type[rinex_nav_sv_clock_bias] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sv_clock_bias], "s");
    coda_type_set_description(rinex_type[rinex_nav_sv_clock_bias], "SV clock bias");

    rinex_type[rinex_nav_sv_clock_drift] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift], "s/s");
    coda_type_set_description(rinex_type[rinex_nav_sv_clock_drift], "SV clock drift");

    rinex_type[rinex_nav_sv_clock_drift_rate] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift_rate], "s/s^2");
    coda_type_set_description(rinex_type[rinex_nav_sv_clock_drift_rate], "SV clock drift rate");

    rinex_type[rinex_nav_iode] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_iode], "Issue of Data, Ephemeris");

    rinex_type[rinex_nav_crs] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_crs], "m");
    coda_type_set_description(rinex_type[rinex_nav_crs], "Crs");

    rinex_type[rinex_nav_delta_n] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_delta_n], "radians/s");
    coda_type_set_description(rinex_type[rinex_nav_delta_n], "Delta n");

    rinex_type[rinex_nav_m0] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_m0], "radians");
    coda_type_set_description(rinex_type[rinex_nav_m0], "M0");

    rinex_type[rinex_nav_cuc] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_cuc], "radians");
    coda_type_set_description(rinex_type[rinex_nav_cuc], "Cuc");

    rinex_type[rinex_nav_e] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_e], "Eccentricity");

    rinex_type[rinex_nav_cus] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_cus], "radians");
    coda_type_set_description(rinex_type[rinex_nav_cus], "Cus");

    rinex_type[rinex_nav_sqrt_a] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sqrt_a], "m^0.5");
    coda_type_set_description(rinex_type[rinex_nav_sqrt_a], "sqrt(A)");

    rinex_type[rinex_nav_toe] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_toe], "s");
    coda_type_set_description(rinex_type[rinex_nav_toe], "Time of Ephemeris (sec of GPS week)");

    rinex_type[rinex_nav_cic] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_cic], "radians");
    coda_type_set_description(rinex_type[rinex_nav_cic], "Cic");

    rinex_type[rinex_nav_omega0] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_omega0], "radians");
    coda_type_set_description(rinex_type[rinex_nav_omega0], "OMEGA0");

    rinex_type[rinex_nav_cis] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_cis], "radians");
    coda_type_set_description(rinex_type[rinex_nav_cis], "Cis");

    rinex_type[rinex_nav_i0] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_i0], "radians");
    coda_type_set_description(rinex_type[rinex_nav_i0], "i0");

    rinex_type[rinex_nav_crc] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_crc], "m");
    coda_type_set_description(rinex_type[rinex_nav_crc], "Crc");

    rinex_type[rinex_nav_omega] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_omega], "radians");
    coda_type_set_description(rinex_type[rinex_nav_omega], "omega");

    rinex_type[rinex_nav_omega_dot] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_omega_dot], "radians");
    coda_type_set_description(rinex_type[rinex_nav_omega_dot], "OMEGA DOT");

    rinex_type[rinex_nav_idot] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_idot], "radians/s");
    coda_type_set_description(rinex_type[rinex_nav_idot], "IDOT");

    rinex_type[rinex_nav_l2_codes] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_l2_codes], "Codes on L2 channel");

    rinex_type[rinex_nav_gps_week] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_gps_week],
                              "GPS Week # (to got with TOE). Continuous number, not mod(1024)!");

    rinex_type[rinex_nav_l2_p_data_flag] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_l2_p_data_flag], "L2 P data flag");

    rinex_type[rinex_nav_sv_accuracy] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sv_accuracy], "m");
    coda_type_set_description(rinex_type[rinex_nav_sv_accuracy], "SV accuracy");

    rinex_type[rinex_nav_sv_health_gps] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_sv_health_gps], "SV health (bits 17-22 w 3 sf 1)");

    rinex_type[rinex_nav_tgd] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_tgd], "s");
    coda_type_set_description(rinex_type[rinex_nav_tgd], "TGD");

    rinex_type[rinex_nav_iodc] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_iodc], "Issue of Data, Clock");

    rinex_type[rinex_nav_transmission_time_gps] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_transmission_time_gps], "s");
    coda_type_set_description(rinex_type[rinex_nav_transmission_time_gps], "Transmission time of message (sec of GPS "
                              "week, derived e.g. from z-count in Hand Over Word (HOW)");

    rinex_type[rinex_nav_fit_interval] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_fit_interval], "hours");
    coda_type_set_description(rinex_type[rinex_nav_fit_interval],
                              "Fit interval (see ICD-GPS-200, 20.3.4.4). Zero if not known");

    rinex_type[rinex_nav_iodnav] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_iodnav], "Issue of Data of the nav batch");

    rinex_type[rinex_nav_data_sources] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_nav_data_sources], coda_native_type_uint32);
    coda_type_set_description(rinex_type[rinex_nav_data_sources], "Data sources. Bit 0 set: I/NAV E1-B; Bit 1 set: "
                              "F/NAV E5a-I; Bit 2 set: I/NAV E5b-I; Bit 8 set: af0-af2, Toc are for E5a,E1; Bit 9 "
                              "set: af0-af2, Toc are for E5b,E1");

    rinex_type[rinex_nav_gal_week] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_gal_week], "GAL Week # (to go with Toe)");

    rinex_type[rinex_nav_sisa] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sisa], "m");
    coda_type_set_description(rinex_type[rinex_nav_sisa], "Signal in space accuracy");

    rinex_type[rinex_nav_sv_health_galileo] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_integer_class);
    coda_type_set_read_type(rinex_type[rinex_nav_sv_health_galileo], coda_native_type_uint32);
    coda_type_set_description(rinex_type[rinex_nav_sv_health_galileo], "SV health. Bit 0: E1B DVS, Bits 1-2: E1B HS, "
                              "Bit 3: E5a DVS, Bits 4-5: E5a HS, Bit 6: E5b DVS, Bits 7-8: E5b HS");

    rinex_type[rinex_nav_bgd_e5a_e1] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_bgd_e5a_e1], "s");
    coda_type_set_description(rinex_type[rinex_nav_bgd_e5a_e1], "BGD E5a/E1");

    rinex_type[rinex_nav_bgd_e5b_e1] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_bgd_e5b_e1], "s");
    coda_type_set_description(rinex_type[rinex_nav_bgd_e5b_e1], "BGD E5b/E1");

    rinex_type[rinex_nav_transmission_time_galileo] = (coda_type *)coda_type_number_new(coda_format_rinex,
                                                                                        coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_transmission_time_galileo], "s");
    coda_type_set_description(rinex_type[rinex_nav_transmission_time_galileo], "Transmission time of message (sec of "
                              "GAL week, derived from WN and TOW of page type 1)");

    rinex_type[rinex_nav_sv_rel_freq_bias] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_sv_rel_freq_bias], "SV relative frequency bias");

    rinex_type[rinex_nav_msg_frame_time] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_msg_frame_time], "s");
    coda_type_set_description(rinex_type[rinex_nav_msg_frame_time], "Message frame time in seconds of the UTC week");

    rinex_type[rinex_nav_sat_pos_x] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_pos_x], "km");
    coda_type_set_description(rinex_type[rinex_nav_sat_pos_x], "Satellite position X");

    rinex_type[rinex_nav_sat_pos_y] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_pos_y], "km");
    coda_type_set_description(rinex_type[rinex_nav_sat_pos_y], "Satellite position Y");

    rinex_type[rinex_nav_sat_pos_z] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_pos_z], "km");
    coda_type_set_description(rinex_type[rinex_nav_sat_pos_z], "Satellite position Z");

    rinex_type[rinex_nav_sat_vel_x] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_vel_x], "km/s");
    coda_type_set_description(rinex_type[rinex_nav_sat_vel_x], "Satellite velocity X dot");

    rinex_type[rinex_nav_sat_vel_y] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_vel_y], "km/s");
    coda_type_set_description(rinex_type[rinex_nav_sat_vel_y], "Satellite velocity Y dot");

    rinex_type[rinex_nav_sat_vel_z] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_vel_z], "km/s");
    coda_type_set_description(rinex_type[rinex_nav_sat_vel_z], "Satellite velocity Z dot");

    rinex_type[rinex_nav_sat_acc_x] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_acc_x], "km/s2");
    coda_type_set_description(rinex_type[rinex_nav_sat_acc_x], "Satellite X acceleration");

    rinex_type[rinex_nav_sat_acc_y] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_acc_y], "km/s2");
    coda_type_set_description(rinex_type[rinex_nav_sat_acc_y], "Satellite Y acceleration");

    rinex_type[rinex_nav_sat_acc_z] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_acc_z], "km/s2");
    coda_type_set_description(rinex_type[rinex_nav_sat_acc_z], "Satellite Z acceleration");

    rinex_type[rinex_nav_sat_health] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_sat_health], "health (0=OK) (Bn)");

    rinex_type[rinex_nav_sat_frequency_number] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_sat_frequency_number], "frequency number (1-24)");

    rinex_type[rinex_nav_age_of_oper_info] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_age_of_oper_info], "days");
    coda_type_set_description(rinex_type[rinex_nav_age_of_oper_info], "Age of oper. information (E)");

    rinex_type[rinex_nav_transmission_time_sbas] = (coda_type *)coda_type_number_new(coda_format_rinex,
                                                                                     coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_transmission_time_sbas], "s");
    coda_type_set_description(rinex_type[rinex_nav_transmission_time_sbas], "Transmission time of message (start of "
                              "the message) in GPS seconds of the week");

    rinex_type[rinex_nav_sat_accuracy_code] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_nav_sat_accuracy_code], "m");
    coda_type_set_description(rinex_type[rinex_nav_sat_accuracy_code], "Accuracy code (URA)");

    rinex_type[rinex_nav_iodn] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_description(rinex_type[rinex_nav_iodn],
                              "Issue of Data Navigation, DO229, 8 first bits after Message Type of MT9");

    rinex_type[rinex_nav_header] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("format_version");
    coda_type_record_field_set_type(field, rinex_type[rinex_format_version]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("file_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_file_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("satellite_system");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_system]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("program");
    coda_type_record_field_set_type(field, rinex_type[rinex_program]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("run_by");
    coda_type_record_field_set_type(field, rinex_type[rinex_run_by]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("datetime");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("datetime_time_zone");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime_time_zone]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("ionospheric_corr");
    coda_type_record_field_set_type(field, rinex_type[rinex_ionospheric_corr_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("time_system_corr");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_corr_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);
    field = coda_type_record_field_new("leap_seconds");
    coda_type_record_field_set_type(field, rinex_type[rinex_leap_seconds]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_header], field);

    rinex_type[rinex_nav_gps_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("number");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_epoch]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sv_clock_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sv_clock_drift");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_drift]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sv_clock_drift_rate");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_drift_rate]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("iode");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_iode]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("crs");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_crs]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("delta_n");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_delta_n]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("m0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_m0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("cuc");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cuc]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("e");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_e]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("cus");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cus]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sqrt_a");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sqrt_a]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("toe");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_toe]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("cic");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cic]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("omega0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("cis");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cis]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("i0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_i0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("crc");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_crc]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("omega");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("omega_dot");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega_dot]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("idot");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_idot]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("l2_codes");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_l2_codes]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("gps_week");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_gps_week]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("l2_p_data_flag");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_l2_p_data_flag]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sv_accuracy");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_accuracy]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("sv_health_gps");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_health_gps]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("tgd");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_tgd]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("iodc");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_iodc]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("transmission_time");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_transmission_time_gps]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);
    field = coda_type_record_field_new("fit_interval");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_fit_interval]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_gps_record], field);

    rinex_type[rinex_nav_glonass_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("number");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_epoch]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sv_clock_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sv_rel_freq_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_rel_freq_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("msg_frame_time");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_msg_frame_time]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_pos_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_vel_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_acc_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_health");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_health]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_pos_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_vel_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_acc_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_frequency_number");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_frequency_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_pos_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_vel_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("sat_acc_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);
    field = coda_type_record_field_new("age_of_oper_info");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_age_of_oper_info]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_glonass_record], field);

    rinex_type[rinex_nav_galileo_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("number");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_epoch]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sv_clock_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sv_clock_drift");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_drift]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sv_clock_drift_rate");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_drift_rate]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("iodnav");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_iodnav]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("crs");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_crs]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("delta_n");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_delta_n]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("m0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_m0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("cuc");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cuc]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("e");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_e]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("cus");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cus]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sqrt_a");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sqrt_a]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("toe");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_toe]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("cic");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cic]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("omega0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("cis");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_cis]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("i0");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_i0]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("crc");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_crc]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("omega");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("omega_dot");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_omega_dot]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("idot");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_idot]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("data_sources");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_data_sources]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("gal_week");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_gal_week]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sisa");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sisa]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("sv_health");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_health_galileo]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("bgd_e5a_e1");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_bgd_e5a_e1]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("bgd_e5b_e1");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_bgd_e5b_e1]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);
    field = coda_type_record_field_new("transmission_time");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_transmission_time_galileo]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_galileo_record], field);

    rinex_type[rinex_nav_sbas_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("number");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_number]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_epoch]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sv_clock_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_clock_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sv_rel_freq_bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sv_rel_freq_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("transmission_time");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_transmission_time_sbas]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_pos_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_vel_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_acc_x");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_x]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_health");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_health]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_pos_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_vel_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_acc_y");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_y]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_accuracy_code");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_accuracy_code]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_pos_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_pos_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_vel_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_vel_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("sat_acc_z");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sat_acc_z]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);
    field = coda_type_record_field_new("iodn");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_iodn]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_sbas_record], field);

    rinex_type[rinex_nav_gps_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_nav_gps_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_nav_gps_array], rinex_type[rinex_nav_gps_record]);

    rinex_type[rinex_nav_glonass_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_nav_glonass_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_nav_glonass_array],
                                  rinex_type[rinex_nav_glonass_record]);

    rinex_type[rinex_nav_galileo_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_nav_galileo_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_nav_galileo_array],
                                  rinex_type[rinex_nav_galileo_record]);

    rinex_type[rinex_nav_sbas_array] = (coda_type *)coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)rinex_type[rinex_nav_sbas_array], NULL);
    coda_type_array_set_base_type((coda_type_array *)rinex_type[rinex_nav_sbas_array],
                                  rinex_type[rinex_nav_sbas_record]);

    rinex_type[rinex_nav_file] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("header");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_header]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_file], field);
    field = coda_type_record_field_new("gps");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_gps_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_file], field);
    field = coda_type_record_field_new("glonass");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_glonass_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_file], field);
    field = coda_type_record_field_new("galileo");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_galileo_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_file], field);
    field = coda_type_record_field_new("sbas");
    coda_type_record_field_set_type(field, rinex_type[rinex_nav_sbas_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_nav_file], field);

    rinex_type[rinex_clk_type] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_clk_type], "Clock data type (AR, AS, CR, DR, MS)");

    rinex_type[rinex_clk_name] = (coda_type *)coda_type_text_new(coda_format_rinex);
    coda_type_set_description(rinex_type[rinex_clk_name], "Receiver or satellite name");

    rinex_type[rinex_clk_epoch] = (coda_type *)coda_type_time_new(coda_format_rinex, NULL);
    coda_type_time_set_base_type((coda_type_special *)rinex_type[rinex_clk_epoch], rinex_type[rinex_epoch_string]);
    coda_type_set_description(rinex_type[rinex_obs_epoch], "Epoch in GPS time (not local time!)");

    rinex_type[rinex_clk_bias] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_bias], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_bias], "Clock bias");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_clk_bias], "s");

    rinex_type[rinex_clk_bias_sigma] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_bias_sigma], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_bias_sigma], "Clock bias sigma");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_clk_bias_sigma], "s");

    rinex_type[rinex_clk_rate] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_rate], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_rate], "Clock rate");

    rinex_type[rinex_clk_rate_sigma] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_rate_sigma], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_rate_sigma], "Clock rate sigma");

    rinex_type[rinex_clk_acceleration] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_acceleration], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_acceleration], "Clock acceleration");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_clk_acceleration], "1/s");

    rinex_type[rinex_clk_acceleration_sigma] = (coda_type *)coda_type_number_new(coda_format_rinex, coda_real_class);
    coda_type_set_read_type(rinex_type[rinex_clk_acceleration_sigma], coda_native_type_double);
    coda_type_set_description(rinex_type[rinex_clk_acceleration_sigma], "Clock acceleration sigma");
    coda_type_number_set_unit((coda_type_number *)rinex_type[rinex_clk_acceleration_sigma], "1/s");

    rinex_type[rinex_clk_header] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("format_version");
    coda_type_record_field_set_type(field, rinex_type[rinex_format_version]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("file_type");
    coda_type_record_field_set_type(field, rinex_type[rinex_file_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("satellite_system");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_system]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("program");
    coda_type_record_field_set_type(field, rinex_type[rinex_program]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("run_by");
    coda_type_record_field_set_type(field, rinex_type[rinex_run_by]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("datetime");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("datetime_time_zone");
    coda_type_record_field_set_type(field, rinex_type[rinex_datetime_time_zone]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("sys");
    coda_type_record_field_set_type(field, rinex_type[rinex_sys_array]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("time_system_id");
    coda_type_record_field_set_type(field, rinex_type[rinex_time_system_id]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);
    field = coda_type_record_field_new("leap_seconds");
    coda_type_record_field_set_type(field, rinex_type[rinex_leap_seconds]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_header], field);

    rinex_type[rinex_clk_record] = (coda_type *)coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("type");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_type]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("name");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_name]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("epoch");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_epoch]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("bias");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_bias]);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("bias_sigma");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_bias_sigma]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("rate");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_rate]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("rate_sigma");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_rate_sigma]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("acceleration");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_acceleration]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);
    field = coda_type_record_field_new("acceleration_sigma");
    coda_type_record_field_set_type(field, rinex_type[rinex_clk_acceleration_sigma]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)rinex_type[rinex_clk_record], field);

    return 0;
}

void coda_rinex_done(void)
{
    int i;

    if (rinex_type == NULL)
    {
        return;
    }
    for (i = 0; i < num_rinex_types; i++)
    {
        if (rinex_type[i] != NULL)
        {
            coda_type_release(rinex_type[i]);
            rinex_type[i] = NULL;
        }
    }
    free(rinex_type);
    rinex_type = NULL;
}

static void rtrim(char *str)
{
    long length;

    length = strlen(str);
    while (length > 0 && str[length - 1] == ' ')
    {
        str[length - 1] = '\0';
        length--;
    }
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
    length = strlen(line);

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

static int read_main_header(ingest_info *info)
{
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    long linelength;

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    if (linelength < 61)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }
    if (strncmp(&line[60], "RINEX VERSION / TYPE", 20) != 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "invalid header item '%s' (line: %ld, byte offset: %ld)",
                       &line[60], info->linenumber, info->offset + 60);
        return -1;
    }
    if (coda_ascii_parse_double(line, 9, &info->format_version, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
        return -1;
    }
    info->file_type = line[20];

    switch (info->file_type)
    {
        case 'O':
            if (info->format_version != 3.0)
            {
                coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "RINEX format version %3.2f is not supported for "
                               "Observation data", info->format_version);
                return -1;
            }
            info->header = coda_mem_record_new((coda_type_record *)rinex_type[rinex_obs_header]);
            break;
        case 'N':
            if (info->format_version != 3.0)
            {
                coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "RINEX format version %3.2f is not supported for "
                               "Navigation data", info->format_version);
                return -1;
            }
            info->header = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_header]);
            break;
        case 'C':
            if (info->format_version != 2.0 && info->format_version != 3.0)
            {
                coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "RINEX format version %3.2f is not supported for "
                               "Clock data", info->format_version);
                return -1;
            }
            info->header = coda_mem_record_new((coda_type_record *)rinex_type[rinex_clk_header]);
            break;
        default:
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "RINEX file type '%c' is not supported", info->file_type);
            return -1;
    }

    if (info->format_version == 3.0)
    {
        info->satellite_system = line[40];
    }
    else
    {
        /* for older RINEX versions the only supported satellite system is GPS */
        info->satellite_system = 'G';
    }

    value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_format_version],
                                                   info->format_version);
    coda_mem_record_add_field(info->header, "format_version", value, 0);
    value = (coda_dynamic_type *)coda_mem_char_new((coda_type_text *)rinex_type[rinex_file_type], info->file_type);
    coda_mem_record_add_field(info->header, "file_type", value, 0);
    value = (coda_dynamic_type *)coda_mem_char_new((coda_type_text *)rinex_type[rinex_satellite_system],
                                                   info->satellite_system);
    coda_mem_record_add_field(info->header, "satellite_system", value, 0);

    return 0;
}

static int handle_observation_definition(ingest_info *info, char *line)
{
    satellite_info *sat_info;
    coda_type_record_field *field;
    coda_dynamic_type *value;
    coda_mem_record *sys;
    coda_mem_array *descriptor_array;
    const char *fieldname = NULL;
    int64_t num_types;
    int i;

    switch (line[0])
    {
        case 'G':      /* GPS */
            sat_info = &info->gps;
            fieldname = "gps";
            break;
        case 'R':      /* GLONASS */
            sat_info = &info->glonass;
            fieldname = "glonass";
            break;
        case 'E':      /* Galileo */
            sat_info = &info->galileo;
            fieldname = "galileo";
            break;
        case 'S':      /* SBAS */
            sat_info = &info->sbas;
            fieldname = "sbas";
            break;
        default:
            coda_set_error(CODA_ERROR_FILE_READ, "invalid satellite system for observation type definition "
                           "(line: %ld, byte offset: %ld)", info->linenumber, info->offset);
            return -1;
    }

    if (sat_info->sat_obs_definition != NULL)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "multiple observation type definitions for type '%c' "
                       "(line: %ld, byte offset: %ld)", line[0], info->linenumber, info->offset);
        return -1;
    }
    sat_info->sat_obs_definition = coda_type_record_new(coda_format_rinex);
    field = coda_type_record_field_new("number");
    coda_type_record_field_set_type(field, rinex_type[rinex_satellite_number]);
    coda_type_record_add_field(sat_info->sat_obs_definition, field);

    if (coda_ascii_parse_int64(&line[3], 3, &num_types, 0) < 0)
    {
        coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 3);
        return -1;
    }
    sys = coda_mem_record_new((coda_type_record *)rinex_type[rinex_sys]);
    value = (coda_dynamic_type *)coda_mem_char_new((coda_type_text *)rinex_type[rinex_sys_code], line[0]);
    coda_mem_record_add_field(sys, "code", value, 0);
    value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_sys_num_obs_types],
                                                      num_types);
    coda_mem_record_add_field(sys, "num_obs_types", value, 0);
    descriptor_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_sys_descriptor_array]);

    sat_info->observable = malloc((size_t)num_types * sizeof(char *));
    if (sat_info->observable == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_types * sizeof(char *), __FILE__, __LINE__);
        return -1;
    }
    sat_info->num_observables = (int)num_types;
    for (i = 0; i < num_types; i++)
    {
        sat_info->observable[i] = NULL;
    }

    for (i = 0; i < num_types; i++)
    {
        char str[4];

        if (i % 13 == 0 && i > 0)
        {
            long linelength;
            long expected_line_length = 6 + 13 * 4;

            /* read next line */
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)sys);
                coda_dynamic_type_delete((coda_dynamic_type *)descriptor_array);
                return -1;
            }
            if (num_types - i < 13)
            {
                expected_line_length = 6 + ((num_types - i) % 13) * 4;
            }
            if (linelength < expected_line_length)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
        }
        memcpy(str, &line[6 + (i % 13) * 4 + 1], 3);
        str[3] = '\0';
        value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_sys_descriptor], str);
        coda_mem_array_add_element(descriptor_array, value);

        field = coda_type_record_field_new(str);
        coda_type_record_field_set_type(field, rinex_type[rinex_observation_record]);
        coda_type_record_add_field(sat_info->sat_obs_definition, field);

        sat_info->observable[i] = strdup(str);
        if (sat_info->observable[i] == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }

    /* update header */
    coda_mem_record_add_field(sys, "descriptor", (coda_dynamic_type *)descriptor_array, 0);
    coda_mem_array_add_element(info->sys_array, (coda_dynamic_type *)sys);

    /* update epoch record definition */
    sat_info->sat_obs_array_definition = coda_type_array_new(coda_format_rinex);
    coda_type_array_add_variable_dimension((coda_type_array *)sat_info->sat_obs_array_definition, NULL);
    coda_type_array_set_base_type(sat_info->sat_obs_array_definition, (coda_type *)sat_info->sat_obs_definition);

    field = coda_type_record_field_new(fieldname);
    coda_type_record_field_set_type(field, (coda_type *)sat_info->sat_obs_array_definition);
    coda_type_record_add_field(info->epoch_record_definition, field);

    return 0;
}

static int read_observation_header(ingest_info *info)
{
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    int64_t int_value;
    char str[61];

    info->sys_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_sys_array]);

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        if (linelength < 61)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }

        if (strncmp(&line[60], "PGM / RUN BY / DATE", 19) == 0)
        {
            coda_dynamic_type *base_type;
            int year, month, day, hour, minute, second;

            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_program], str);
            coda_mem_record_add_field(info->header, "program", value, 0);
            memcpy(str, &line[20], 20);
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_run_by], str);
            coda_mem_record_add_field(info->header, "run_by", value, 0);
            memcpy(str, &line[40], 15);
            str[15] = '\0';
            if (strcmp(str, "               ") != 0)
            {
                if (sscanf(str, "%4d%2d%2d %2d%2d%2d", &year, &month, &day, &hour, &minute, &second) != 6)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                                   str, info->linenumber, info->offset + 40);
                    return -1;
                }
                if (coda_datetime_to_double(year, month, day, hour, minute, second, 0, &double_value) != 0)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                                   info->linenumber, info->offset + 40);
                    return -1;
                }
            }
            else
            {
                double_value = coda_NaN();
            }
            base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_string],
                                                               str);
            value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_datetime],
                                                           double_value, base_type);
            coda_mem_record_add_field(info->header, "datetime", value, 0);
            memcpy(str, &line[56], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_time_zone], str);
            coda_mem_record_add_field(info->header, "datetime_time_zone", value, 0);
        }
        else if (strncmp(&line[60], "COMMENT", 7) == 0)
        {
            /* ignore comments */
        }
        else if (strncmp(&line[60], "MARKER NAME", 11) == 0)
        {
            memcpy(str, line, 60);
            str[60] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_marker_name], str);
            coda_mem_record_add_field(info->header, "marker_name", value, 0);
        }
        else if (strncmp(&line[60], "MARKER NUMBER", 13) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_marker_number], str);
            coda_mem_record_add_field(info->header, "marker_number", value, 0);
        }
        else if (strncmp(&line[60], "MARKER TYPE", 10) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_marker_type], str);
            coda_mem_record_add_field(info->header, "marker_type", value, 0);
        }
        else if (strncmp(&line[60], "OBSERVER / AGENCY", 17) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_observer], str);
            coda_mem_record_add_field(info->header, "observer", value, 0);
            memcpy(str, &line[20], 40);
            str[40] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_agency], str);
            coda_mem_record_add_field(info->header, "agency", value, 0);
        }
        else if (strncmp(&line[60], "REC # / TYPE / VERS", 19) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_receiver_number], str);
            coda_mem_record_add_field(info->header, "receiver_number", value, 0);
            memcpy(str, &line[20], 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_receiver_type], str);
            coda_mem_record_add_field(info->header, "receiver_type", value, 0);
            memcpy(str, &line[40], 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_receiver_version], str);
            coda_mem_record_add_field(info->header, "receiver_version", value, 0);
        }
        else if (strncmp(&line[60], "ANT # / TYPE", 10) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_antenna_number], str);
            coda_mem_record_add_field(info->header, "antenna_number", value, 0);
            memcpy(str, &line[20], 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_antenna_type], str);
            coda_mem_record_add_field(info->header, "antenna_type", value, 0);
        }
        else if (strncmp(&line[60], "APPROX POSITION XYZ", 19) == 0)
        {
            if (coda_ascii_parse_double(line, 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_approx_position_x],
                                                           double_value);
            coda_mem_record_add_field(info->header, "approx_position_x", value, 0);
            if (coda_ascii_parse_double(&line[14], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 14);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_approx_position_y],
                                                           double_value);
            coda_mem_record_add_field(info->header, "approx_position_y", value, 0);
            if (coda_ascii_parse_double(&line[28], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 28);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_approx_position_z],
                                                           double_value);
            coda_mem_record_add_field(info->header, "approx_position_z", value, 0);
        }
        else if (strncmp(&line[60], "ANTENNA: DELTA H/E/N", 20) == 0)
        {
            if (coda_ascii_parse_double(line, 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_antenna_delta_h],
                                                           double_value);
            coda_mem_record_add_field(info->header, "antenna_delta_h", value, 0);
            if (coda_ascii_parse_double(&line[14], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 14);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_antenna_delta_e],
                                                           double_value);
            coda_mem_record_add_field(info->header, "antenna_delta_e", value, 0);
            if (coda_ascii_parse_double(&line[28], 14, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 28);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_antenna_delta_n],
                                                           double_value);
            coda_mem_record_add_field(info->header, "antenna_delta_n", value, 0);
        }
        else if (strncmp(&line[60], "ANTENNA: DELTA X/Y/Z", 20) == 0)
        {
        }
        else if (strncmp(&line[60], "ANTENNA: PHASECENTER", 20) == 0)
        {
        }
        else if (strncmp(&line[60], "ANTENNA: B.SIGHT XYZ", 20) == 0)
        {
        }
        else if (strncmp(&line[60], "ANTENNA: ZERODIR AZI", 20) == 0)
        {
        }
        else if (strncmp(&line[60], "ANTENNA: ZERODIR XYZ", 20) == 0)
        {
        }
        else if (strncmp(&line[60], "CENTER OF MASS: XYZ", 19) == 0)
        {
        }
        else if (strncmp(&line[60], "SYS / # / OBS TYPES", 19) == 0)
        {
            if (handle_observation_definition(info, line) != 0)
            {
                return -1;
            }
        }
        else if (strncmp(&line[60], "SIGNAL STRENGTH UNIT", 20) == 0)
        {
            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_signal_strength_unit],
                                                           str);
            coda_mem_record_add_field(info->header, "signal_strength_unit", value, 0);
        }
        else if (strncmp(&line[60], "INTERVAL", 8) == 0)
        {
            if (coda_ascii_parse_double(line, 10, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_obs_interval],
                                                           double_value);
            coda_mem_record_add_field(info->header, "obs_interval", value, 0);
        }
        else if (strncmp(&line[60], "TIME OF FIRST OBS", 17) == 0)
        {
            coda_dynamic_type *base_type;
            int year, month, day, hour, minute, second;
            double second_double;

            memcpy(str, line, 43);
            str[43] = '\0';
            if (sscanf(str, "%6d%6d%6d%6d%6d%lf", &year, &month, &day, &hour, &minute, &second_double) != 6)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                               str, info->linenumber, info->offset);
                return -1;
            }
            second = (int)second_double;
            if (coda_datetime_to_double(year, month, day, hour, minute, second, (int)((second_double - second) * 1e6),
                                        &double_value) != 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset);
                return -1;
            }
            base_type =
                (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_of_first_obs_string],
                                                       str);
            value =
                (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_time_of_first_obs],
                                                       double_value, base_type);
            coda_mem_record_add_field(info->header, "time_of_first_obs", value, 0);
            memcpy(str, &line[48], 3);
            str[3] = '\0';
            if (strcmp(str, "   ") == 0)
            {
                switch (info->satellite_system)
                {
                    case 'G':
                        strcpy(str, "GPS");
                        break;
                    case 'R':
                        strcpy(str, "GLO");
                        break;
                    case 'E':
                        strcpy(str, "GAL");
                        break;
                }
            }
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_of_obs_time_zone],
                                                           str);
            coda_mem_record_add_field(info->header, "time_of_first_obs_time_zone", value, 0);
        }
        else if (strncmp(&line[60], "TIME OF LAST OBS", 16) == 0)
        {
            coda_dynamic_type *base_type;
            int year, month, day, hour, minute, second;
            double second_double;

            memcpy(str, line, 43);
            str[43] = '\0';
            if (sscanf(str, "%6d%6d%6d%6d%6d%lf", &year, &month, &day, &hour, &minute, &second_double) != 6)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                               str, info->linenumber, info->offset);
                return -1;
            }
            second = (int)second_double;
            if (coda_datetime_to_double(year, month, day, hour, minute, second, (int)((second_double - second) * 1e6),
                                        &double_value) != 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset);
                return -1;
            }
            base_type =
                (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_of_last_obs_string],
                                                       str);
            value =
                (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_time_of_last_obs],
                                                       double_value, base_type);
            coda_mem_record_add_field(info->header, "time_of_last_obs", value, 0);
            memcpy(str, &line[48], 3);
            str[3] = '\0';
            if (strcmp(str, "   ") == 0)
            {
                switch (info->satellite_system)
                {
                    case 'G':
                        strcpy(str, "GPS");
                        break;
                    case 'R':
                        strcpy(str, "GLO");
                        break;
                    case 'E':
                        strcpy(str, "GAL");
                        break;
                }
            }
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_of_obs_time_zone],
                                                           str);
            coda_mem_record_add_field(info->header, "time_of_last_obs_time_zone", value, 0);
        }
        else if (strncmp(&line[60], "RCV CLOCK OFFS APPL", 19) == 0)
        {
            if (coda_ascii_parse_int64(line, 6, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_rcv_clock_offs_appl],
                                                              int_value);
            coda_mem_record_add_field(info->header, "rcv_clock_offs_appl", value, 0);
        }
        else if (strncmp(&line[60], "SYS / DCBS APPLIED", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "SYS / PCVS APPLIED", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "SYS / SCALE FACTOR", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "LEAP SECONDS", 12) == 0)
        {
            if (coda_ascii_parse_int64(line, 6, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_leap_seconds],
                                                              int_value);
            coda_mem_record_add_field(info->header, "leap_seconds", value, 0);
        }
        else if (strncmp(&line[60], "# OF SATELLITES", 15) == 0)
        {
            if (coda_ascii_parse_int64(line, 6, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_num_satellites],
                                                              int_value);
            coda_mem_record_add_field(info->header, "num_satellites", value, 0);
        }
        else if (strncmp(&line[60], "PRN / # OF OBS", 14) == 0)
        {
        }
        else if (strncmp(&line[60], "END OF HEADER", 13) == 0)
        {
            /* end of header */
            break;
        }
        else
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid header item '%s' (line: %ld, byte offset: %ld)",
                           &line[60], info->linenumber, info->offset + 60);
            return -1;
        }

        info->offset = ftell(info->f);
        info->linenumber++;
        linelength = get_line(info->f, line);
        if (linelength < 0)
        {
            return -1;
        }
    }

    coda_mem_record_add_field(info->header, "sys", (coda_dynamic_type *)info->sys_array, 0);
    info->sys_array = NULL;

    info->offset = ftell(info->f);
    info->linenumber++;
    return 0;
}

static int read_observation_record_for_satellite(ingest_info *info)
{
    satellite_info *sat_info;
    coda_mem_record *sat_obs;
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    long linelength;
    char str[17];
    int number;
    int i;

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }

    switch (line[0])
    {
        case 'G':      /* GPS */
            sat_info = &info->gps;
            break;
        case 'R':      /* GLONASS */
            sat_info = &info->glonass;
            break;
        case 'E':      /* Galileo */
            sat_info = &info->galileo;
            break;
        case 'S':      /* SBAS */
            sat_info = &info->sbas;
            break;
        default:
            coda_set_error(CODA_ERROR_FILE_READ, "invalid satellite system for epoch record "
                           "(line: %ld, byte offset: %ld)", info->linenumber, info->offset);
            return -1;
    }

    if (sat_info->sat_obs_array == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "satellite system '%c' was not defined in header for this observation "
                       "record (line: %ld, byte offset: %ld)", line[0], info->linenumber, info->offset);
        return -1;
    }
    assert(sat_info->sat_obs_definition != NULL);

    if (linelength >= 3 + sat_info->num_observables * 16 - 2)
    {
        /* append truncated 'blank' values back again to ease processing */
        while (linelength < 3 + sat_info->num_observables * 16)
        {
            line[linelength] = ' ';
            linelength++;
        }
    }

    if (linelength < 3 + sat_info->num_observables * 16)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "epoch line length (%ld) too short (line: %ld, byte offset: %ld)",
                       linelength, info->linenumber, info->offset);
        return -1;
    }

    sat_obs = coda_mem_record_new(sat_info->sat_obs_definition);

    memcpy(str, &line[1], 2);
    str[2] = '\0';
    if (sscanf(str, "%2d", &number) != 1)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)sat_obs);
        coda_set_error(CODA_ERROR_FILE_READ, "invalid satellite number (line: %ld, byte offset: %ld)",
                       info->linenumber, info->offset + 1);
        return -1;
    }
    value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_satellite_number], number);
    coda_mem_record_add_field(sat_obs, "number", value, 0);

    for (i = 0; i < sat_info->num_observables; i++)
    {
        uint8_t lli;
        uint8_t signal_strength;
        double observation;
        coda_mem_record *observation_record;

        memcpy(str, &line[3 + i * 16], 16);
        lli = (str[14] >= '0' && str[14] <= '9' ? str[14] - '0' : 0);
        signal_strength = (str[15] >= '0' && str[15] <= '9' ? str[15] - '0' : 0);
        str[14] = '\0';
        if (sscanf(str, "%lf", &observation) != 1)
        {
            if (strcmp(str, "              ") != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)sat_obs);
                coda_set_error(CODA_ERROR_FILE_READ, "invalid observation value (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset + 3 + i * 16);
                return -1;
            }
            /* if we have all blanks, use a 'missing value' of 0 */
            observation = 0.0;
        }

        observation_record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_observation_record]);
        value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_observation], observation);
        coda_mem_record_add_field(observation_record, "observation", value, 0);
        value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_lli], lli);
        coda_mem_record_add_field(observation_record, "lli", value, 0);
        value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_signal_strength],
                                                          signal_strength);
        coda_mem_record_add_field(observation_record, "signal_strength", value, 0);
        coda_mem_record_add_field(sat_obs, sat_info->observable[i], (coda_dynamic_type *)observation_record, 0);
    }

    coda_mem_array_add_element(sat_info->sat_obs_array, (coda_dynamic_type *)sat_obs);

    return 0;
}

static int read_observation_records(ingest_info *info)
{
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    char str[61];
    int i;

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        coda_dynamic_type *base_type;
        coda_dynamic_type *value;
        char epoch_string[28];
        int year, month, day, hour, minute, second;
        double second_double;
        char epoch_flag;
        int num_satellites;

        if (linelength < 35)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }
        if (line[0] != '>')
        {
            coda_set_error(CODA_ERROR_FILE_READ, "expected '>' as start of epoch record (line: %ld, byte offset: %ld)",
                           info->linenumber, info->offset);
            return -1;
        }

        info->epoch_record = coda_mem_record_new(info->epoch_record_definition);

        memcpy(epoch_string, &line[2], 27);
        epoch_string[27] = '\0';
        if (strcmp(epoch_string, "                           ") != 0)
        {
            if (sscanf(epoch_string, "%4d %2d %2d %2d %2d%lf", &year, &month, &day, &hour, &minute, &second_double) !=
                6)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                               epoch_string, info->linenumber, info->offset + 2);
                return -1;
            }
            second = (int)second_double;
            if (coda_datetime_to_double(year, month, day, hour, minute, second, (int)((second_double - second) * 1e6),
                                        &double_value) != 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset + 2);
                return -1;
            }
        }
        else
        {
            double_value = coda_NaN();
        }
        base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_epoch_string],
                                                           epoch_string);
        value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_obs_epoch],
                                                       double_value, base_type);
        coda_mem_record_add_field(info->epoch_record, "epoch", value, 0);

        epoch_flag = line[31];
        value = (coda_dynamic_type *)coda_mem_char_new((coda_type_text *)rinex_type[rinex_obs_epoch_flag], epoch_flag);
        coda_mem_record_add_field(info->epoch_record, "flag", value, 0);

        memcpy(str, &line[32], 3);
        str[3] = '\0';
        if (sscanf(str, "%3d", &num_satellites) != 1)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid 'number of satellites' entry in epoch record "
                           "(line: %ld, byte offset: %ld)", info->linenumber, info->offset + 34);
            return -1;
        }

        if (linelength >= 56)
        {
            if (coda_ascii_parse_double(&line[41], 15, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
        }
        else
        {
            double_value = 0;
        }
        value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_receiver_clock_offset],
                                                       double_value);
        coda_mem_record_add_field(info->epoch_record, "receiver_clock_offset", value, 0);

        if (info->gps.sat_obs_array_definition != NULL)
        {
            info->gps.sat_obs_array = coda_mem_array_new(info->gps.sat_obs_array_definition);
        }
        if (info->glonass.sat_obs_array_definition != NULL)
        {
            info->glonass.sat_obs_array = coda_mem_array_new(info->glonass.sat_obs_array_definition);
        }
        if (info->galileo.sat_obs_array_definition != NULL)
        {
            info->galileo.sat_obs_array = coda_mem_array_new(info->galileo.sat_obs_array_definition);
        }
        if (info->sbas.sat_obs_array_definition != NULL)
        {
            info->sbas.sat_obs_array = coda_mem_array_new(info->sbas.sat_obs_array_definition);
        }

        /* check epoch flag */
        if (epoch_flag != '0')
        {
            /* we skip the remaing part of this record if epoch flag != 0 */
            for (i = 0; i < num_satellites; i++)
            {
                info->offset = ftell(info->f);
                info->linenumber++;
                linelength = get_line(info->f, line);
                if (linelength < 0)
                {
                    return -1;
                }
            }
        }
        else
        {
            for (i = 0; i < num_satellites; i++)
            {
                if (read_observation_record_for_satellite(info) != 0)
                {
                    return -1;
                }
            }
        }

        if (info->gps.sat_obs_array != NULL)
        {
            coda_mem_record_add_field(info->epoch_record, "gps", (coda_dynamic_type *)info->gps.sat_obs_array, 0);
            info->gps.sat_obs_array = NULL;
        }
        if (info->glonass.sat_obs_array != NULL)
        {
            coda_mem_record_add_field(info->epoch_record, "glonass",
                                      (coda_dynamic_type *)info->glonass.sat_obs_array, 0);
            info->glonass.sat_obs_array = NULL;
        }
        if (info->galileo.sat_obs_array != NULL)
        {
            coda_mem_record_add_field(info->epoch_record, "galileo",
                                      (coda_dynamic_type *)info->galileo.sat_obs_array, 0);
            info->galileo.sat_obs_array = NULL;
        }
        if (info->sbas.sat_obs_array != NULL)
        {
            coda_mem_record_add_field(info->epoch_record, "sbas", (coda_dynamic_type *)info->sbas.sat_obs_array, 0);
            info->sbas.sat_obs_array = NULL;
        }
        coda_mem_array_add_element(info->records, (coda_dynamic_type *)info->epoch_record);
        info->epoch_record = NULL;

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

static int read_navigation_header(ingest_info *info)
{
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    int64_t int_value;
    char str[61];

    info->ionospheric_corr_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_ionospheric_corr_array]);
    info->time_system_corr_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_time_system_corr_array]);

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        if (linelength < 61)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }

        if (strncmp(&line[60], "PGM / RUN BY / DATE", 19) == 0)
        {
            coda_dynamic_type *base_type;
            int year, month, day, hour, minute, second;

            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_program], str);
            coda_mem_record_add_field(info->header, "program", value, 0);
            memcpy(str, &line[20], 20);
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_run_by], str);
            coda_mem_record_add_field(info->header, "run_by", value, 0);
            memcpy(str, &line[40], 15);
            str[15] = '\0';
            if (strcmp(str, "               ") != 0)
            {
                if (sscanf(str, "%4d%2d%2d %2d%2d%2d", &year, &month, &day, &hour, &minute, &second) != 6)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                                   str, info->linenumber, info->offset + 40);
                    return -1;
                }
                if (coda_datetime_to_double(year, month, day, hour, minute, second, 0, &double_value) != 0)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                                   info->linenumber, info->offset + 40);
                    return -1;
                }
            }
            else
            {
                double_value = coda_NaN();
            }
            base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_string],
                                                               str);
            value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_datetime],
                                                           double_value, base_type);
            coda_mem_record_add_field(info->header, "datetime", value, 0);
            memcpy(str, &line[56], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_time_zone], str);
            coda_mem_record_add_field(info->header, "datetime_time_zone", value, 0);
        }
        else if (strncmp(&line[60], "COMMENT", 7) == 0)
        {
            /* ignore comments */
        }
        else if (strncmp(&line[60], "IONOSPHERIC CORR", 16) == 0)
        {
            coda_mem_record *ionospheric_corr;
            coda_mem_array *parameter_array;
            int i;

            ionospheric_corr = coda_mem_record_new((coda_type_record *)rinex_type[rinex_ionospheric_corr]);

            memcpy(str, line, 4);
            str[4] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_ionospheric_corr_type],
                                                           str);
            coda_mem_record_add_field(ionospheric_corr, "type", value, 0);

            parameter_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_ionospheric_corr_parameter_array]);
            for (i = 0; i < 4; i++)
            {
                if (coda_ascii_parse_double(&line[5 + i * 12], 12, &double_value, 0) < 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)parameter_array);
                    coda_dynamic_type_delete((coda_dynamic_type *)ionospheric_corr);
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                value =
                    (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)
                                                           rinex_type[rinex_ionospheric_corr_parameter], double_value);
                coda_mem_array_set_element(parameter_array, i, value);
            }
            coda_mem_record_add_field(ionospheric_corr, "parameter", (coda_dynamic_type *)parameter_array, 0);

            coda_mem_array_add_element(info->ionospheric_corr_array, (coda_dynamic_type *)ionospheric_corr);
        }
        else if (strncmp(&line[60], "TIME SYSTEM CORR", 16) == 0)
        {
            coda_mem_record *time_system_corr;
            int is_sbas;

            time_system_corr = coda_mem_record_new((coda_type_record *)rinex_type[rinex_time_system_corr]);

            memcpy(str, line, 4);
            str[4] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_system_corr_type],
                                                           str);
            coda_mem_record_add_field(time_system_corr, "type", value, 0);
            is_sbas = (str[0] == 'S' && str[1] == 'B');

            if (coda_ascii_parse_double(&line[5], 17, &double_value, 0) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)time_system_corr);
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 5);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_time_system_corr_a0],
                                                           double_value);
            coda_mem_record_add_field(time_system_corr, "a0", value, 0);

            if (coda_ascii_parse_double(&line[22], 16, &double_value, 0) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)time_system_corr);
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 22);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_time_system_corr_a1],
                                                           double_value);
            coda_mem_record_add_field(time_system_corr, "a1", value, 0);

            if (coda_ascii_parse_int64(&line[38], 7, &int_value, 0) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)time_system_corr);
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 38);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_time_system_corr_t],
                                                              int_value);
            coda_mem_record_add_field(time_system_corr, "T", value, 0);

            if (coda_ascii_parse_int64(&line[45], 5, &int_value, 0) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)time_system_corr);
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 45);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_time_system_corr_w],
                                                              int_value);
            coda_mem_record_add_field(time_system_corr, "W", value, 0);

            if (is_sbas)
            {
                memcpy(str, &line[51], 5);
                str[5] = '\0';
                rtrim(str);
                value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_system_corr_s],
                                                               str);
                coda_mem_record_add_field(time_system_corr, "S", value, 0);

                if (coda_ascii_parse_int64(&line[57], 2, &int_value, 0) < 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)time_system_corr);
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 57);
                    return -1;
                }
                value =
                    (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_time_system_corr_u],
                                                              int_value);
                coda_mem_record_add_field(time_system_corr, "U", value, 0);
            }

            coda_mem_array_add_element(info->time_system_corr_array, (coda_dynamic_type *)time_system_corr);
        }
        else if (strncmp(&line[60], "LEAP SECONDS", 12) == 0)
        {
            if (coda_ascii_parse_int64(line, 6, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_leap_seconds],
                                                              int_value);
            coda_mem_record_add_field(info->header, "leap_seconds", value, 0);
        }
        else if (strncmp(&line[60], "END OF HEADER", 13) == 0)
        {
            /* end of header */
            break;
        }
        else
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid header item '%s' (line: %ld, byte offset: %ld)",
                           &line[60], info->linenumber, info->offset + 60);
            return -1;
        }

        info->offset = ftell(info->f);
        info->linenumber++;
        linelength = get_line(info->f, line);
        if (linelength < 0)
        {
            return -1;
        }
    }

    coda_mem_record_add_field(info->header, "ionospheric_corr", (coda_dynamic_type *)info->ionospheric_corr_array, 0);
    info->ionospheric_corr_array = NULL;
    coda_mem_record_add_field(info->header, "time_system_corr", (coda_dynamic_type *)info->time_system_corr_array, 0);
    info->time_system_corr_array = NULL;

    info->offset = ftell(info->f);
    info->linenumber++;
    return 0;
}

static int read_navigation_record_values(ingest_info *info, char *line, int num_values, double *value)
{
    int i;

    for (i = 0; i < num_values; i++)
    {
        int index = (i + 1) % 4;

        if (index == 0)
        {
            long expected_line_length = 4 + 4 * 19;
            long linelength;

            /* read next line */
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                return -1;
            }
            if (num_values - i < 4)
            {
                expected_line_length = 4 + ((num_values - i) % 4) * 19;
            }
            if (linelength < expected_line_length)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
        }
        if (coda_ascii_parse_double(&line[4 + index * 19], 19, &value[i], 0) < 0)
        {
            coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset + 4 + index * 19);
            return -1;
        }
    }

    return 0;
}

static int read_navigation_records(ingest_info *info)
{
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    char str[61];

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        coda_mem_record *record;
        coda_dynamic_type *base_type;
        coda_dynamic_type *value;
        char epoch_string[20];
        int year, month, day, hour, minute, second;
        int number;
        double record_value[31];
        char satellite_system;

        if (linelength < 23)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }
        satellite_system = line[0];

        switch (satellite_system)
        {
            case 'G':
                record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_gps_record]);
                break;
            case 'R':
                record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_glonass_record]);
                break;
            case 'E':
                record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_galileo_record]);
                break;
            case 'S':
                record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_sbas_record]);
                break;
            default:
                coda_set_error(CODA_ERROR_FILE_READ, "invalid satellite system for navigation record "
                               "(line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
        }

        memcpy(str, &line[1], 2);
        str[2] = '\0';
        if (sscanf(str, "%2d", &number) != 1)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)record);
            coda_set_error(CODA_ERROR_FILE_READ, "invalid satellite number (line: %ld, byte offset: %ld)",
                           info->linenumber, info->offset + 1);
            return -1;
        }
        value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_satellite_number],
                                                          number);
        coda_mem_record_add_field(record, "number", value, 0);

        memcpy(epoch_string, &line[4], 19);
        epoch_string[19] = '\0';
        if (sscanf(epoch_string, "%4d %2d %2d %2d %2d %d", &year, &month, &day, &hour, &minute, &second) != 6)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)record);
            coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                           epoch_string, info->linenumber, info->offset + 4);
            return -1;
        }
        if (coda_datetime_to_double(year, month, day, hour, minute, second, 0, &double_value) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)record);
            coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                           info->linenumber, info->offset + 4);
            return -1;
        }
        base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_epoch_string],
                                                           epoch_string);
        value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_nav_epoch],
                                                       double_value, base_type);
        coda_mem_record_add_field(record, "epoch", value, 0);

        if (satellite_system == 'G')
        {
            if (read_navigation_record_values(info, line, 29, record_value) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)record);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_bias],
                                                           record_value[0]);
            coda_mem_record_add_field(record, "sv_clock_bias", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift],
                                                           record_value[1]);
            coda_mem_record_add_field(record, "sv_clock_drift", value, 0);
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift_rate],
                                                       record_value[2]);
            coda_mem_record_add_field(record, "sv_clock_drift_rate", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_iode],
                                                           record_value[3]);
            coda_mem_record_add_field(record, "iode", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_crs],
                                                           record_value[4]);
            coda_mem_record_add_field(record, "crs", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_delta_n],
                                                           record_value[5]);
            coda_mem_record_add_field(record, "delta_n", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_m0],
                                                           record_value[6]);
            coda_mem_record_add_field(record, "m0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cuc],
                                                           record_value[7]);
            coda_mem_record_add_field(record, "cuc", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_e],
                                                           record_value[8]);
            coda_mem_record_add_field(record, "e", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cus],
                                                           record_value[9]);
            coda_mem_record_add_field(record, "cus", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sqrt_a],
                                                           record_value[10]);
            coda_mem_record_add_field(record, "sqrt_a", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_toe],
                                                           record_value[11]);
            coda_mem_record_add_field(record, "toe", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cic],
                                                           record_value[12]);
            coda_mem_record_add_field(record, "cic", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega0],
                                                           record_value[13]);
            coda_mem_record_add_field(record, "omega0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cis],
                                                           record_value[14]);
            coda_mem_record_add_field(record, "cis", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_i0],
                                                           record_value[15]);
            coda_mem_record_add_field(record, "i0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_crc],
                                                           record_value[16]);
            coda_mem_record_add_field(record, "crc", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega],
                                                           record_value[17]);
            coda_mem_record_add_field(record, "omega", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega_dot],
                                                           record_value[18]);
            coda_mem_record_add_field(record, "omega_dot", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_idot],
                                                           record_value[19]);
            coda_mem_record_add_field(record, "idot", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_l2_codes],
                                                           record_value[20]);
            coda_mem_record_add_field(record, "l2_codes", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_gps_week],
                                                           record_value[21]);
            coda_mem_record_add_field(record, "gps_week", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_l2_p_data_flag],
                                                           record_value[22]);
            coda_mem_record_add_field(record, "l2_p_data_flag", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_accuracy],
                                                           record_value[23]);
            coda_mem_record_add_field(record, "sv_accuracy", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_health_gps],
                                                           record_value[24]);
            coda_mem_record_add_field(record, "sv_health_gps", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_tgd],
                                                           record_value[25]);
            coda_mem_record_add_field(record, "tgd", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_iodc],
                                                           record_value[26]);
            coda_mem_record_add_field(record, "iodc", value, 0);
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_transmission_time_gps],
                                                       record_value[27]);
            coda_mem_record_add_field(record, "transmission_time", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_fit_interval],
                                                           record_value[28]);
            coda_mem_record_add_field(record, "fit_interval", value, 0);
            coda_mem_array_add_element(info->gps.records, (coda_dynamic_type *)record);
        }
        else if (satellite_system == 'R')
        {
            if (read_navigation_record_values(info, line, 15, record_value) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)record);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_bias],
                                                           record_value[0]);
            coda_mem_record_add_field(record, "sv_clock_bias", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_rel_freq_bias],
                                                           record_value[1]);
            coda_mem_record_add_field(record, "sv_rel_freq_bias", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_msg_frame_time],
                                                           record_value[2]);
            coda_mem_record_add_field(record, "msg_frame_time", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_x],
                                                           record_value[3]);
            coda_mem_record_add_field(record, "sat_pos_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_x],
                                                           record_value[4]);
            coda_mem_record_add_field(record, "sat_vel_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_x],
                                                           record_value[5]);
            coda_mem_record_add_field(record, "sat_acc_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_health],
                                                           record_value[6]);
            coda_mem_record_add_field(record, "sat_health", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_y],
                                                           record_value[7]);
            coda_mem_record_add_field(record, "sat_pos_y", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_y],
                                                           record_value[8]);
            coda_mem_record_add_field(record, "sat_vel_y", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_y],
                                                           record_value[9]);
            coda_mem_record_add_field(record, "sat_acc_y", value, 0);
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_frequency_number],
                                                       record_value[10]);
            coda_mem_record_add_field(record, "sat_frequency_number", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_z],
                                                           record_value[11]);
            coda_mem_record_add_field(record, "sat_pos_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_z],
                                                           record_value[12]);
            coda_mem_record_add_field(record, "sat_vel_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_z],
                                                           record_value[13]);
            coda_mem_record_add_field(record, "sat_acc_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_age_of_oper_info],
                                                           record_value[14]);
            coda_mem_record_add_field(record, "age_of_oper_info", value, 0);
            coda_mem_array_add_element(info->glonass.records, (coda_dynamic_type *)record);
        }
        else if (satellite_system == 'E')
        {
            if (read_navigation_record_values(info, line, 28, record_value) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)record);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_bias],
                                                           record_value[0]);
            coda_mem_record_add_field(record, "sv_clock_bias", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift],
                                                           record_value[1]);
            coda_mem_record_add_field(record, "sv_clock_drift", value, 0);
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_drift_rate],
                                                       record_value[2]);
            coda_mem_record_add_field(record, "sv_clock_drift_rate", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_iodnav],
                                                           record_value[3]);
            coda_mem_record_add_field(record, "iodnav", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_crs],
                                                           record_value[4]);
            coda_mem_record_add_field(record, "crs", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_delta_n],
                                                           record_value[5]);
            coda_mem_record_add_field(record, "delta_n", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_m0],
                                                           record_value[6]);
            coda_mem_record_add_field(record, "m0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cuc],
                                                           record_value[7]);
            coda_mem_record_add_field(record, "cuc", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_e],
                                                           record_value[8]);
            coda_mem_record_add_field(record, "e", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cus],
                                                           record_value[9]);
            coda_mem_record_add_field(record, "cus", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sqrt_a],
                                                           record_value[10]);
            coda_mem_record_add_field(record, "sqrt_a", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_toe],
                                                           record_value[11]);
            coda_mem_record_add_field(record, "toe", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cic],
                                                           record_value[12]);
            coda_mem_record_add_field(record, "cic", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega0],
                                                           record_value[13]);
            coda_mem_record_add_field(record, "omega0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_cis],
                                                           record_value[14]);
            coda_mem_record_add_field(record, "cis", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_i0],
                                                           record_value[15]);
            coda_mem_record_add_field(record, "i0", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_crc],
                                                           record_value[16]);
            coda_mem_record_add_field(record, "crc", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega],
                                                           record_value[17]);
            coda_mem_record_add_field(record, "omega", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_omega_dot],
                                                           record_value[18]);
            coda_mem_record_add_field(record, "omega_dot", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_idot],
                                                           record_value[19]);
            coda_mem_record_add_field(record, "idot", value, 0);
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_nav_data_sources],
                                                              (int64_t)record_value[20]);
            coda_mem_record_add_field(record, "data_sources", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_gal_week],
                                                           record_value[21]);
            coda_mem_record_add_field(record, "gal_week", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sisa],
                                                           record_value[23]);
            coda_mem_record_add_field(record, "sisa", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_health_galileo],
                                                           record_value[24]);
            coda_mem_record_add_field(record, "sv_health", value, 0);
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_nav_bgd_e5a_e1],
                                                              (int64_t)record_value[25]);
            coda_mem_record_add_field(record, "bgd_e5a_e1", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_bgd_e5b_e1],
                                                           record_value[26]);
            coda_mem_record_add_field(record, "bgd_e5b_e1", value, 0);
            value =
                (coda_dynamic_type *)
                coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_transmission_time_galileo],
                                  record_value[27]);
            coda_mem_record_add_field(record, "transmission_time", value, 0);
            coda_mem_array_add_element(info->galileo.records, (coda_dynamic_type *)record);
        }
        else if (satellite_system == 'S')
        {
            if (read_navigation_record_values(info, line, 15, record_value) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)record);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_clock_bias],
                                                           record_value[0]);
            coda_mem_record_add_field(record, "sv_clock_bias", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sv_rel_freq_bias],
                                                           record_value[1]);
            coda_mem_record_add_field(record, "sv_rel_freq_bias", value, 0);
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_transmission_time_sbas],
                                                       record_value[2]);
            coda_mem_record_add_field(record, "transmission_time", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_x],
                                                           record_value[3]);
            coda_mem_record_add_field(record, "sat_pos_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_x],
                                                           record_value[4]);
            coda_mem_record_add_field(record, "sat_vel_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_x],
                                                           record_value[5]);
            coda_mem_record_add_field(record, "sat_acc_x", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_health],
                                                           record_value[6]);
            coda_mem_record_add_field(record, "sat_health", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_y],
                                                           record_value[7]);
            coda_mem_record_add_field(record, "sat_pos_y", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_y],
                                                           record_value[8]);
            coda_mem_record_add_field(record, "sat_vel_y", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_y],
                                                           record_value[9]);
            coda_mem_record_add_field(record, "sat_acc_y", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_accuracy_code],
                                                           record_value[10]);
            coda_mem_record_add_field(record, "sat_accuracy_code", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_pos_z],
                                                           record_value[11]);
            coda_mem_record_add_field(record, "sat_pos_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_vel_z],
                                                           record_value[12]);
            coda_mem_record_add_field(record, "sat_vel_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_sat_acc_z],
                                                           record_value[13]);
            coda_mem_record_add_field(record, "sat_acc_z", value, 0);
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_nav_iodn],
                                                           record_value[14]);
            coda_mem_record_add_field(record, "iodn", value, 0);
            coda_mem_array_add_element(info->sbas.records, (coda_dynamic_type *)record);
        }

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

static int read_clock_header(ingest_info *info)
{
    coda_dynamic_type *value;
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    int64_t int_value;
    char str[61];

    info->sys_array = coda_mem_array_new((coda_type_array *)rinex_type[rinex_sys_array]);

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        if (linelength < 61)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "header line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }

        if (strncmp(&line[60], "PGM / RUN BY / DATE", 19) == 0)
        {
            coda_dynamic_type *base_type;
            int year, month, day, hour, minute, second;

            memcpy(str, line, 20);
            str[20] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_program], str);
            coda_mem_record_add_field(info->header, "program", value, 0);
            memcpy(str, &line[20], 20);
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_run_by], str);
            coda_mem_record_add_field(info->header, "run_by", value, 0);
            memcpy(str, &line[40], 15);
            str[15] = '\0';
            if (strcmp(str, "               ") != 0)
            {
                if (sscanf(str, "%4d%2d%2d %2d%2d%2d", &year, &month, &day, &hour, &minute, &second) != 6)
                {
                    if (info->format_version == 3.0)
                    {
                        coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                                       str, info->linenumber, info->offset + 40);
                        return -1;
                    }
                    /* for older RINEX Clock versions just set datetime to NaN */
                    double_value = coda_NaN();
                }
                else if (coda_datetime_to_double(year, month, day, hour, minute, second, 0, &double_value) != 0)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                                   info->linenumber, info->offset + 40);
                    return -1;
                }
            }
            else
            {
                double_value = coda_NaN();
            }
            base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_string],
                                                               str);
            value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_datetime],
                                                           double_value, base_type);
            coda_mem_record_add_field(info->header, "datetime", value, 0);
            memcpy(str, &line[56], 3);
            str[3] = '\0';
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_datetime_time_zone], str);
            coda_mem_record_add_field(info->header, "datetime_time_zone", value, 0);
        }
        else if (strncmp(&line[60], "COMMENT", 7) == 0)
        {
            /* ignore comments */
        }
        else if (strncmp(&line[60], "SYS / # / OBS TYPES", 19) == 0)
        {
            if (handle_observation_definition(info, line) != 0)
            {
                return -1;
            }
        }
        else if (strncmp(&line[60], "TIME SYSTEM ID", 14) == 0)
        {
            memcpy(str, &line[3], 3);
            str[3] = '\0';
            rtrim(str);
            value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_time_system_id], str);
            coda_mem_record_add_field(info->header, "time_system_id", value, 0);
        }
        else if (strncmp(&line[60], "LEAP SECONDS", 12) == 0)
        {
            if (coda_ascii_parse_int64(line, 6, &int_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)rinex_type[rinex_leap_seconds],
                                                              int_value);
            coda_mem_record_add_field(info->header, "leap_seconds", value, 0);
        }
        else if (strncmp(&line[60], "SYS / DCBS APPLIED", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "SYS / PCVS APPLIED", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "# / TYPES OF DATA", 17) == 0)
        {
        }
        else if (strncmp(&line[60], "STATION NAME / NUM", 18) == 0)
        {
        }
        else if (strncmp(&line[60], "STATION CLK REF", 15) == 0)
        {
        }
        else if (strncmp(&line[60], "ANALYSIS CENTER", 15) == 0)
        {
        }
        else if (strncmp(&line[60], "# OF CLK REF", 12) == 0)
        {
        }
        else if (strncmp(&line[60], "ANALYSIS CLK REF", 16) == 0)
        {
        }
        else if (strncmp(&line[60], "# OF SOLN STA / TRF", 19) == 0)
        {
        }
        else if (strncmp(&line[60], "SOLN STA NAME / NUM", 19) == 0)
        {
        }
        else if (strncmp(&line[60], "# OF SOLN SATS", 14) == 0)
        {
        }
        else if (strncmp(&line[60], "PRN LIST", 8) == 0)
        {
        }
        else if (strncmp(&line[60], "END OF HEADER", 13) == 0)
        {
            /* end of header */
            break;
        }
        else
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid header item '%s' (line: %ld, byte offset: %ld)",
                           &line[60], info->linenumber, info->offset + 60);
            return -1;
        }

        info->offset = ftell(info->f);
        info->linenumber++;
        linelength = get_line(info->f, line);
        if (linelength < 0)
        {
            return -1;
        }
    }

    coda_mem_record_add_field(info->header, "sys", (coda_dynamic_type *)info->sys_array, 0);
    info->sys_array = NULL;

    info->offset = ftell(info->f);
    info->linenumber++;
    return 0;
}

static int read_clock_records(ingest_info *info)
{
    char line[MAX_LINE_LENGTH];
    long linelength;
    double double_value;
    char str[61];

    info->offset = ftell(info->f);
    info->linenumber++;
    linelength = get_line(info->f, line);
    if (linelength < 0)
    {
        return -1;
    }
    while (linelength > 0)
    {
        coda_dynamic_type *base_type;
        coda_dynamic_type *value;
        char epoch_string[28];
        int year, month, day, hour, minute, second;
        double second_double;
        int num_values;

        if (linelength < 40)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                           linelength, info->linenumber, info->offset);
            return -1;
        }

        info->epoch_record = coda_mem_record_new((coda_type_record *)rinex_type[rinex_clk_record]);

        memcpy(str, line, 2);
        str[2] = '\0';
        rtrim(str);
        value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_clk_type], str);
        coda_mem_record_add_field(info->epoch_record, "type", value, 0);

        memcpy(str, &line[3], 4);
        str[4] = '\0';
        rtrim(str);
        value = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_clk_name], str);
        coda_mem_record_add_field(info->epoch_record, "name", value, 0);

        memcpy(epoch_string, &line[8], 27);
        epoch_string[27] = '\0';
        if (strcmp(epoch_string, "                           ") != 0)
        {
            if (sscanf(epoch_string, "%4d %2d %2d %2d %2d%lf", &year, &month, &day, &hour, &minute, &second_double) !=
                6)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time string '%s' (line: %ld, byte offset: %ld)",
                               epoch_string, info->linenumber, info->offset + 2);
                return -1;
            }
            second = (int)second_double;
            if (coda_datetime_to_double(year, month, day, hour, minute, second, (int)((second_double - second) * 1e6),
                                        &double_value) != 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "invalid time value (line: %ld, byte offset: %ld)",
                               info->linenumber, info->offset + 2);
                return -1;
            }
        }
        else
        {
            double_value = coda_NaN();
        }
        base_type = (coda_dynamic_type *)coda_mem_text_new((coda_type_text *)rinex_type[rinex_epoch_string],
                                                           epoch_string);
        value = (coda_dynamic_type *)coda_mem_time_new((coda_type_special *)rinex_type[rinex_clk_epoch],
                                                       double_value, base_type);
        coda_mem_record_add_field(info->epoch_record, "epoch", value, 0);

        memcpy(str, &line[34], 3);
        str[3] = '\0';
        if (sscanf(str, "%3d", &num_values) != 1 || num_values < 1 || num_values > 6)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "invalid 'number of data values' entry in clock record "
                           "(line: %ld, byte offset: %ld)", info->linenumber, info->offset + 34);
            return -1;
        }

        if (coda_ascii_parse_double(&line[40], 19, &double_value, 0) < 0)
        {
            coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
            return -1;
        }
        value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_bias], double_value);
        coda_mem_record_add_field(info->epoch_record, "bias", value, 0);
        if (num_values > 1)
        {
            if (linelength < 79)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
            if (coda_ascii_parse_double(&line[60], 19, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_bias_sigma],
                                                           double_value);
            coda_mem_record_add_field(info->epoch_record, "bias_sigma", value, 0);
        }

        if (num_values > 2)
        {
            /* read next line */
            info->offset = ftell(info->f);
            info->linenumber++;
            linelength = get_line(info->f, line);
            if (linelength < 0)
            {
                return -1;
            }
            if (linelength < (num_values - 2) * 20 - 1)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "record line length (%ld) too short (line: %ld, byte offset: %ld)",
                               linelength, info->linenumber, info->offset);
                return -1;
            }
            if (coda_ascii_parse_double(line, 19, &double_value, 0) < 0)
            {
                coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                return -1;
            }
            value =
                (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_rate], double_value);
            coda_mem_record_add_field(info->epoch_record, "rate", value, 0);
            if (num_values > 3)
            {
                if (coda_ascii_parse_double(&line[20], 19, &double_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_rate_sigma],
                                                               double_value);
                coda_mem_record_add_field(info->epoch_record, "rate_sigma", value, 0);
            }
            if (num_values > 4)
            {
                if (coda_ascii_parse_double(&line[40], 19, &double_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                value = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_acceleration],
                                                               double_value);
                coda_mem_record_add_field(info->epoch_record, "acceleration", value, 0);
            }
            if (num_values > 5)
            {
                if (coda_ascii_parse_double(&line[60], 19, &double_value, 0) < 0)
                {
                    coda_add_error_message(" (line: %ld, byte offset: %ld)", info->linenumber, info->offset);
                    return -1;
                }
                value =
                    (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)rinex_type[rinex_clk_acceleration_sigma],
                                                           double_value);
                coda_mem_record_add_field(info->epoch_record, "acceleration_sigma", value, 0);
            }
        }
        coda_mem_array_add_element(info->records, (coda_dynamic_type *)info->epoch_record);
        info->epoch_record = NULL;

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

static int read_file(const char *filename, coda_dynamic_type **root)
{
    coda_type_array *records_definition;
    coda_type_record_field *field;
    coda_type_record *definition;
    coda_mem_record *root_type = NULL;
    ingest_info info;

    ingest_info_init(&info);

    info.f = fopen(filename, "r");
    if (info.f == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", filename);
        return -1;
    }

    if (read_main_header(&info) != 0)
    {
        ingest_info_cleanup(&info);
        return -1;
    }

    if (info.file_type == 'O')
    {
        info.epoch_record_definition = coda_type_record_new(coda_format_rinex);
        field = coda_type_record_field_new("epoch");
        coda_type_record_field_set_type(field, rinex_type[rinex_obs_epoch]);
        coda_type_record_add_field(info.epoch_record_definition, field);
        field = coda_type_record_field_new("flag");
        coda_type_record_field_set_type(field, rinex_type[rinex_obs_epoch_flag]);
        coda_type_record_add_field(info.epoch_record_definition, field);
        field = coda_type_record_field_new("receiver_clock_offset");
        coda_type_record_field_set_type(field, rinex_type[rinex_receiver_clock_offset]);
        coda_type_record_add_field(info.epoch_record_definition, field);

        if (read_observation_header(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }
        if (coda_mem_record_validate(info.header) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        /* create /record array */
        records_definition = coda_type_array_new(coda_format_rinex);
        coda_type_array_add_variable_dimension((coda_type_array *)records_definition, NULL);
        coda_type_array_set_base_type(records_definition, (coda_type *)info.epoch_record_definition);
        info.records = coda_mem_array_new(records_definition);
        coda_type_release((coda_type *)records_definition);

        if (read_observation_records(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        /* create root record */
        definition = coda_type_record_new(coda_format_rinex);
        root_type = coda_mem_record_new(definition);
        coda_type_release((coda_type *)definition);
        coda_mem_record_add_field(root_type, "header", (coda_dynamic_type *)info.header, 1);
        info.header = NULL;
        coda_mem_record_add_field(root_type, "record", (coda_dynamic_type *)info.records, 1);
        info.records = NULL;
    }
    else if (info.file_type == 'N')
    {
        info.epoch_record_definition = coda_type_record_new(coda_format_rinex);
        field = coda_type_record_field_new("epoch");
        coda_type_record_field_set_type(field, rinex_type[rinex_obs_epoch]);
        coda_type_record_add_field(info.epoch_record_definition, field);
        field = coda_type_record_field_new("flag");
        coda_type_record_field_set_type(field, rinex_type[rinex_obs_epoch_flag]);
        coda_type_record_add_field(info.epoch_record_definition, field);
        field = coda_type_record_field_new("receiver_clock_offset");
        coda_type_record_field_set_type(field, rinex_type[rinex_receiver_clock_offset]);
        coda_type_record_add_field(info.epoch_record_definition, field);

        if (read_navigation_header(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }
        if (coda_mem_record_validate(info.header) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        info.gps.records = coda_mem_array_new((coda_type_array *)rinex_type[rinex_nav_gps_array]);
        info.glonass.records = coda_mem_array_new((coda_type_array *)rinex_type[rinex_nav_glonass_array]);
        info.galileo.records = coda_mem_array_new((coda_type_array *)rinex_type[rinex_nav_galileo_array]);
        info.sbas.records = coda_mem_array_new((coda_type_array *)rinex_type[rinex_nav_sbas_array]);

        if (read_navigation_records(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        /* create root record */
        root_type = coda_mem_record_new((coda_type_record *)rinex_type[rinex_nav_file]);
        coda_mem_record_add_field(root_type, "header", (coda_dynamic_type *)info.header, 0);
        info.header = NULL;
        coda_mem_record_add_field(root_type, "gps", (coda_dynamic_type *)info.gps.records, 0);
        info.gps.records = NULL;
        coda_mem_record_add_field(root_type, "glonass", (coda_dynamic_type *)info.glonass.records, 0);
        info.glonass.records = NULL;
        coda_mem_record_add_field(root_type, "galileo", (coda_dynamic_type *)info.galileo.records, 0);
        info.galileo.records = NULL;
        coda_mem_record_add_field(root_type, "sbas", (coda_dynamic_type *)info.sbas.records, 0);
        info.sbas.records = NULL;
    }
    else        /* file_type == 'C' */
    {
        if (read_clock_header(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }
        if (coda_mem_record_validate(info.header) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        /* create /record array */
        records_definition = coda_type_array_new(coda_format_rinex);
        coda_type_array_add_variable_dimension((coda_type_array *)records_definition, NULL);
        coda_type_array_set_base_type(records_definition, rinex_type[rinex_clk_record]);
        info.records = coda_mem_array_new(records_definition);
        coda_type_release((coda_type *)records_definition);

        if (read_clock_records(&info) != 0)
        {
            ingest_info_cleanup(&info);
            return -1;
        }

        /* create root record */
        definition = coda_type_record_new(coda_format_rinex);
        root_type = coda_mem_record_new(definition);
        coda_type_release((coda_type *)definition);
        coda_mem_record_add_field(root_type, "header", (coda_dynamic_type *)info.header, 1);
        info.header = NULL;
        coda_mem_record_add_field(root_type, "record", (coda_dynamic_type *)info.records, 1);
        info.records = NULL;
    }

    *root = (coda_dynamic_type *)root_type;

    ingest_info_cleanup(&info);

    return 0;
}

int coda_rinex_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                    coda_product **product)
{
    coda_product *product_file;

    if (rinex_init() != 0)
    {
        return -1;
    }

    product_file = (coda_product *)malloc(sizeof(coda_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_rinex;
    product_file->root_type = NULL;
    product_file->product_definition = definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
#if CODA_USE_QIAP
    product_file->qiap_info = NULL;
#endif

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_rinex_close(product_file);
        return -1;
    }

    /* create root type */
    if (read_file(filename, &product_file->root_type) != 0)
    {
        coda_rinex_close(product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_rinex_close(coda_product *product)
{
    if (product->root_type != NULL)
    {
        coda_dynamic_type_delete(product->root_type);
    }

    if (product->filename != NULL)
    {
        free(product->filename);
    }

    free(product);

    return 0;
}

int coda_rinex_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for memory backend */

    return 0;
}
