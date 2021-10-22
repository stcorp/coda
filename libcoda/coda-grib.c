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

#include "coda-internal.h"
#include "coda-grib-internal.h"
#include "coda-definition.h"
#include "coda-mem-internal.h"
#include "coda-read-bytes.h"
#include "coda-read-bytes-in-bounds.h"
#ifndef WORDS_BIGENDIAN
#include "coda-swap4.h"
#include "coda-swap8.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


enum
{
    grib1_localRecordIndex,
    grib1_gridRecordIndex,
    grib1_table2Version,
    grib1_editionNumber,
    grib1_centre,
    grib1_generatingProcessIdentifier,
    grib1_gridDefinition,
    grib1_indicatorOfParameter,
    grib1_indicatorOfTypeOfLevel,
    grib1_level,
    grib1_yearOfCentury,
    grib1_year,
    grib1_month,
    grib1_day,
    grib1_hour,
    grib1_minute,
    grib1_second,
    grib1_unitOfTimeRange,
    grib1_P1,
    grib1_P2,
    grib1_timeRangeIndicator,
    grib1_numberIncludedInAverage,
    grib1_numberMissingFromAveragesOrAccumulations,
    grib1_centuryOfReferenceTimeOfData,
    grib1_subCentre,
    grib1_decimalScaleFactor,
    grib1_discipline,
    grib1_masterTablesVersion,
    grib1_localTablesVersion,
    grib1_significanceOfReferenceTime,
    grib1_productionStatusOfProcessedData,
    grib1_typeOfProcessedData,
    grib1_local,
    grib1_numberOfVerticalCoordinateValues,
    grib1_dataRepresentationType,
    grib1_shapeOfTheEarth,
    grib1_scaleFactorOfRadiusOfSphericalEarth,
    grib1_scaledValueOfRadiusOfSphericalEarth,
    grib1_scaleFactorOfEarthMajorAxis,
    grib1_scaledValueOfEarthMajorAxis,
    grib1_scaleFactorOfEarthMinorAxis,
    grib1_scaledValueOfEarthMinorAxis,
    grib1_Ni,
    grib1_Nj,
    grib1_basicAngleOfTheInitialProductionDomain,
    grib1_subdivisionsOfBasicAngle,
    grib1_latitudeOfFirstGridPoint,
    grib1_longitudeOfFirstGridPoint,
    grib1_resolutionAndComponentFlags,
    grib1_latitudeOfLastGridPoint,
    grib1_longitudeOfLastGridPoint,
    grib1_iDirectionIncrement,
    grib1_jDirectionIncrement,
    grib1_N,
    grib1_scanningMode,
    grib1_coordinateValues,
    grib1_coordinateValues_array,
    grib1_listOfNumbers,
    grib1_listOfNumbers_array,
    grib1_sourceOfGridDefinition,
    grib1_numberOfDataPoints,
    grib1_gridDefinitionTemplateNumber,
    grib1_bitsPerValue,
    grib1_binaryScaleFactor,
    grib1_referenceValue,
    grib1_values,
    grib1_grid,
    grib1_data,
    grib1_message,

    grib2_localRecordIndex,
    grib2_gridRecordIndex,
    grib2_editionNumber,
    grib2_centre,
    grib2_year,
    grib2_month,
    grib2_day,
    grib2_hour,
    grib2_minute,
    grib2_second,
    grib2_subCentre,
    grib2_decimalScaleFactor,
    grib2_discipline,
    grib2_masterTablesVersion,
    grib2_localTablesVersion,
    grib2_significanceOfReferenceTime,
    grib2_productionStatusOfProcessedData,
    grib2_typeOfProcessedData,
    grib2_local,
    grib2_numberOfVerticalCoordinateValues,
    grib2_dataRepresentationType,
    grib2_shapeOfTheEarth,
    grib2_scaleFactorOfRadiusOfSphericalEarth,
    grib2_scaledValueOfRadiusOfSphericalEarth,
    grib2_scaleFactorOfEarthMajorAxis,
    grib2_scaledValueOfEarthMajorAxis,
    grib2_scaleFactorOfEarthMinorAxis,
    grib2_scaledValueOfEarthMinorAxis,
    grib2_Ni,
    grib2_Nj,
    grib2_basicAngleOfTheInitialProductionDomain,
    grib2_subdivisionsOfBasicAngle,
    grib2_latitudeOfFirstGridPoint,
    grib2_longitudeOfFirstGridPoint,
    grib2_resolutionAndComponentFlags,
    grib2_latitudeOfLastGridPoint,
    grib2_longitudeOfLastGridPoint,
    grib2_iDirectionIncrement,
    grib2_jDirectionIncrement,
    grib2_N,
    grib2_scanningMode,
    grib2_listOfNumbers,
    grib2_listOfNumbers_array,
    grib2_sourceOfGridDefinition,
    grib2_numberOfDataPoints,
    grib2_interpretationOfListOfNumbers,
    grib2_parameterCategory,
    grib2_parameterNumber,
    grib2_constituentType,
    grib2_typeOfGeneratingProcess,
    grib2_backgroundProcess,
    grib2_generatingProcessIdentifier,
    grib2_hoursAfterDataCutoff,
    grib2_minutesAfterDataCutoff,
    grib2_indicatorOfUnitOfTimeRange,
    grib2_forecastTime,
    grib2_typeOfFirstFixedSurface,
    grib2_firstFixedSurface,
    grib2_typeOfSecondFixedSurface,
    grib2_secondFixedSurface,
    grib2_coordinateValues,
    grib2_coordinateValues_array,
    grib2_gridDefinitionTemplateNumber,
    grib2_bitsPerValue,
    grib2_binaryScaleFactor,
    grib2_referenceValue,
    grib2_values,
    grib2_grid,
    grib2_data,
    grib2_local_array,
    grib2_grid_array,
    grib2_data_array,
    grib2_message,

    grib_message,
    grib_root,

    num_grib_types
};

static THREAD_LOCAL coda_type **grib_type = NULL;

static int grib_init(void)
{
    coda_endianness endianness;
    coda_type_record_field *field;
    coda_type *basic_type;
    int i;

    if (grib_type != NULL)
    {
        return 0;
    }

#ifdef WORDS_BIGENDIAN
    endianness = coda_big_endian;
#else
    endianness = coda_little_endian;
#endif

    grib_type = malloc(num_grib_types * sizeof(coda_type *));
    if (grib_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_grib_types * sizeof(coda_type *), __FILE__, __LINE__);
        return -1;
    }
    for (i = 0; i < num_grib_types; i++)
    {
        grib_type[i] = NULL;
    }

    /* GRIB1 */

    grib_type[grib1_localRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_localRecordIndex], endianness);
    coda_type_set_read_type(grib_type[grib1_localRecordIndex], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib1_localRecordIndex], 32);

    grib_type[grib1_gridRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_gridRecordIndex], endianness);
    coda_type_set_read_type(grib_type[grib1_gridRecordIndex], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_gridRecordIndex], 32);

    grib_type[grib1_table2Version] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_table2Version], endianness);
    coda_type_set_read_type(grib_type[grib1_table2Version], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_table2Version], 8);
    coda_type_set_description(grib_type[grib1_table2Version],
                              "Parameter Table Version number, currently 3 for international exchange. "
                              "(Parameter table version numbers 128-254 are reserved for local use.)");

    grib_type[grib1_editionNumber] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_editionNumber], endianness);
    coda_type_set_read_type(grib_type[grib1_editionNumber], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_editionNumber], 8);
    coda_type_set_description(grib_type[grib1_editionNumber], "GRIB edition number");

    grib_type[grib1_centre] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_centre], endianness);
    coda_type_set_read_type(grib_type[grib1_centre], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_centre], 8);
    coda_type_set_description(grib_type[grib1_centre], "Identification of center");

    grib_type[grib1_generatingProcessIdentifier] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_generatingProcessIdentifier], endianness);
    coda_type_set_read_type(grib_type[grib1_generatingProcessIdentifier], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_generatingProcessIdentifier], 8);
    coda_type_set_description(grib_type[grib1_generatingProcessIdentifier], "Generating process ID number");

    grib_type[grib1_gridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_gridDefinition], endianness);
    coda_type_set_read_type(grib_type[grib1_gridDefinition], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_gridDefinition], 8);
    coda_type_set_description(grib_type[grib1_gridDefinition], "Grid Identification");

    grib_type[grib1_indicatorOfParameter] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_indicatorOfParameter], endianness);
    coda_type_set_read_type(grib_type[grib1_indicatorOfParameter], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_indicatorOfParameter], 8);
    coda_type_set_description(grib_type[grib1_indicatorOfParameter], "Indicator of parameter and units");

    grib_type[grib1_indicatorOfTypeOfLevel] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_indicatorOfTypeOfLevel], endianness);
    coda_type_set_read_type(grib_type[grib1_indicatorOfTypeOfLevel], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_indicatorOfTypeOfLevel], 8);
    coda_type_set_description(grib_type[grib1_indicatorOfTypeOfLevel], "Indicator of type of level or layer");

    grib_type[grib1_level] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_level], endianness);
    coda_type_set_read_type(grib_type[grib1_level], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_level], 16);
    coda_type_set_description(grib_type[grib1_level], "Height, pressure, etc. of the level or layer");

    grib_type[grib1_yearOfCentury] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_yearOfCentury], endianness);
    coda_type_set_read_type(grib_type[grib1_yearOfCentury], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_yearOfCentury], 8);
    coda_type_set_description(grib_type[grib1_yearOfCentury], "Year of century");

    grib_type[grib1_year] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_year], endianness);
    coda_type_set_read_type(grib_type[grib1_year], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_year], 16);
    coda_type_set_description(grib_type[grib1_year], "Year");

    grib_type[grib1_month] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_month], endianness);
    coda_type_set_read_type(grib_type[grib1_month], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_month], 8);
    coda_type_set_description(grib_type[grib1_month], "Month of year");

    grib_type[grib1_day] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_day], endianness);
    coda_type_set_read_type(grib_type[grib1_day], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_day], 8);
    coda_type_set_description(grib_type[grib1_day], "Day of month");

    grib_type[grib1_hour] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_hour], endianness);
    coda_type_set_read_type(grib_type[grib1_hour], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_hour], 8);
    coda_type_set_description(grib_type[grib1_hour], "Hour of day");

    grib_type[grib1_minute] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_minute], endianness);
    coda_type_set_read_type(grib_type[grib1_minute], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_minute], 8);
    coda_type_set_description(grib_type[grib1_minute], "Minute of hour");

    grib_type[grib1_second] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_second], endianness);
    coda_type_set_read_type(grib_type[grib1_second], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_second], 8);
    coda_type_set_description(grib_type[grib1_second], "Second of minute");

    grib_type[grib1_unitOfTimeRange] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_unitOfTimeRange], endianness);
    coda_type_set_read_type(grib_type[grib1_unitOfTimeRange], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_unitOfTimeRange], 8);
    coda_type_set_description(grib_type[grib1_unitOfTimeRange], "Forecast time unit");

    grib_type[grib1_P1] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_P1], endianness);
    coda_type_set_read_type(grib_type[grib1_P1], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_P1], 8);
    coda_type_set_description(grib_type[grib1_P1], "Period of time (Number of time units)");

    grib_type[grib1_P2] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_P2], endianness);
    coda_type_set_read_type(grib_type[grib1_P2], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_P2], 8);
    coda_type_set_description(grib_type[grib1_P2], "Period of time (Number of time units)");

    grib_type[grib1_timeRangeIndicator] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_timeRangeIndicator], endianness);
    coda_type_set_read_type(grib_type[grib1_timeRangeIndicator], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_timeRangeIndicator], 8);
    coda_type_set_description(grib_type[grib1_timeRangeIndicator], "Time range indicator");

    grib_type[grib1_numberIncludedInAverage] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_numberIncludedInAverage], endianness);
    coda_type_set_read_type(grib_type[grib1_numberIncludedInAverage], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_numberIncludedInAverage], 16);
    coda_type_set_description(grib_type[grib1_numberIncludedInAverage], "Number included in average, when "
                              "timeRangeIndicator indicates an average or accumulation; otherwise set to zero.");

    grib_type[grib1_numberMissingFromAveragesOrAccumulations] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                                  coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_numberMissingFromAveragesOrAccumulations],
                                    endianness);
    coda_type_set_read_type(grib_type[grib1_numberMissingFromAveragesOrAccumulations], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_numberMissingFromAveragesOrAccumulations], 8);
    coda_type_set_description(grib_type[grib1_numberMissingFromAveragesOrAccumulations], "Number Missing from "
                              "averages or accumulations.");

    grib_type[grib1_centuryOfReferenceTimeOfData] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                      coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_centuryOfReferenceTimeOfData], endianness);
    coda_type_set_read_type(grib_type[grib1_centuryOfReferenceTimeOfData], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_centuryOfReferenceTimeOfData], 8);
    coda_type_set_description(grib_type[grib1_centuryOfReferenceTimeOfData], "Century of Initial (Reference) time "
                              "(=20 until Jan. 1, 2001)");

    grib_type[grib1_subCentre] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_subCentre], endianness);
    coda_type_set_read_type(grib_type[grib1_subCentre], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_subCentre], 8);
    coda_type_set_description(grib_type[grib1_subCentre], "Identification of sub-center (allocated by the originating "
                              "center; See Table C)");

    grib_type[grib1_decimalScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_decimalScaleFactor], endianness);
    coda_type_set_read_type(grib_type[grib1_decimalScaleFactor], coda_native_type_int16);
    coda_type_set_bit_size(grib_type[grib1_decimalScaleFactor], 16);
    coda_type_set_description(grib_type[grib1_decimalScaleFactor], "The decimal scale factor D");

    grib_type[grib1_discipline] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_discipline], endianness);
    coda_type_set_read_type(grib_type[grib1_discipline], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_discipline], 8);
    coda_type_set_description(grib_type[grib1_discipline], "GRIB Master Table Number");

    grib_type[grib1_masterTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_masterTablesVersion], endianness);
    coda_type_set_read_type(grib_type[grib1_masterTablesVersion], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_masterTablesVersion], 8);
    coda_type_set_description(grib_type[grib1_masterTablesVersion], "GRIB Master Tables Version Number");

    grib_type[grib1_localTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_localTablesVersion], endianness);
    coda_type_set_read_type(grib_type[grib1_localTablesVersion], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_localTablesVersion], 8);
    coda_type_set_description(grib_type[grib1_localTablesVersion], "GRIB Local Tables Version Number");

    grib_type[grib1_significanceOfReferenceTime] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_significanceOfReferenceTime], endianness);
    coda_type_set_read_type(grib_type[grib1_significanceOfReferenceTime], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_significanceOfReferenceTime], 8);
    coda_type_set_description(grib_type[grib1_significanceOfReferenceTime], "Significance of Reference Time");

    grib_type[grib1_productionStatusOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                         coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_productionStatusOfProcessedData], endianness);
    coda_type_set_read_type(grib_type[grib1_productionStatusOfProcessedData], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_productionStatusOfProcessedData], 8);
    coda_type_set_description(grib_type[grib1_productionStatusOfProcessedData],
                              "Production status of processed data in this GRIB message");

    grib_type[grib1_typeOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_typeOfProcessedData], endianness);
    coda_type_set_read_type(grib_type[grib1_typeOfProcessedData], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_typeOfProcessedData], 8);
    coda_type_set_description(grib_type[grib1_typeOfProcessedData], "Type of processed data in this GRIB message");

    grib_type[grib1_local] = (coda_type *)coda_type_raw_new(coda_format_grib);
    coda_type_set_description(grib_type[grib1_local], "Reserved for originating center use");

    grib_type[grib1_numberOfVerticalCoordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                          coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_numberOfVerticalCoordinateValues], endianness);
    coda_type_set_read_type(grib_type[grib1_numberOfVerticalCoordinateValues], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_numberOfVerticalCoordinateValues], 8);
    coda_type_set_description(grib_type[grib1_numberOfVerticalCoordinateValues],
                              "NV, the number of vertical coordinate parameter");

    grib_type[grib1_dataRepresentationType] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_dataRepresentationType], endianness);
    coda_type_set_read_type(grib_type[grib1_dataRepresentationType], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_dataRepresentationType], 8);
    coda_type_set_description(grib_type[grib1_dataRepresentationType], "Data representation type");

    grib_type[grib1_shapeOfTheEarth] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_shapeOfTheEarth], endianness);
    coda_type_set_read_type(grib_type[grib1_shapeOfTheEarth], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_shapeOfTheEarth], 8);

    grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                             coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth],
                                    endianness);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth], 8);

    grib_type[grib1_scaledValueOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                             coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaledValueOfRadiusOfSphericalEarth],
                                    endianness);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfRadiusOfSphericalEarth], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_scaledValueOfRadiusOfSphericalEarth], 32);

    grib_type[grib1_scaleFactorOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaleFactorOfEarthMajorAxis], endianness);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfEarthMajorAxis], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_scaleFactorOfEarthMajorAxis], 8);

    grib_type[grib1_scaledValueOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaledValueOfEarthMajorAxis], endianness);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfEarthMajorAxis], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_scaledValueOfEarthMajorAxis], 32);

    grib_type[grib1_scaleFactorOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaleFactorOfEarthMinorAxis], endianness);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfEarthMinorAxis], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_scaleFactorOfEarthMinorAxis], 8);

    grib_type[grib1_scaledValueOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scaledValueOfEarthMinorAxis], endianness);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfEarthMinorAxis], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_scaledValueOfEarthMinorAxis], 32);

    grib_type[grib1_Ni] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_Ni], endianness);
    coda_type_set_read_type(grib_type[grib1_Ni], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_Ni], 16);
    coda_type_set_description(grib_type[grib1_Ni], "No. of points along a latitude circle");

    grib_type[grib1_Nj] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_Nj], endianness);
    coda_type_set_read_type(grib_type[grib1_Nj], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_Nj], 16);
    coda_type_set_description(grib_type[grib1_Nj], "No. of points along a longitude meridian");

    grib_type[grib1_basicAngleOfTheInitialProductionDomain] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                                coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_basicAngleOfTheInitialProductionDomain],
                                    endianness);
    coda_type_set_read_type(grib_type[grib1_basicAngleOfTheInitialProductionDomain], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_basicAngleOfTheInitialProductionDomain], 32);

    grib_type[grib1_subdivisionsOfBasicAngle] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_subdivisionsOfBasicAngle], endianness);
    coda_type_set_read_type(grib_type[grib1_subdivisionsOfBasicAngle], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_subdivisionsOfBasicAngle], 32);

    grib_type[grib1_latitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_latitudeOfFirstGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib1_latitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib1_latitudeOfFirstGridPoint], 32);
    coda_type_set_description(grib_type[grib1_latitudeOfFirstGridPoint], "La1 - latitude of first grid point, units: "
                              "millidegrees (degrees x 1000), values limited to range 0 - 90,000");

    grib_type[grib1_longitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                   coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_longitudeOfFirstGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib1_longitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib1_longitudeOfFirstGridPoint], 32);
    coda_type_set_description(grib_type[grib1_longitudeOfFirstGridPoint], "Lo1 - longitude of first grid point, "
                              "units: millidegrees (degrees x 1000), values limited to range 0 - 360,000");

    grib_type[grib1_resolutionAndComponentFlags] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_resolutionAndComponentFlags], endianness);
    coda_type_set_read_type(grib_type[grib1_resolutionAndComponentFlags], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_resolutionAndComponentFlags], 8);
    coda_type_set_description(grib_type[grib1_resolutionAndComponentFlags], "Resolution and component flags");

    grib_type[grib1_latitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_latitudeOfLastGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib1_latitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib1_latitudeOfLastGridPoint], 32);
    coda_type_set_description(grib_type[grib1_latitudeOfLastGridPoint], "La2 - Latitude of last grid point (same "
                              "units and value range as latitudeOfFirstGridPoint)");

    grib_type[grib1_longitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_longitudeOfLastGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib1_longitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib1_longitudeOfLastGridPoint], 32);
    coda_type_set_description(grib_type[grib1_longitudeOfLastGridPoint], "Lo2 - Longitude of last grid point (same "
                              "units and value range as longitudeOfFirstGridPoint)");

    grib_type[grib1_iDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_iDirectionIncrement], endianness);
    coda_type_set_read_type(grib_type[grib1_iDirectionIncrement], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_iDirectionIncrement], 16);
    coda_type_set_description(grib_type[grib1_iDirectionIncrement], "Di - Longitudinal Direction Increment (same "
                              "units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib1_jDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_jDirectionIncrement], endianness);
    coda_type_set_read_type(grib_type[grib1_jDirectionIncrement], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_jDirectionIncrement], 16);
    coda_type_set_description(grib_type[grib1_jDirectionIncrement], "Dj - Latitudinal Direction Increment (same "
                              "units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib1_N] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_N], endianness);
    coda_type_set_read_type(grib_type[grib1_N], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_N], 16);
    coda_type_set_description(grib_type[grib1_N], "N - number of latitude circles between a pole and the equator, "
                              "Mandatory if Gaussian Grid specified");

    grib_type[grib1_scanningMode] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_scanningMode], endianness);
    coda_type_set_read_type(grib_type[grib1_scanningMode], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_scanningMode], 8);
    coda_type_set_description(grib_type[grib1_scanningMode], "Scanning mode flags");

    grib_type[grib1_coordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_coordinateValues], endianness);
    coda_type_set_read_type(grib_type[grib1_coordinateValues], coda_native_type_float);
    coda_type_set_bit_size(grib_type[grib1_coordinateValues], 32);
    grib_type[grib1_coordinateValues_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_set_description(grib_type[grib1_coordinateValues_array], "List of vertical coordinate parameters");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_coordinateValues_array],
                                  grib_type[grib1_coordinateValues]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_coordinateValues_array], NULL);

    grib_type[grib1_listOfNumbers] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_listOfNumbers], endianness);
    coda_type_set_read_type(grib_type[grib1_listOfNumbers], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_listOfNumbers], 16);
    grib_type[grib1_listOfNumbers_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_set_description(grib_type[grib1_listOfNumbers_array],
                              "List of numbers of points for each row, used for quasi-regular grids");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_listOfNumbers_array],
                                  grib_type[grib1_listOfNumbers]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_listOfNumbers_array], NULL);

    grib_type[grib1_sourceOfGridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_sourceOfGridDefinition], endianness);
    coda_type_set_read_type(grib_type[grib1_sourceOfGridDefinition], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_sourceOfGridDefinition], 8);
    coda_type_set_description(grib_type[grib1_sourceOfGridDefinition], "Source of grid definition");

    grib_type[grib1_numberOfDataPoints] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_numberOfDataPoints], endianness);
    coda_type_set_read_type(grib_type[grib1_numberOfDataPoints], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib1_numberOfDataPoints], 32);
    coda_type_set_description(grib_type[grib1_numberOfDataPoints], "Number of data points");

    grib_type[grib1_gridDefinitionTemplateNumber] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                      coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_gridDefinitionTemplateNumber], endianness);
    coda_type_set_read_type(grib_type[grib1_gridDefinitionTemplateNumber], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib1_gridDefinitionTemplateNumber], 16);
    coda_type_set_description(grib_type[grib1_gridDefinitionTemplateNumber], "Grid Definition Template Number");

    grib_type[grib1_bitsPerValue] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_bitsPerValue], endianness);
    coda_type_set_read_type(grib_type[grib1_bitsPerValue], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib1_bitsPerValue], 8);
    coda_type_set_description(grib_type[grib1_bitsPerValue], "Number of bits into which a datum point is packed.");

    grib_type[grib1_binaryScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_binaryScaleFactor], endianness);
    coda_type_set_read_type(grib_type[grib1_binaryScaleFactor], coda_native_type_int16);
    coda_type_set_bit_size(grib_type[grib1_binaryScaleFactor], 16);
    coda_type_set_description(grib_type[grib1_binaryScaleFactor], "The binary scale factor (E).");

    grib_type[grib1_referenceValue] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib1_referenceValue], endianness);
    coda_type_set_read_type(grib_type[grib1_referenceValue], coda_native_type_float);
    coda_type_set_bit_size(grib_type[grib1_referenceValue], 32);
    coda_type_set_description(grib_type[grib1_referenceValue], "Reference value (minimum value). "
                              "This is the overall or 'global' minimum that has been subtracted from all the values.");

    grib_type[grib1_values] = (coda_type *)coda_type_array_new(coda_format_grib);
    basic_type = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)basic_type, endianness);
    coda_type_set_read_type(basic_type, coda_native_type_float);
    coda_type_set_bit_size(basic_type, 32);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_values], basic_type);
    coda_type_release(basic_type);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_values], NULL);

    grib_type[grib1_grid] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("numberOfVerticalCoordinateValues");
    coda_type_record_field_set_type(field, grib_type[grib1_numberOfVerticalCoordinateValues]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("dataRepresentationType");
    coda_type_record_field_set_type(field, grib_type[grib1_dataRepresentationType]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("Ni");
    coda_type_record_field_set_type(field, grib_type[grib1_Ni]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("Nj");
    coda_type_record_field_set_type(field, grib_type[grib1_Nj]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("latitudeOfFirstGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib1_latitudeOfFirstGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("longitudeOfFirstGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib1_longitudeOfFirstGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("resolutionAndComponentFlags");
    coda_type_record_field_set_type(field, grib_type[grib1_resolutionAndComponentFlags]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("latitudeOfLastGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib1_latitudeOfLastGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("longitudeOfLastGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib1_longitudeOfLastGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("iDirectionIncrement");
    coda_type_record_field_set_type(field, grib_type[grib1_iDirectionIncrement]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("jDirectionIncrement");
    coda_type_record_field_set_type(field, grib_type[grib1_jDirectionIncrement]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("N");
    coda_type_record_field_set_type(field, grib_type[grib1_N]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("scanningMode");
    coda_type_record_field_set_type(field, grib_type[grib1_scanningMode]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("coordinateValues");
    coda_type_record_field_set_type(field, grib_type[grib1_coordinateValues_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);
    field = coda_type_record_field_new("listOfNumbers");
    coda_type_record_field_set_type(field, grib_type[grib1_listOfNumbers_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);

    grib_type[grib1_data] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("bitsPerValue");
    coda_type_record_field_set_type(field, grib_type[grib1_bitsPerValue]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_data], field);
    field = coda_type_record_field_new("binaryScaleFactor");
    coda_type_record_field_set_type(field, grib_type[grib1_binaryScaleFactor]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_data], field);
    field = coda_type_record_field_new("referenceValue");
    coda_type_record_field_set_type(field, grib_type[grib1_referenceValue]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_data], field);
    field = coda_type_record_field_new("values");
    coda_type_record_field_set_type(field, grib_type[grib1_values]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_data], field);

    grib_type[grib1_message] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("editionNumber");
    coda_type_record_field_set_type(field, grib_type[grib1_editionNumber]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("table2Version");
    coda_type_record_field_set_type(field, grib_type[grib1_table2Version]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("centre");
    coda_type_record_field_set_type(field, grib_type[grib1_centre]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("generatingProcessIdentifier");
    coda_type_record_field_set_type(field, grib_type[grib1_generatingProcessIdentifier]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("gridDefinition");
    coda_type_record_field_set_type(field, grib_type[grib1_gridDefinition]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("indicatorOfParameter");
    coda_type_record_field_set_type(field, grib_type[grib1_indicatorOfParameter]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("indicatorOfTypeOfLevel");
    coda_type_record_field_set_type(field, grib_type[grib1_indicatorOfTypeOfLevel]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("level");
    coda_type_record_field_set_type(field, grib_type[grib1_level]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("yearOfCentury");
    coda_type_record_field_set_type(field, grib_type[grib1_yearOfCentury]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("month");
    coda_type_record_field_set_type(field, grib_type[grib1_month]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("day");
    coda_type_record_field_set_type(field, grib_type[grib1_day]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("hour");
    coda_type_record_field_set_type(field, grib_type[grib1_hour]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("minute");
    coda_type_record_field_set_type(field, grib_type[grib1_minute]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("unitOfTimeRange");
    coda_type_record_field_set_type(field, grib_type[grib1_unitOfTimeRange]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("P1");
    coda_type_record_field_set_type(field, grib_type[grib1_P1]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("P2");
    coda_type_record_field_set_type(field, grib_type[grib1_P2]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("timeRangeIndicator");
    coda_type_record_field_set_type(field, grib_type[grib1_timeRangeIndicator]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("numberIncludedInAverage");
    coda_type_record_field_set_type(field, grib_type[grib1_numberIncludedInAverage]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("numberMissingFromAveragesOrAccumulations");
    coda_type_record_field_set_type(field, grib_type[grib1_numberMissingFromAveragesOrAccumulations]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("centuryOfReferenceTimeOfData");
    coda_type_record_field_set_type(field, grib_type[grib1_centuryOfReferenceTimeOfData]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("subCentre");
    coda_type_record_field_set_type(field, grib_type[grib1_subCentre]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("decimalScaleFactor");
    coda_type_record_field_set_type(field, grib_type[grib1_decimalScaleFactor]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("local");
    coda_type_record_field_set_type(field, grib_type[grib1_local]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("grid");
    coda_type_record_field_set_type(field, grib_type[grib1_grid]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);
    field = coda_type_record_field_new("data");
    coda_type_record_field_set_type(field, grib_type[grib1_data]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_message], field);

    /* GRIB2 */

    grib_type[grib2_localRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_localRecordIndex], endianness);
    coda_type_set_read_type(grib_type[grib2_localRecordIndex], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib2_localRecordIndex], 32);

    grib_type[grib2_gridRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_gridRecordIndex], endianness);
    coda_type_set_read_type(grib_type[grib2_gridRecordIndex], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_gridRecordIndex], 32);

    grib_type[grib2_editionNumber] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_editionNumber], endianness);
    coda_type_set_read_type(grib_type[grib2_editionNumber], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_editionNumber], 8);
    coda_type_set_description(grib_type[grib2_editionNumber], "GRIB edition number");

    grib_type[grib2_centre] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_centre], endianness);
    coda_type_set_read_type(grib_type[grib2_centre], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_centre], 16);
    coda_type_set_description(grib_type[grib2_centre], "Identification of originating/generating centre");

    grib_type[grib2_year] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_year], endianness);
    coda_type_set_read_type(grib_type[grib2_year], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_year], 16);
    coda_type_set_description(grib_type[grib2_year], "Year");

    grib_type[grib2_month] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_month], endianness);
    coda_type_set_read_type(grib_type[grib2_month], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_month], 8);
    coda_type_set_description(grib_type[grib2_month], "Month of year");

    grib_type[grib2_day] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_day], endianness);
    coda_type_set_read_type(grib_type[grib2_day], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_day], 8);
    coda_type_set_description(grib_type[grib2_day], "Day of month");

    grib_type[grib2_hour] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_hour], endianness);
    coda_type_set_read_type(grib_type[grib2_hour], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_hour], 8);
    coda_type_set_description(grib_type[grib2_hour], "Hour of day");

    grib_type[grib2_minute] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_minute], endianness);
    coda_type_set_read_type(grib_type[grib2_minute], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_minute], 8);
    coda_type_set_description(grib_type[grib2_minute], "Minute of hour");

    grib_type[grib2_second] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_second], endianness);
    coda_type_set_read_type(grib_type[grib2_second], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_second], 8);
    coda_type_set_description(grib_type[grib2_second], "Second of minute");

    grib_type[grib2_subCentre] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_subCentre], endianness);
    coda_type_set_read_type(grib_type[grib2_subCentre], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_subCentre], 16);
    coda_type_set_description(grib_type[grib2_subCentre], "Identification of originating/generating sub-centre "
                              "(allocated by originating/generating centre)");

    grib_type[grib2_decimalScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_decimalScaleFactor], endianness);
    coda_type_set_read_type(grib_type[grib2_decimalScaleFactor], coda_native_type_int16);
    coda_type_set_bit_size(grib_type[grib2_decimalScaleFactor], 16);
    coda_type_set_description(grib_type[grib2_decimalScaleFactor], "The decimal scale factor D");

    grib_type[grib2_discipline] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_discipline], endianness);
    coda_type_set_read_type(grib_type[grib2_discipline], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_discipline], 8);
    coda_type_set_description(grib_type[grib2_discipline], "GRIB Master Table Number");

    grib_type[grib2_masterTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_masterTablesVersion], endianness);
    coda_type_set_read_type(grib_type[grib2_masterTablesVersion], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_masterTablesVersion], 8);
    coda_type_set_description(grib_type[grib2_masterTablesVersion], "GRIB Master Tables Version Number");

    grib_type[grib2_localTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_localTablesVersion], endianness);
    coda_type_set_read_type(grib_type[grib2_localTablesVersion], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_localTablesVersion], 8);
    coda_type_set_description(grib_type[grib2_localTablesVersion], "GRIB Local Tables Version Number");

    grib_type[grib2_significanceOfReferenceTime] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_significanceOfReferenceTime], endianness);
    coda_type_set_read_type(grib_type[grib2_significanceOfReferenceTime], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_significanceOfReferenceTime], 8);
    coda_type_set_description(grib_type[grib2_significanceOfReferenceTime], "Significance of Reference Time");

    grib_type[grib2_productionStatusOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                         coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_productionStatusOfProcessedData], endianness);
    coda_type_set_read_type(grib_type[grib2_productionStatusOfProcessedData], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_productionStatusOfProcessedData], 8);
    coda_type_set_description(grib_type[grib2_productionStatusOfProcessedData],
                              "Production status of processed data in this GRIB message");

    grib_type[grib2_typeOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_typeOfProcessedData], endianness);
    coda_type_set_read_type(grib_type[grib2_typeOfProcessedData], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_typeOfProcessedData], 8);
    coda_type_set_description(grib_type[grib2_typeOfProcessedData], "Type of processed data in this GRIB message");

    grib_type[grib2_local] = (coda_type *)coda_type_raw_new(coda_format_grib);
    coda_type_set_description(grib_type[grib2_local], "Reserved for originating center use");

    grib_type[grib2_numberOfVerticalCoordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                          coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_numberOfVerticalCoordinateValues], endianness);
    coda_type_set_read_type(grib_type[grib2_numberOfVerticalCoordinateValues], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_numberOfVerticalCoordinateValues], 8);
    coda_type_set_description(grib_type[grib2_numberOfVerticalCoordinateValues],
                              "NV, the number of vertical coordinate parameter");

    grib_type[grib2_dataRepresentationType] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_dataRepresentationType], endianness);
    coda_type_set_read_type(grib_type[grib2_dataRepresentationType], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_dataRepresentationType], 8);
    coda_type_set_description(grib_type[grib2_dataRepresentationType], "Data representation type");

    grib_type[grib2_shapeOfTheEarth] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_shapeOfTheEarth], endianness);
    coda_type_set_read_type(grib_type[grib2_shapeOfTheEarth], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_shapeOfTheEarth], 8);

    grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                             coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth],
                                    endianness);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth], 8);

    grib_type[grib2_scaledValueOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                             coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaledValueOfRadiusOfSphericalEarth],
                                    endianness);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfRadiusOfSphericalEarth], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_scaledValueOfRadiusOfSphericalEarth], 32);

    grib_type[grib2_scaleFactorOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaleFactorOfEarthMajorAxis], endianness);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfEarthMajorAxis], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_scaleFactorOfEarthMajorAxis], 8);

    grib_type[grib2_scaledValueOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaledValueOfEarthMajorAxis], endianness);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfEarthMajorAxis], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_scaledValueOfEarthMajorAxis], 32);

    grib_type[grib2_scaleFactorOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaleFactorOfEarthMinorAxis], endianness);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfEarthMinorAxis], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_scaleFactorOfEarthMinorAxis], 8);

    grib_type[grib2_scaledValueOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scaledValueOfEarthMinorAxis], endianness);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfEarthMinorAxis], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_scaledValueOfEarthMinorAxis], 32);

    grib_type[grib2_Ni] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_Ni], endianness);
    coda_type_set_read_type(grib_type[grib2_Ni], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_Ni], 32);
    coda_type_set_description(grib_type[grib2_Ni], "No. of points along a latitude circle");

    grib_type[grib2_Nj] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_Nj], endianness);
    coda_type_set_read_type(grib_type[grib2_Nj], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_Nj], 32);
    coda_type_set_description(grib_type[grib2_Nj], "No. of points along a longitude meridian");

    grib_type[grib2_basicAngleOfTheInitialProductionDomain] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                                coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_basicAngleOfTheInitialProductionDomain],
                                    endianness);
    coda_type_set_read_type(grib_type[grib2_basicAngleOfTheInitialProductionDomain], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_basicAngleOfTheInitialProductionDomain], 32);

    grib_type[grib2_subdivisionsOfBasicAngle] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_subdivisionsOfBasicAngle], endianness);
    coda_type_set_read_type(grib_type[grib2_subdivisionsOfBasicAngle], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_subdivisionsOfBasicAngle], 32);

    grib_type[grib2_latitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_latitudeOfFirstGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib2_latitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib2_latitudeOfFirstGridPoint], 32);
    coda_type_set_description(grib_type[grib2_latitudeOfFirstGridPoint], "La1 - latitude of first grid point, units: "
                              "millidegrees (degrees x 1000), values limited to range 0 - 90,000");

    grib_type[grib2_longitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                   coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_longitudeOfFirstGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib2_longitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib2_longitudeOfFirstGridPoint], 32);
    coda_type_set_description(grib_type[grib2_longitudeOfFirstGridPoint], "Lo1 - longitude of first grid point, "
                              "units: millidegrees (degrees x 1000), values limited to range 0 - 360,000");

    grib_type[grib2_resolutionAndComponentFlags] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_resolutionAndComponentFlags], endianness);
    coda_type_set_read_type(grib_type[grib2_resolutionAndComponentFlags], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_resolutionAndComponentFlags], 8);
    coda_type_set_description(grib_type[grib2_resolutionAndComponentFlags], "Resolution and component flags");

    grib_type[grib2_latitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_latitudeOfLastGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib2_latitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib2_latitudeOfLastGridPoint], 32);
    coda_type_set_description(grib_type[grib2_latitudeOfLastGridPoint], "La2 - Latitude of last grid point (same "
                              "units and value range as latitudeOfFirstGridPoint)");

    grib_type[grib2_longitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_longitudeOfLastGridPoint], endianness);
    coda_type_set_read_type(grib_type[grib2_longitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_bit_size(grib_type[grib2_longitudeOfLastGridPoint], 32);
    coda_type_set_description(grib_type[grib2_longitudeOfLastGridPoint], "Lo2 - Longitude of last grid point (same "
                              "units and value range as longitudeOfFirstGridPoint)");

    grib_type[grib2_iDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_iDirectionIncrement], endianness);
    coda_type_set_read_type(grib_type[grib2_iDirectionIncrement], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_iDirectionIncrement], 32);
    coda_type_set_description(grib_type[grib2_iDirectionIncrement], "Di - Longitudinal Direction Increment (same "
                              "units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib2_jDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_jDirectionIncrement], endianness);
    coda_type_set_read_type(grib_type[grib2_jDirectionIncrement], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_jDirectionIncrement], 32);
    coda_type_set_description(grib_type[grib2_jDirectionIncrement], "Dj - Latitudinal Direction Increment (same "
                              "units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib2_N] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_N], endianness);
    coda_type_set_read_type(grib_type[grib2_N], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_N], 32);
    coda_type_set_description(grib_type[grib2_N], "N - number of latitude circles between a pole and the equator, "
                              "Mandatory if Gaussian Grid specified");

    grib_type[grib2_scanningMode] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_scanningMode], endianness);
    coda_type_set_read_type(grib_type[grib2_scanningMode], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_scanningMode], 8);
    coda_type_set_description(grib_type[grib2_scanningMode], "Scanning mode flags");

    grib_type[grib2_listOfNumbers] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_listOfNumbers], endianness);
    coda_type_set_read_type(grib_type[grib2_listOfNumbers], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_listOfNumbers], 32);
    grib_type[grib2_listOfNumbers_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_set_description(grib_type[grib2_listOfNumbers_array],
                              "List of numbers of points for each row, used for quasi-regular grids");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_listOfNumbers_array],
                                  grib_type[grib2_listOfNumbers]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_listOfNumbers_array], NULL);

    grib_type[grib2_sourceOfGridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_sourceOfGridDefinition], endianness);
    coda_type_set_read_type(grib_type[grib2_sourceOfGridDefinition], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_sourceOfGridDefinition], 8);
    coda_type_set_description(grib_type[grib2_sourceOfGridDefinition], "Source of grid definition");

    grib_type[grib2_numberOfDataPoints] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_numberOfDataPoints], endianness);
    coda_type_set_read_type(grib_type[grib2_numberOfDataPoints], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_numberOfDataPoints], 32);
    coda_type_set_description(grib_type[grib2_numberOfDataPoints], "Number of data points");

    grib_type[grib2_interpretationOfListOfNumbers] =
        (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_interpretationOfListOfNumbers], endianness);
    coda_type_set_read_type(grib_type[grib2_interpretationOfListOfNumbers], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_interpretationOfListOfNumbers], 8);
    coda_type_set_description(grib_type[grib2_interpretationOfListOfNumbers],
                              "Interpretation of list of numbers defining number of points");

    grib_type[grib2_parameterCategory] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_parameterCategory], endianness);
    coda_type_set_read_type(grib_type[grib2_parameterCategory], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_parameterCategory], 8);
    coda_type_set_description(grib_type[grib2_parameterCategory], "Parameter Category");

    grib_type[grib2_parameterNumber] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_parameterNumber], endianness);
    coda_type_set_read_type(grib_type[grib2_parameterNumber], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_parameterNumber], 8);
    coda_type_set_description(grib_type[grib2_parameterNumber], "Parameter Number");

    grib_type[grib2_constituentType] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_constituentType], endianness);
    coda_type_set_read_type(grib_type[grib2_constituentType], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_constituentType], 16);
    coda_type_set_description(grib_type[grib2_constituentType], "Constituent Number");

    grib_type[grib2_typeOfGeneratingProcess] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_typeOfGeneratingProcess], endianness);
    coda_type_set_read_type(grib_type[grib2_typeOfGeneratingProcess], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_typeOfGeneratingProcess], 8);
    coda_type_set_description(grib_type[grib2_typeOfGeneratingProcess], "Type of generating process");

    grib_type[grib2_backgroundProcess] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_backgroundProcess], endianness);
    coda_type_set_read_type(grib_type[grib2_backgroundProcess], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_backgroundProcess], 8);
    coda_type_set_description(grib_type[grib2_backgroundProcess], "Background generating process identifier");

    grib_type[grib2_generatingProcessIdentifier] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                     coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_generatingProcessIdentifier], endianness);
    coda_type_set_read_type(grib_type[grib2_generatingProcessIdentifier], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_generatingProcessIdentifier], 8);
    coda_type_set_description(grib_type[grib2_generatingProcessIdentifier],
                              "Analysis or forecast generating process identifier");

    grib_type[grib2_hoursAfterDataCutoff] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_hoursAfterDataCutoff], endianness);
    coda_type_set_read_type(grib_type[grib2_hoursAfterDataCutoff], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_hoursAfterDataCutoff], 16);
    coda_type_set_description(grib_type[grib2_hoursAfterDataCutoff],
                              "Hours of observational data cut-off after reference time");

    grib_type[grib2_minutesAfterDataCutoff] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_minutesAfterDataCutoff], endianness);
    coda_type_set_read_type(grib_type[grib2_minutesAfterDataCutoff], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_minutesAfterDataCutoff], 8);
    coda_type_set_description(grib_type[grib2_minutesAfterDataCutoff],
                              "Minutes of observational data cut-off after reference time");

    grib_type[grib2_indicatorOfUnitOfTimeRange] =
        (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_indicatorOfUnitOfTimeRange], endianness);
    coda_type_set_read_type(grib_type[grib2_indicatorOfUnitOfTimeRange], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_indicatorOfUnitOfTimeRange], 8);
    coda_type_set_description(grib_type[grib2_indicatorOfUnitOfTimeRange], "Indicator of unit of time range");

    grib_type[grib2_forecastTime] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_forecastTime], endianness);
    coda_type_set_read_type(grib_type[grib2_forecastTime], coda_native_type_uint32);
    coda_type_set_bit_size(grib_type[grib2_forecastTime], 32);
    coda_type_set_description(grib_type[grib2_forecastTime],
                              "Forecast time in units defined by indicatorOfUnitOfTimeRange");

    grib_type[grib2_typeOfFirstFixedSurface] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_typeOfFirstFixedSurface], endianness);
    coda_type_set_read_type(grib_type[grib2_typeOfFirstFixedSurface], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_typeOfFirstFixedSurface], 8);
    coda_type_set_description(grib_type[grib2_typeOfFirstFixedSurface], "Type of first fixed surface");

    grib_type[grib2_firstFixedSurface] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_firstFixedSurface], endianness);
    coda_type_set_read_type(grib_type[grib2_firstFixedSurface], coda_native_type_double);
    coda_type_set_bit_size(grib_type[grib2_firstFixedSurface], 64);
    coda_type_set_description(grib_type[grib2_firstFixedSurface], "First fixed surface");

    grib_type[grib2_typeOfSecondFixedSurface] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_typeOfSecondFixedSurface], endianness);
    coda_type_set_read_type(grib_type[grib2_typeOfSecondFixedSurface], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_typeOfSecondFixedSurface], 8);
    coda_type_set_description(grib_type[grib2_typeOfSecondFixedSurface], "Type of second fixed surface");

    grib_type[grib2_secondFixedSurface] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_secondFixedSurface], endianness);
    coda_type_set_read_type(grib_type[grib2_secondFixedSurface], coda_native_type_double);
    coda_type_set_bit_size(grib_type[grib2_secondFixedSurface], 64);
    coda_type_set_description(grib_type[grib2_secondFixedSurface], "Second fixed surface");

    grib_type[grib2_coordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_coordinateValues], endianness);
    coda_type_set_read_type(grib_type[grib2_coordinateValues], coda_native_type_float);
    coda_type_set_bit_size(grib_type[grib2_coordinateValues], 32);
    grib_type[grib2_coordinateValues_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_set_description(grib_type[grib2_coordinateValues_array], "List of vertical coordinate parameters");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_coordinateValues_array],
                                  grib_type[grib2_coordinateValues]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_coordinateValues_array], NULL);

    grib_type[grib2_gridDefinitionTemplateNumber] = (coda_type *)coda_type_number_new(coda_format_grib,
                                                                                      coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_gridDefinitionTemplateNumber], endianness);
    coda_type_set_read_type(grib_type[grib2_gridDefinitionTemplateNumber], coda_native_type_uint16);
    coda_type_set_bit_size(grib_type[grib2_gridDefinitionTemplateNumber], 16);
    coda_type_set_description(grib_type[grib2_gridDefinitionTemplateNumber], "Grid Definition Template Number");

    grib_type[grib2_bitsPerValue] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_bitsPerValue], endianness);
    coda_type_set_read_type(grib_type[grib2_bitsPerValue], coda_native_type_uint8);
    coda_type_set_bit_size(grib_type[grib2_bitsPerValue], 8);
    coda_type_set_description(grib_type[grib2_bitsPerValue], "Number of bits into which a datum point is packed.");

    grib_type[grib2_binaryScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib, coda_integer_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_binaryScaleFactor], endianness);
    coda_type_set_read_type(grib_type[grib2_binaryScaleFactor], coda_native_type_int16);
    coda_type_set_bit_size(grib_type[grib2_binaryScaleFactor], 16);
    coda_type_set_description(grib_type[grib2_binaryScaleFactor], "The binary scale factor (E).");

    grib_type[grib2_referenceValue] = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)grib_type[grib2_referenceValue], endianness);
    coda_type_set_read_type(grib_type[grib2_referenceValue], coda_native_type_float);
    coda_type_set_bit_size(grib_type[grib2_referenceValue], 32);
    coda_type_set_description(grib_type[grib2_referenceValue], "Reference value (minimum value). "
                              "This is the overall or 'global' minimum that has been subtracted from all the values.");

    grib_type[grib2_values] = (coda_type *)coda_type_array_new(coda_format_grib);
    basic_type = (coda_type *)coda_type_number_new(coda_format_grib, coda_real_class);
    coda_type_number_set_endianness((coda_type_number *)basic_type, endianness);
    coda_type_set_read_type(basic_type, coda_native_type_float);
    coda_type_set_bit_size(basic_type, 32);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_values], basic_type);
    coda_type_release(basic_type);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_values], NULL);

    grib_type[grib2_grid] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("localRecordIndex");
    coda_type_record_field_set_type(field, grib_type[grib2_localRecordIndex]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("sourceOfGridDefinition");
    coda_type_record_field_set_type(field, grib_type[grib2_sourceOfGridDefinition]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("numberOfDataPoints");
    coda_type_record_field_set_type(field, grib_type[grib2_numberOfDataPoints]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("interpretationOfListOfNumbers");
    coda_type_record_field_set_type(field, grib_type[grib2_interpretationOfListOfNumbers]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("gridDefinitionTemplateNumber");
    coda_type_record_field_set_type(field, grib_type[grib2_gridDefinitionTemplateNumber]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("shapeOfTheEarth");
    coda_type_record_field_set_type(field, grib_type[grib2_shapeOfTheEarth]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaleFactorOfRadiusOfSphericalEarth");
    coda_type_record_field_set_type(field, grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaledValueOfRadiusOfSphericalEarth");
    coda_type_record_field_set_type(field, grib_type[grib2_scaledValueOfRadiusOfSphericalEarth]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaleFactorOfEarthMajorAxis");
    coda_type_record_field_set_type(field, grib_type[grib2_scaleFactorOfEarthMajorAxis]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaledValueOfEarthMajorAxis");
    coda_type_record_field_set_type(field, grib_type[grib2_scaledValueOfEarthMajorAxis]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaleFactorOfEarthMinorAxis");
    coda_type_record_field_set_type(field, grib_type[grib2_scaleFactorOfEarthMinorAxis]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scaledValueOfEarthMinorAxis");
    coda_type_record_field_set_type(field, grib_type[grib2_scaledValueOfEarthMinorAxis]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("Ni");
    coda_type_record_field_set_type(field, grib_type[grib2_Ni]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("Nj");
    coda_type_record_field_set_type(field, grib_type[grib2_Nj]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("basicAngleOfTheInitialProductionDomain");
    coda_type_record_field_set_type(field, grib_type[grib2_basicAngleOfTheInitialProductionDomain]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("subdivisionsOfBasicAngle");
    coda_type_record_field_set_type(field, grib_type[grib2_subdivisionsOfBasicAngle]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("latitudeOfFirstGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib2_latitudeOfFirstGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("longitudeOfFirstGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib2_longitudeOfFirstGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("resolutionAndComponentFlags");
    coda_type_record_field_set_type(field, grib_type[grib2_resolutionAndComponentFlags]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("latitudeOfLastGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib2_latitudeOfLastGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("longitudeOfLastGridPoint");
    coda_type_record_field_set_type(field, grib_type[grib2_longitudeOfLastGridPoint]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("iDirectionIncrement");
    coda_type_record_field_set_type(field, grib_type[grib2_iDirectionIncrement]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("jDirectionIncrement");
    coda_type_record_field_set_type(field, grib_type[grib2_jDirectionIncrement]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("N");
    coda_type_record_field_set_type(field, grib_type[grib2_N]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("scanningMode");
    coda_type_record_field_set_type(field, grib_type[grib2_scanningMode]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("listOfNumbers");
    coda_type_record_field_set_type(field, grib_type[grib2_listOfNumbers_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);

    grib_type[grib2_data] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("gridRecordIndex");
    coda_type_record_field_set_type(field, grib_type[grib2_gridRecordIndex]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("parameterCategory");
    coda_type_record_field_set_type(field, grib_type[grib2_parameterCategory]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("parameterNumber");
    coda_type_record_field_set_type(field, grib_type[grib2_parameterNumber]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("constituentType");
    coda_type_record_field_set_type(field, grib_type[grib2_constituentType]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    coda_type_record_field_set_optional(field);
    field = coda_type_record_field_new("typeOfGeneratingProcess");
    coda_type_record_field_set_type(field, grib_type[grib2_typeOfGeneratingProcess]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("backgroundProcess");
    coda_type_record_field_set_type(field, grib_type[grib2_backgroundProcess]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("generatingProcessIdentifier");
    coda_type_record_field_set_type(field, grib_type[grib2_generatingProcessIdentifier]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("hoursAfterDataCutoff");
    coda_type_record_field_set_type(field, grib_type[grib2_hoursAfterDataCutoff]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("minutesAfterDataCutoff");
    coda_type_record_field_set_type(field, grib_type[grib2_minutesAfterDataCutoff]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("indicatorOfUnitOfTimeRange");
    coda_type_record_field_set_type(field, grib_type[grib2_indicatorOfUnitOfTimeRange]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("forecastTime");
    coda_type_record_field_set_type(field, grib_type[grib2_forecastTime]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("typeOfFirstFixedSurface");
    coda_type_record_field_set_type(field, grib_type[grib2_typeOfFirstFixedSurface]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("firstFixedSurface");
    coda_type_record_field_set_type(field, grib_type[grib2_firstFixedSurface]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("typeOfSecondFixedSurface");
    coda_type_record_field_set_type(field, grib_type[grib2_typeOfSecondFixedSurface]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("secondFixedSurface");
    coda_type_record_field_set_type(field, grib_type[grib2_secondFixedSurface]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("coordinateValues");
    coda_type_record_field_set_type(field, grib_type[grib2_coordinateValues_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("bitsPerValue");
    coda_type_record_field_set_type(field, grib_type[grib2_bitsPerValue]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("decimalScaleFactor");
    coda_type_record_field_set_type(field, grib_type[grib2_decimalScaleFactor]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("binaryScaleFactor");
    coda_type_record_field_set_type(field, grib_type[grib2_binaryScaleFactor]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("referenceValue");
    coda_type_record_field_set_type(field, grib_type[grib2_referenceValue]);
    coda_type_record_field_set_hidden(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);
    field = coda_type_record_field_new("values");
    coda_type_record_field_set_type(field, grib_type[grib2_values]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_data], field);

    grib_type[grib2_local_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_local_array], grib_type[grib2_local]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_local_array], NULL);

    grib_type[grib2_grid_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_grid_array], grib_type[grib2_grid]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_grid_array], NULL);

    grib_type[grib2_data_array] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_data_array], grib_type[grib2_data]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_data_array], NULL);

    grib_type[grib2_message] = (coda_type *)coda_type_record_new(coda_format_grib);
    field = coda_type_record_field_new("editionNumber");
    coda_type_record_field_set_type(field, grib_type[grib2_editionNumber]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("discipline");
    coda_type_record_field_set_type(field, grib_type[grib2_discipline]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("centre");
    coda_type_record_field_set_type(field, grib_type[grib2_centre]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("subCentre");
    coda_type_record_field_set_type(field, grib_type[grib2_subCentre]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("masterTablesVersion");
    coda_type_record_field_set_type(field, grib_type[grib2_masterTablesVersion]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("localTablesVersion");
    coda_type_record_field_set_type(field, grib_type[grib2_localTablesVersion]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("significanceOfReferenceTime");
    coda_type_record_field_set_type(field, grib_type[grib2_significanceOfReferenceTime]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("year");
    coda_type_record_field_set_type(field, grib_type[grib2_year]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("month");
    coda_type_record_field_set_type(field, grib_type[grib2_month]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("day");
    coda_type_record_field_set_type(field, grib_type[grib2_day]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("hour");
    coda_type_record_field_set_type(field, grib_type[grib2_hour]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("minute");
    coda_type_record_field_set_type(field, grib_type[grib2_minute]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("second");
    coda_type_record_field_set_type(field, grib_type[grib2_second]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("productionStatusOfProcessedData");
    coda_type_record_field_set_type(field, grib_type[grib2_productionStatusOfProcessedData]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("typeOfProcessedData");
    coda_type_record_field_set_type(field, grib_type[grib2_typeOfProcessedData]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("local");
    coda_type_record_field_set_type(field, grib_type[grib2_local_array]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("grid");
    coda_type_record_field_set_type(field, grib_type[grib2_grid_array]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);
    field = coda_type_record_field_new("data");
    coda_type_record_field_set_type(field, grib_type[grib2_data_array]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_message], field);

    /* GRIB common */

    grib_type[grib_message] = (coda_type *)coda_type_union_new(coda_format_grib);
    field = coda_type_record_field_new("grib1");
    coda_type_record_field_set_type(field, grib_type[grib1_message]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib_message], field);
    field = coda_type_record_field_new("grib2");
    coda_type_record_field_set_type(field, grib_type[grib2_message]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib_message], field);

    grib_type[grib_root] = (coda_type *)coda_type_array_new(coda_format_grib);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib_root], grib_type[grib_message]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib_root], NULL);

    return 0;
}

void coda_grib_done(void)
{
    int i;

    if (grib_type == NULL)
    {
        return;
    }
    for (i = 0; i < num_grib_types; i++)
    {
        if (grib_type[i] != NULL)
        {
            coda_type_release(grib_type[i]);
            grib_type[i] = NULL;
        }
    }
    free(grib_type);
    grib_type = NULL;
}

/* asumes input is big endian */
static float ibmfloat_to_iee754(uint8_t bytes[4])
{
    union
    {
        float as_float;
        uint32_t as_uint32;
    } value;
    int sign;
    int exponent;
    uint32_t mantissa;

    sign = bytes[0] & 0x80 ? 1 : 0;
    exponent = bytes[0] & 0x7F;
    mantissa = ((bytes[1] * 256 + bytes[2]) * 256 + bytes[3]);

    if (mantissa == 0)
    {
        return 0;
    }

    /* change the exponent from base 16, 64 radix, point before first digit ->
       base 2, 127 radix, point after first digit: (exp - 64) * 4 + 127 - 1 */
    exponent = (exponent << 2) - 130;

    /* normalize */
    while (mantissa < 0x800000)
    {
        mantissa <<= 1;
        exponent--;
    }

    if (exponent >= 255)
    {
        /* overflow */
        return (float)coda_PlusInf();
    }
    if (exponent <= 0)
    {
        /* underflow */
        if (exponent < -24)
        {
            mantissa = 0;
        }
        else
        {
            mantissa >>= -exponent;
        }
        exponent = 0;
    }
    value.as_uint32 = (sign << 31) | (exponent << 23) | (mantissa & 0x7FFFFF);
    return value.as_float;
}

static int read_grib1_message(coda_grib_product *product, coda_mem_record *message, int64_t file_offset)
{
    coda_product *cproduct = (coda_product *)product;
    coda_dynamic_type *type;
    coda_type *gtype;
    coda_mem_record *bds;
    uint8_t *bitmask = NULL;
    uint8_t buffer[28];
    long section_size;
    int has_gds, has_bms;
    long num_elements = 0;
    int16_t decimalScaleFactor;
    int16_t binaryScaleFactor;
    float referenceValue;
    uint8_t bitsPerValue;
    uint8_t gridDefinition;
    int32_t intvalue;

    /* Section 1: Product Definition Section (PDS) */
    if (read_bytes(product->raw_product, file_offset, 28, buffer) < 0)
    {
        return -1;
    }

    section_size = (((long)buffer[0] * 256) + buffer[1]) * 256 + buffer[2];

    gtype = grib_type[grib1_table2Version];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[3]);
    coda_mem_record_add_field(message, "table2Version", type, 0);

    gtype = grib_type[grib1_centre];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[4]);
    coda_mem_record_add_field(message, "centre", type, 0);

    gtype = grib_type[grib1_generatingProcessIdentifier];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[5]);
    coda_mem_record_add_field(message, "generatingProcessIdentifier", type, 0);

    gridDefinition = buffer[6];
    gtype = grib_type[grib1_gridDefinition];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, gridDefinition);
    coda_mem_record_add_field(message, "gridDefinition", type, 0);

    has_gds = buffer[7] & 0x80 ? 1 : 0;
    has_bms = buffer[7] & 0x40 ? 1 : 0;

    gtype = grib_type[grib1_indicatorOfParameter];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[8]);
    coda_mem_record_add_field(message, "indicatorOfParameter", type, 0);

    gtype = grib_type[grib1_indicatorOfTypeOfLevel];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[9]);
    coda_mem_record_add_field(message, "indicatorOfTypeOfLevel", type, 0);

    gtype = grib_type[grib1_level];
    type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                    buffer[10] * 256 + buffer[11]);
    coda_mem_record_add_field(message, "level", type, 0);

    gtype = grib_type[grib1_yearOfCentury];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[12]);
    coda_mem_record_add_field(message, "yearOfCentury", type, 0);

    gtype = grib_type[grib1_month];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[13]);
    coda_mem_record_add_field(message, "month", type, 0);

    gtype = grib_type[grib1_day];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[14]);
    coda_mem_record_add_field(message, "day", type, 0);

    gtype = grib_type[grib1_hour];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[15]);
    coda_mem_record_add_field(message, "hour", type, 0);

    gtype = grib_type[grib1_minute];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[16]);
    coda_mem_record_add_field(message, "minute", type, 0);

    gtype = grib_type[grib1_unitOfTimeRange];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[17]);
    coda_mem_record_add_field(message, "unitOfTimeRange", type, 0);

    gtype = grib_type[grib1_P1];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[18]);
    coda_mem_record_add_field(message, "P1", type, 0);

    gtype = grib_type[grib1_P2];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[19]);
    coda_mem_record_add_field(message, "P2", type, 0);

    gtype = grib_type[grib1_timeRangeIndicator];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[20]);
    coda_mem_record_add_field(message, "timeRangeIndicator", type, 0);

    gtype = grib_type[grib1_numberIncludedInAverage];
    type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                    buffer[21] * 256 + buffer[22]);
    coda_mem_record_add_field(message, "numberIncludedInAverage", type, 0);

    gtype = grib_type[grib1_numberMissingFromAveragesOrAccumulations];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[23]);
    coda_mem_record_add_field(message, "numberMissingFromAveragesOrAccumulations", type, 0);

    gtype = grib_type[grib1_centuryOfReferenceTimeOfData];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[24]);
    coda_mem_record_add_field(message, "centuryOfReferenceTimeOfData", type, 0);

    gtype = grib_type[grib1_subCentre];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[25]);
    coda_mem_record_add_field(message, "subCentre", type, 0);

    decimalScaleFactor = (buffer[26] & 0x80 ? -1 : 1) * ((buffer[26] & 0x7F) * 256 + buffer[27]);
    gtype = grib_type[grib1_decimalScaleFactor];
    type = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)gtype, NULL, cproduct, decimalScaleFactor);
    coda_mem_record_add_field(message, "decimalScaleFactor", type, 0);

    file_offset += 28;

    if (section_size > 28)
    {
        if (section_size > 40)
        {
            uint8_t *raw_data;

            raw_data = malloc((section_size - 40) * sizeof(uint8_t));
            if (raw_data == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)((section_size - 40) * sizeof(uint8_t)), __FILE__, __LINE__);
                return -1;
            }
            /* skip bytes 29-40 which are reserved */
            if (read_bytes(product->raw_product, file_offset + 12, section_size - 40, raw_data) < 0)
            {
                free(raw_data);
                return -1;
            }
            type = (coda_dynamic_type *)coda_mem_raw_new((coda_type_raw *)grib_type[grib1_local], NULL, cproduct,
                                                         section_size - 40, raw_data);
            free(raw_data);
            coda_mem_record_add_field(message, "local", type, 0);
        }
        file_offset += section_size - 28;
    }

    if (has_gds)
    {
        coda_mem_record *gds;

        /* Section 2: Grid Description Section (GDS) */
        if (read_bytes(product->raw_product, file_offset, 6, buffer) < 0)
        {
            return -1;
        }

        section_size = (((long)buffer[0] * 256) + buffer[1]) * 256 + buffer[2];

        file_offset += 6;

        /* supported dataRepresentationType values
         * 0: latitude/longitude grid (equidistant cylindrical or Plate Carree projection)
         * 4: Gaussian latitude/longitude grid
         * 10: rotated latitude/longitude grid
         * 14: rotated Gaussian latitude/longitude grid
         * 20: stretched latitude/longitude grid
         * 24: stretched Gaussian latitude/longitude grid
         * 30: stretched and rotated latitude/longitude grid
         * 34: stretched and rotated Gaussian latitude/longitude grid
         */
        if (buffer[5] == 0 || buffer[5] == 4 || buffer[5] == 10 || buffer[5] == 14 || buffer[5] == 20 ||
            buffer[5] == 24 || buffer[5] == 30 || buffer[5] == 34)
        {
            int is_gaussian = (buffer[5] == 4 || buffer[5] == 14 || buffer[5] == 24 || buffer[5] == 34);
            uint8_t NV;
            uint8_t PVL;
            int Ni, Nj;

            /* data representation type is Latitude/Longitude Grid */
            gds = coda_mem_record_new((coda_type_record *)grib_type[grib1_grid], NULL);

            NV = buffer[3];
            gtype = grib_type[grib1_numberOfVerticalCoordinateValues];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, NV);
            coda_mem_record_add_field(gds, "numberOfVerticalCoordinateValues", type, 0);

            PVL = buffer[4];

            gtype = grib_type[grib1_dataRepresentationType];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[5]);
            coda_mem_record_add_field(gds, "dataRepresentationType", type, 0);

            if (read_bytes(product->raw_product, file_offset, 26, buffer) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)gds);
                return -1;
            }

            Ni = buffer[0] * 256 + buffer[1];
            Nj = buffer[2] * 256 + buffer[3];
            if (Ni == 65535 && Nj == 65535)
            {
                coda_set_error(CODA_ERROR_PRODUCT,
                               "grid definition with MISSING value (65535) for both Ni and Nj not supported");
                coda_dynamic_type_delete((coda_dynamic_type *)gds);
                return -1;
            }
            if (Ni != 65535 && Nj != 65535)
            {
                num_elements = ((long)Ni) * Nj;
            }

            gtype = grib_type[grib1_Ni];
            type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct, Ni);
            coda_mem_record_add_field(gds, "Ni", type, 0);

            gtype = grib_type[grib1_Nj];
            type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct, Nj);
            coda_mem_record_add_field(gds, "Nj", type, 0);

            intvalue = (buffer[4] & 0x80 ? -1 : 1) * (((buffer[4] & 0x7F) * 256 + buffer[5]) * 256 + buffer[6]);
            gtype = grib_type[grib1_latitudeOfFirstGridPoint];
            type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
            coda_mem_record_add_field(gds, "latitudeOfFirstGridPoint", type, 0);

            intvalue = (buffer[7] & 0x80 ? -1 : 1) * (((buffer[7] & 0x7F) * 256 + buffer[8]) * 256 + buffer[9]);
            gtype = grib_type[grib1_longitudeOfFirstGridPoint];
            type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
            coda_mem_record_add_field(gds, "longitudeOfFirstGridPoint", type, 0);

            gtype = grib_type[grib1_resolutionAndComponentFlags];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[10]);
            coda_mem_record_add_field(gds, "resolutionAndComponentFlags", type, 0);

            intvalue = (buffer[11] & 0x80 ? -1 : 1) * (((buffer[11] & 0x7F) * 256 + buffer[12]) * 256 + buffer[13]);
            gtype = grib_type[grib1_latitudeOfLastGridPoint];
            type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
            coda_mem_record_add_field(gds, "latitudeOfLastGridPoint", type, 0);

            intvalue = (buffer[14] & 0x80 ? -1 : 1) * (((buffer[14] & 0x7F) * 256 + buffer[15]) * 256 + buffer[16]);
            gtype = grib_type[grib1_longitudeOfLastGridPoint];
            type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
            coda_mem_record_add_field(gds, "longitudeOfLastGridPoint", type, 0);

            gtype = grib_type[grib1_iDirectionIncrement];
            type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                            buffer[17] * 256 + buffer[18]);
            coda_mem_record_add_field(gds, "iDirectionIncrement", type, 0);

            if (is_gaussian)
            {
                gtype = grib_type[grib1_N];
                type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                                buffer[19] * 256 + buffer[20]);
                coda_mem_record_add_field(gds, "N", type, 0);
            }
            else
            {
                gtype = grib_type[grib1_jDirectionIncrement];
                type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                                buffer[19] * 256 + buffer[20]);
                coda_mem_record_add_field(gds, "jDirectionIncrement", type, 0);
            }

            gtype = grib_type[grib1_scanningMode];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[21]);
            coda_mem_record_add_field(gds, "scanningMode", type, 0);

            file_offset += 26;

            if (PVL != 255)
            {
                int i;

                PVL--;  /* make offset zero based */
                file_offset += PVL - 32;
                if (NV > 0)
                {
                    coda_mem_array *coordinateArray;

                    gtype = grib_type[grib1_coordinateValues_array];
                    coordinateArray = coda_mem_array_new((coda_type_array *)gtype, NULL);
                    for (i = 0; i < NV; i++)
                    {
                        if (read_bytes(product->raw_product, file_offset, 4, buffer) < 0)
                        {
                            coda_dynamic_type_delete((coda_dynamic_type *)coordinateArray);
                            coda_dynamic_type_delete((coda_dynamic_type *)gds);
                            return -1;
                        }
                        gtype = grib_type[grib1_coordinateValues];
                        type = (coda_dynamic_type *)coda_mem_float_new((coda_type_number *)gtype, NULL, cproduct,
                                                                       ibmfloat_to_iee754(buffer));
                        coda_mem_array_add_element(coordinateArray, type);

                        file_offset += 4;
                    }
                    coda_mem_record_add_field(gds, "coordinateValues", (coda_dynamic_type *)coordinateArray, 0);
                }
                if (section_size > PVL + NV * 4)
                {
                    coda_mem_array *listOfNumbersArray;
                    long N;

                    if (Ni == 65535)
                    {
                        N = Nj;
                    }
                    else if (Nj == 65535)
                    {
                        N = Ni;
                    }
                    else
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "'list of numbers' in GDS should only be present if Ni or "
                                       "Nj have a MISSING value (65535)");
                        coda_dynamic_type_delete((coda_dynamic_type *)gds);
                        return -1;
                    }

                    /* remaining bytes should provide 'list of numbers of points in each row' */
                    if (section_size - (PVL + NV * 4) != 2 * N)
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "invalid length (%d) for 'list of numbers' in GDS "
                                       "(expected %d)", (int)(section_size - (PVL + NV * 4)), 2 * N);
                        coda_dynamic_type_delete((coda_dynamic_type *)gds);
                        return -1;
                    }

                    gtype = grib_type[grib1_listOfNumbers_array];
                    listOfNumbersArray = coda_mem_array_new((coda_type_array *)gtype, NULL);
                    num_elements = 0;
                    for (i = 0; i < N; i++)
                    {
                        if (read_bytes(product->raw_product, file_offset, 2, buffer) < 0)
                        {
                            coda_dynamic_type_delete((coda_dynamic_type *)listOfNumbersArray);
                            coda_dynamic_type_delete((coda_dynamic_type *)gds);
                            return -1;
                        }
                        num_elements += buffer[0] * 256 + buffer[1];
                        gtype = grib_type[grib1_listOfNumbers];
                        type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                                        buffer[0] * 256 + buffer[1]);
                        coda_mem_array_add_element(listOfNumbersArray, type);
                        file_offset += 2;
                    }
                    coda_mem_record_add_field(gds, "listOfNumbers", (coda_dynamic_type *)listOfNumbersArray, 0);
                }
                else if (Ni == 65535 || Nj == 65535)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "grid definition with MISSING value (65535) for Ni or Nj "
                                   "without 'List of numbers' not supported");
                    coda_dynamic_type_delete((coda_dynamic_type *)gds);
                    return -1;
                }
            }
            else
            {
                if (Ni == 65535 || Nj == 65535)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "grid definition with MISSING value (65535) for Ni or Nj "
                                   "without 'List of numbers' not supported");
                    coda_dynamic_type_delete((coda_dynamic_type *)gds);
                    return -1;
                }
                if (section_size > 32)
                {
                    file_offset += section_size - 32;
                }
            }

            coda_mem_record_add_field(message, "grid", (coda_dynamic_type *)gds, 0);
        }
        else
        {
            coda_set_error(CODA_ERROR_PRODUCT, "unsupported data representation type (%d) in GDS", buffer[5]);
            return -1;
        }
    }
    else
    {
        /* try to determine grid properties (i.e. number of elements in BDS) from gridDefinition */
        switch (gridDefinition)
        {
            case 1:
                num_elements = 1679;
                break;
            case 2:
                num_elements = 10512;
                break;
            case 3:
                num_elements = 65160;
                break;
            case 4:
                num_elements = 259920;
                break;
            case 5:
            case 6:
                num_elements = 2385;
                break;
            case 8:
                num_elements = 5104;
                break;
            case 21:
            case 22:
            case 23:
            case 24:
                num_elements = 1333;
                break;
            case 25:
            case 26:
                num_elements = 1297;
                break;
            case 27:
            case 28:
                num_elements = 4225;
                break;
            case 29:
            case 30:
                num_elements = 5365;
                break;
            case 33:
            case 34:
                num_elements = 8326;
                break;
            case 50:
                num_elements = 964;
                break;
            case 53:
                num_elements = 5967;
                break;
            case 55:
            case 56:
                num_elements = 6177;
                break;
            case 61:
            case 62:
            case 63:
            case 64:
                num_elements = 4096;
                break;
            case 75:
            case 76:
            case 77:
                num_elements = 12321;
                break;
            case 85:
            case 86:
                num_elements = 32400;
                break;
            case 87:
                num_elements = 5022;
                break;
            case 90:
                num_elements = 12902;
                break;
            case 91:
                num_elements = 25803;
                break;
            case 92:
                num_elements = 81213;
                break;
            case 93:
                num_elements = 162425;
                break;
            case 94:
                num_elements = 48916;
                break;
            case 95:
                num_elements = 97831;
                break;
            case 96:
                num_elements = 41630;
                break;
            case 97:
                num_elements = 83259;
                break;
            case 100:
                num_elements = 6889;
                break;
            case 101:
                num_elements = 10283;
                break;
            case 103:
                num_elements = 3640;
                break;
            case 104:
                num_elements = 16170;
                break;
            case 105:
                num_elements = 6889;
                break;
            case 106:
                num_elements = 19305;
                break;
            case 107:
                num_elements = 11040;
                break;
            default:
                coda_set_error(CODA_ERROR_PRODUCT, "gridDefinition (%d) not supported", gridDefinition);
                return -1;
        }
    }

    if (has_bms)
    {
        /* Section 3: Bit Map Section (BMS) */
        if (read_bytes(product->raw_product, file_offset, 6, buffer) < 0)
        {
            return -1;
        }

        section_size = (((long)buffer[0] * 256) + buffer[1]) * 256 + buffer[2];
        if ((buffer[4] * 256) + buffer[5] != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "Bit Map Section with predefined bit map not supported");
            return -1;
        }
        if (section_size - 6 < bit_size_to_byte_size(num_elements))
        {
            coda_set_error(CODA_ERROR_PRODUCT, "Size of bitmap in Bit Map Section (%ld bytes) does not match expected "
                           "size (%ld bytes) based on %ld grid elements", (long)section_size - 6,
                           bit_size_to_byte_size(num_elements), num_elements);
            return -1;
        }

        bitmask = malloc((section_size - 6) * sizeof(uint8_t));
        if (bitmask == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)((section_size - 6) * sizeof(uint8_t)), __FILE__, __LINE__);
            return -1;
        }
        if (read_bytes(product->raw_product, file_offset + 6, section_size - 6, bitmask) < 0)
        {
            free(bitmask);
            return -1;
        }
        file_offset += section_size;
    }

    /* Section 4: Binary Data Section (BDS) */
    if (read_bytes(product->raw_product, file_offset, 11, buffer) < 0)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        return -1;
    }

    section_size = (((long)buffer[0] * 256) + buffer[1]) * 256 + buffer[2];
    if (buffer[3] & 0x80)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        coda_set_error(CODA_ERROR_PRODUCT, "spherical harmonic coefficients data not supported");
        return -1;
    }
    if (buffer[3] & 0x40)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        coda_set_error(CODA_ERROR_PRODUCT, "second order ('Complex') Packing not supported");
        return -1;
    }
    /* int isIntegerData = (buffer[3] & 0x20 ? 1 : 0); */
    if (buffer[3] & 0x10)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        coda_set_error(CODA_ERROR_PRODUCT, "presence of additional flags in BDS not supported");
        return -1;
    }
    binaryScaleFactor = (buffer[4] & 0x80 ? -1 : 1) * ((buffer[4] & 0x7F) * 256 + buffer[5]);
    referenceValue = ibmfloat_to_iee754(&buffer[6]);
    bitsPerValue = buffer[10];
    if (bitsPerValue > 63)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        coda_set_error(CODA_ERROR_PRODUCT, "bitsPerValue (%d) too large in BDS", bitsPerValue);
        return -1;
    }
    bds = coda_mem_record_new((coda_type_record *)grib_type[grib1_data], NULL);
    gtype = grib_type[grib1_bitsPerValue];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, bitsPerValue);
    coda_mem_record_add_field(bds, "bitsPerValue", type, 0);
    gtype = grib_type[grib1_binaryScaleFactor];
    type = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)gtype, NULL, cproduct, binaryScaleFactor);
    coda_mem_record_add_field(bds, "binaryScaleFactor", type, 0);
    gtype = grib_type[grib1_referenceValue];
    type = (coda_dynamic_type *)coda_mem_float_new((coda_type_number *)gtype, NULL, cproduct, referenceValue);
    coda_mem_record_add_field(bds, "referenceValue", type, 0);

    file_offset += 11;

    type = (coda_dynamic_type *)coda_grib_value_array_simple_packing_new((coda_type_array *)grib_type[grib1_values],
                                                                         num_elements, file_offset, bitsPerValue,
                                                                         decimalScaleFactor, binaryScaleFactor,
                                                                         referenceValue, bitmask);
    if (bitmask != NULL)
    {
        free(bitmask);
    }
    coda_mem_record_add_field(bds, "values", type, 0);

    coda_mem_record_add_field(message, "data", (coda_dynamic_type *)bds, 0);

    file_offset += section_size - 11;

    /* Section 5: '7777' (ASCII Characters) */
    if (read_bytes(product->raw_product, file_offset, 4, buffer) < 0)
    {
        return -1;
    }
    if (memcmp(buffer, "7777", 4) != 0)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_FILE_READ, "invalid GRIB termination section at byte position %s", file_offset_str);
        return -1;
    }
    file_offset += 4;

    return 0;
}

static int read_grib2_message(coda_grib_product *product, coda_mem_record *message, int64_t file_offset)
{
    coda_product *cproduct = (coda_product *)product;
    coda_mem_array *localArray;
    coda_mem_array *gridArray;
    coda_mem_array *dataArray;
    coda_dynamic_type *type;
    coda_type *gtype;
    int has_bitmask = 0;
    int64_t bitmask_offset = -1;
    int64_t bitmask_length = 0;
    long localRecordIndex = -1;
    long gridSectionIndex = -1;
    uint16_t num_coordinate_values = 0;
    int64_t coordinate_values_offset = -1;
    uint8_t parameterCategory = 0;
    uint8_t parameterNumber = 0;
    int has_constituentType = 0;
    uint16_t constituentType = 0;
    uint8_t typeOfGeneratingProcess = 0;
    uint8_t backgroundProcess = 0;
    uint8_t generatingProcessIdentifier = 0;
    uint16_t hoursAfterDataCutoff = 0;
    uint8_t minutesAfterDataCutoff = 0;
    uint8_t indicatorOfUnitOfTimeRange = 0;
    uint32_t forecastTime = 0;
    uint8_t typeOfFirstFixedSurface = 0;
    double firstFixedSurface = 0;
    uint8_t typeOfSecondFixedSurface = 0;
    double secondFixedSurface = 0;
    int16_t decimalScaleFactor = 0;
    int16_t binaryScaleFactor = 0;
    float referenceValue = 0;
    uint8_t bitsPerValue = 0;
    uint32_t num_elements = 0;
    uint32_t section_size;
    uint8_t buffer[64];
    uint8_t prev_section;

    /* Section 1: Identification Section */
    if (read_bytes(product->raw_product, file_offset, 21, buffer) < 0)
    {
        return -1;
    }

    section_size = (((uint32_t)buffer[0] * 256 + buffer[1]) * 256 + buffer[2]) * 256 + buffer[3];

    if (buffer[4] != 1)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_PRODUCT, "wrong Section Number (%d) for Identification Section at offset %s",
                       buffer[4] - '0', file_offset_str);
        return -1;
    }
    prev_section = 1;

    gtype = grib_type[grib2_centre];
    type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                    buffer[5] * 256 + buffer[6]);
    coda_mem_record_add_field(message, "centre", type, 0);

    gtype = grib_type[grib2_subCentre];
    type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                    buffer[7] * 256 + buffer[8]);
    coda_mem_record_add_field(message, "subCentre", type, 0);

    gtype = grib_type[grib2_masterTablesVersion];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[9]);
    coda_mem_record_add_field(message, "masterTablesVersion", type, 0);

    gtype = grib_type[grib2_localTablesVersion];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[10]);
    coda_mem_record_add_field(message, "localTablesVersion", type, 0);

    gtype = grib_type[grib2_significanceOfReferenceTime];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[11]);
    coda_mem_record_add_field(message, "significanceOfReferenceTime", type, 0);

    gtype = grib_type[grib2_year];
    type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                    buffer[12] * 256 + buffer[13]);
    coda_mem_record_add_field(message, "year", type, 0);

    gtype = grib_type[grib2_month];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[14]);
    coda_mem_record_add_field(message, "month", type, 0);

    gtype = grib_type[grib2_day];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[15]);
    coda_mem_record_add_field(message, "day", type, 0);

    gtype = grib_type[grib2_hour];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[16]);
    coda_mem_record_add_field(message, "hour", type, 0);

    gtype = grib_type[grib2_minute];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[17]);
    coda_mem_record_add_field(message, "minute", type, 0);

    gtype = grib_type[grib2_second];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[18]);
    coda_mem_record_add_field(message, "second", type, 0);

    gtype = grib_type[grib2_productionStatusOfProcessedData];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[19]);
    coda_mem_record_add_field(message, "productionStatusOfProcessedData", type, 0);

    gtype = grib_type[grib2_typeOfProcessedData];
    type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[20]);
    coda_mem_record_add_field(message, "typeOfProcessedData", type, 0);

    localArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_local_array], NULL);
    coda_mem_record_add_field(message, "local", (coda_dynamic_type *)localArray, 0);

    gridArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_grid_array], NULL);
    coda_mem_record_add_field(message, "grid", (coda_dynamic_type *)gridArray, 0);

    dataArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_data_array], NULL);
    coda_mem_record_add_field(message, "data", (coda_dynamic_type *)dataArray, 0);

    file_offset += 21;

    if (section_size > 21)
    {
        file_offset += section_size - 21;
    }

    /* keep looping over message sections until we find section 8 or until we encounter an error */
    if (read_bytes(product->raw_product, file_offset, 4, buffer) < 0)
    {
        return -1;
    }
    file_offset += 4;
    while (memcmp(buffer, "7777", 4) != 0)
    {
        section_size = (((uint32_t)buffer[0] * 256 + buffer[1]) * 256 + buffer[2]) * 256 + buffer[3];

        /* read section number */
        if (read_bytes(product->raw_product, file_offset, 1, buffer) < 0)
        {
            return -1;
        }
        file_offset += 1;

        if (*buffer == 2)
        {
            uint8_t *raw_data;

            /* Section 2: Local Use Section */
            if (prev_section != 1 && prev_section != 7)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (section_size > 5)
            {
                raw_data = malloc((section_size - 5) * sizeof(uint8_t));
                if (raw_data == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)((section_size - 5) * sizeof(uint8_t)), __FILE__, __LINE__);
                    return -1;
                }
                if (read_bytes(product->raw_product, file_offset, section_size - 5, raw_data) < 0)
                {
                    free(raw_data);
                    return -1;
                }
                gtype = grib_type[grib2_local];
                type = (coda_dynamic_type *)coda_mem_raw_new((coda_type_raw *)gtype, NULL, cproduct, section_size - 5,
                                                             raw_data);
                coda_mem_array_add_element(localArray, type);
                file_offset += section_size - 5;
                localRecordIndex++;
            }
            prev_section = 2;
        }
        else if (*buffer == 3)
        {
            coda_mem_record *grid;
            uint32_t num_data_points;
            uint16_t template_number;
            uint8_t number_size;
            uint8_t number_interpretation;

            /* Section 3: Grid Definition Section */
            if (prev_section != 1 && prev_section != 2 && prev_section != 7)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (read_bytes(product->raw_product, file_offset, 9, buffer) < 0)
            {
                return -1;
            }

            grid = coda_mem_record_new((coda_type_record *)grib_type[grib2_grid], NULL);

            gtype = grib_type[grib2_localRecordIndex];
            type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, localRecordIndex);
            coda_mem_record_add_field(grid, "localRecordIndex", type, 0);

            gtype = grib_type[grib2_sourceOfGridDefinition];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[0]);
            coda_mem_record_add_field(grid, "sourceOfGridDefinition", type, 0);

            num_data_points = (((uint32_t)buffer[1] * 256 + buffer[2]) * 256 + buffer[3]) * 256 + buffer[4];
            gtype = grib_type[grib2_numberOfDataPoints];
            type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, num_data_points);
            coda_mem_record_add_field(grid, "numberOfDataPoints", type, 0);

            number_size = buffer[5];
            number_interpretation = buffer[6];
            gtype = grib_type[grib2_interpretationOfListOfNumbers];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           number_interpretation);
            coda_mem_record_add_field(grid, "interpretationOfListOfNumbers", type, 0);

            template_number = buffer[7] * 256 + buffer[8];
            gtype = grib_type[grib2_gridDefinitionTemplateNumber];
            type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct, template_number);
            coda_mem_record_add_field(grid, "gridDefinitionTemplateNumber", type, 0);

            file_offset += 9;

            /* supported gridDefinitionTemplateNumber values
             * 0: latitude/longitude grid (equidistant cylindrical or Plate Carree projection)
             * 1: rotated latitude/longitude grid
             * 2: stretched latitude/longitude grid
             * 3: stretched and rotated latitude/longitude grid
             * 40: Gaussian latitude/longitude grid
             * 41: rotated Gaussian latitude/longitude grid
             * 42: stretched Gaussian latitude/longitude grid
             * 43: stretched and rotated Gaussian latitude/longitude grid
             */
            if (buffer[0] == 0 && (template_number <= 3 || (template_number >= 40 && template_number <= 43)))
            {
                uint32_t intvalue;
                uint32_t Ni, Nj;

                if (read_bytes(product->raw_product, file_offset, 58, buffer) < 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)grid);
                    return -1;
                }

                gtype = grib_type[grib2_shapeOfTheEarth];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[0]);
                coda_mem_record_add_field(grid, "shapeOfTheEarth", type, 0);

                gtype = grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[1]);
                coda_mem_record_add_field(grid, "scaleFactorOfRadiusOfSphericalEarth", type, 0);

                intvalue = (((uint32_t)buffer[2] * 256 + buffer[3]) * 256 + buffer[4]) * 256 + buffer[5];
                gtype = grib_type[grib2_scaledValueOfRadiusOfSphericalEarth];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfRadiusOfSphericalEarth", type, 0);

                gtype = grib_type[grib2_scaleFactorOfEarthMajorAxis];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[6]);
                coda_mem_record_add_field(grid, "scaleFactorOfEarthMajorAxis", type, 0);

                intvalue = (((uint32_t)buffer[7] * 256 + buffer[8]) * 256 + buffer[9]) * 256 + buffer[10];
                gtype = grib_type[grib2_scaledValueOfEarthMajorAxis];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfEarthMajorAxis", type, 0);

                gtype = grib_type[grib2_scaleFactorOfEarthMinorAxis];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[11]);
                coda_mem_record_add_field(grid, "scaleFactorOfEarthMinorAxis", type, 0);

                intvalue = (((uint32_t)buffer[12] * 256 + buffer[13]) * 256 + buffer[14]) * 256 + buffer[15];
                gtype = grib_type[grib2_scaledValueOfEarthMinorAxis];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfEarthMinorAxis", type, 0);

                Ni = (((uint32_t)buffer[16] * 256 + buffer[17]) * 256 + buffer[18]) * 256 + buffer[19];
                gtype = grib_type[grib2_Ni];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, Ni);
                coda_mem_record_add_field(grid, "Ni", type, 0);

                Nj = (((uint32_t)buffer[20] * 256 + buffer[21]) * 256 + buffer[22]) * 256 + buffer[23];
                gtype = grib_type[grib2_Nj];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, Nj);
                coda_mem_record_add_field(grid, "Nj", type, 0);

                intvalue = (((uint32_t)buffer[24] * 256 + buffer[25]) * 256 + buffer[26]) * 256 + buffer[27];
                gtype = grib_type[grib2_basicAngleOfTheInitialProductionDomain];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "basicAngleOfTheInitialProductionDomain", type, 0);

                intvalue = (((uint32_t)buffer[28] * 256 + buffer[29]) * 256 + buffer[30]) * 256 + buffer[31];
                gtype = grib_type[grib2_subdivisionsOfBasicAngle];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "subdivisionsOfBasicAngle", type, 0);

                intvalue = (((uint32_t)buffer[32] * 256 + buffer[33]) * 256 + buffer[34]) * 256 + buffer[35];
                intvalue = buffer[32] & 0x80 ? -((int64_t)intvalue - (1 << 31)) : intvalue;
                gtype = grib_type[grib2_latitudeOfFirstGridPoint];
                type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "latitudeOfFirstGridPoint", type, 0);

                intvalue = (((uint32_t)buffer[36] * 256 + buffer[37]) * 256 + buffer[38]) * 256 + buffer[39];
                intvalue = buffer[36] & 0x80 ? -((int64_t)intvalue - (1 << 31)) : intvalue;
                gtype = grib_type[grib2_longitudeOfFirstGridPoint];
                type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "longitudeOfFirstGridPoint", type, 0);

                gtype = grib_type[grib2_resolutionAndComponentFlags];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[40]);
                coda_mem_record_add_field(grid, "resolutionAndComponentFlags", type, 0);

                intvalue = (((uint32_t)buffer[41] * 256 + buffer[42]) * 256 + buffer[43]) * 256 + buffer[44];
                intvalue = buffer[41] & 0x80 ? -((int64_t)intvalue - (1 << 31)) : intvalue;
                gtype = grib_type[grib2_latitudeOfLastGridPoint];
                type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "latitudeOfLastGridPoint", type, 0);

                intvalue = (((uint32_t)buffer[45] * 256 + buffer[46]) * 256 + buffer[47]) * 256 + buffer[48];
                intvalue = buffer[45] & 0x80 ? -((int64_t)intvalue - (1 << 31)) : intvalue;
                gtype = grib_type[grib2_longitudeOfLastGridPoint];
                type = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "longitudeOfLastGridPoint", type, 0);

                intvalue = (((uint32_t)buffer[49] * 256 + buffer[50]) * 256 + buffer[51]) * 256 + buffer[52];
                gtype = grib_type[grib2_iDirectionIncrement];
                type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, intvalue);
                coda_mem_record_add_field(grid, "iDirectionIncrement", type, 0);

                intvalue = (((uint32_t)buffer[53] * 256 + buffer[54]) * 256 + buffer[55]) * 256 + buffer[56];
                if (template_number >= 40 && template_number <= 43)
                {
                    gtype = grib_type[grib2_N];
                    type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct,
                                                                    intvalue);
                    coda_mem_record_add_field(grid, "N", type, 0);
                }
                else
                {
                    gtype = grib_type[grib2_jDirectionIncrement];
                    type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct,
                                                                    intvalue);
                    coda_mem_record_add_field(grid, "jDirectionIncrement", type, 0);
                }

                gtype = grib_type[grib2_scanningMode];
                type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, buffer[57]);
                coda_mem_record_add_field(grid, "scanningMode", type, 0);

                file_offset += 58;

                if (number_interpretation > 0)
                {
                    coda_mem_array *listOfNumbersArray;
                    long N;
                    long i;

                    if (Ni == 4294967295)
                    {
                        N = Nj;
                    }
                    else if (Nj == 4294967295)
                    {
                        N = Ni;
                    }
                    else
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "'list of numbers' in GDS should only be present if Ni or "
                                       "Nj have a MISSING value (4294967295)");
                        coda_dynamic_type_delete((coda_dynamic_type *)grid);
                        return -1;
                    }

                    /* remaining bytes should provide 'list of numbers of points in each row' */
                    if (section_size - 72 != number_size * N)
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "invalid length (%ld) for 'list of numbers' in GDS "
                                       "(expected %ld)", (long)(section_size - 72), number_size * N);
                        coda_dynamic_type_delete((coda_dynamic_type *)grid);
                        return -1;
                    }

                    gtype = grib_type[grib2_listOfNumbers_array];
                    listOfNumbersArray = coda_mem_array_new((coda_type_array *)gtype, NULL);
                    for (i = 0; i < N; i++)
                    {
                        uint32_t value;

                        if (read_bytes(product->raw_product, file_offset, 2, buffer) < 0)
                        {
                            coda_dynamic_type_delete((coda_dynamic_type *)listOfNumbersArray);
                            coda_dynamic_type_delete((coda_dynamic_type *)grid);
                            return -1;
                        }
                        switch (number_size)
                        {
                            case 1:
                                value = buffer[0];
                                break;
                            case 2:
                                value = buffer[0] * 256 + buffer[1];
                                break;
                            case 4:
                                value = (((uint32_t)buffer[0] * 256 + buffer[1]) * 256 + buffer[2]) * 256 + buffer[3];
                                break;
                            default:
                                coda_set_error(CODA_ERROR_PRODUCT, "unsupported octect size (%d) for numbers in 'list "
                                               "of numbers' in GDS", (int)number_size);
                                coda_dynamic_type_delete((coda_dynamic_type *)listOfNumbersArray);
                                coda_dynamic_type_delete((coda_dynamic_type *)grid);
                                return -1;
                        }
                        gtype = grib_type[grib2_listOfNumbers];
                        type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct,
                                                                        value);
                        coda_mem_array_add_element(listOfNumbersArray, type);
                        file_offset += number_size;
                    }
                    coda_mem_record_add_field(grid, "listOfNumbers", (coda_dynamic_type *)listOfNumbersArray, 0);
                }
                else
                {
                    if (Ni == 4294967295 || Nj == 4294967295)
                    {
                        coda_set_error(CODA_ERROR_PRODUCT,
                                       "grid definition with MISSING value (4294967295) for Ni or Nj "
                                       "without 'List of numbers' not supported");
                        coda_dynamic_type_delete((coda_dynamic_type *)grid);
                        return -1;
                    }
                    if (section_size > 72)
                    {
                        file_offset += section_size - 72;
                    }
                }
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unsupported grid source/template (%d/%d)", buffer[0],
                               template_number);
                coda_dynamic_type_delete((coda_dynamic_type *)grid);
                return -1;
            }

            coda_mem_array_add_element(gridArray, (coda_dynamic_type *)grid);

            gridSectionIndex++;
            prev_section = 3;
        }
        else if (*buffer == 4)
        {
            uint16_t productDefinitionTemplate;

            /* Section 4: Product Definition Section */
            if (prev_section != 3 && prev_section != 7)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (read_bytes(product->raw_product, file_offset, 4, buffer) < 0)
            {
                return -1;
            }
            num_coordinate_values = (buffer[0] * 256 + buffer[1]);
            productDefinitionTemplate = (buffer[2] * 256 + buffer[3]);
            file_offset += 4;

            /* we could possibly add support for 41, 44, 45 and 48 in the future as well */
            if (productDefinitionTemplate <= 6 || productDefinitionTemplate == 15  || productDefinitionTemplate == 51)
            {
                if (read_bytes(product->raw_product, file_offset, 25, buffer) < 0)
                {
                    return -1;
                }
                parameterCategory = buffer[0];
                parameterNumber = buffer[1];
                typeOfGeneratingProcess = buffer[2];
                backgroundProcess = buffer[3];
                generatingProcessIdentifier = buffer[4];
                hoursAfterDataCutoff = buffer[5] * 256 + buffer[6];
                minutesAfterDataCutoff = buffer[7];
                indicatorOfUnitOfTimeRange = buffer[8];
                forecastTime = (((uint32_t)buffer[9] * 256 + buffer[10]) * 256 + buffer[11]) * 256 + buffer[12];
                typeOfFirstFixedSurface = buffer[13];
                if (typeOfFirstFixedSurface != 255)
                {
                    int8_t scaleFactor = ((int8_t *)buffer)[14];

                    firstFixedSurface = (((uint32_t)buffer[15] * 256 + buffer[16]) * 256 + buffer[17]) * 256 +
                        buffer[18];
                    while (scaleFactor < 0)
                    {
                        firstFixedSurface *= 10;
                        scaleFactor++;
                    }
                    while (scaleFactor > 0)
                    {
                        firstFixedSurface /= 10;
                        scaleFactor--;
                    }
                }
                else
                {
                    firstFixedSurface = coda_NaN();
                }
                typeOfSecondFixedSurface = buffer[19];
                if (typeOfSecondFixedSurface != 255)
                {
                    int8_t scaleFactor = ((int8_t *)buffer)[20];

                    secondFixedSurface = (((uint32_t)buffer[21] * 256 + buffer[22]) * 256 + buffer[23]) * 256 +
                        buffer[24];
                    while (scaleFactor < 0)
                    {
                        secondFixedSurface *= 10;
                        scaleFactor++;
                    }
                    while (scaleFactor > 0)
                    {
                        secondFixedSurface /= 10;
                        scaleFactor--;
                    }
                }
                else
                {
                    secondFixedSurface = coda_NaN();
                }
                file_offset += 25;
                coordinate_values_offset = num_coordinate_values > 0 ? file_offset : -1;
            }
            else if (productDefinitionTemplate == 40)
            {
                if (read_bytes(product->raw_product, file_offset, 25, buffer) < 0)
                {
                    return -1;
                }
                parameterCategory = buffer[0];
                parameterNumber = buffer[1];
                has_constituentType = 1;
                constituentType = buffer[2] * 256 + buffer[3];
                typeOfGeneratingProcess = buffer[4];
                backgroundProcess = buffer[5];
                generatingProcessIdentifier = buffer[6];
                hoursAfterDataCutoff = buffer[7] * 256 + buffer[8];
                minutesAfterDataCutoff = buffer[9];
                indicatorOfUnitOfTimeRange = buffer[10];
                forecastTime = (((uint32_t)buffer[11] * 256 + buffer[12]) * 256 + buffer[13]) * 256 + buffer[14];
                typeOfFirstFixedSurface = buffer[15];
                if (typeOfFirstFixedSurface != 255)
                {
                    int8_t scaleFactor = ((int8_t *)buffer)[16];

                    firstFixedSurface = (((uint32_t)buffer[17] * 256 + buffer[18]) * 256 + buffer[19]) * 256 +
                        buffer[20];
                    while (scaleFactor < 0)
                    {
                        firstFixedSurface *= 10;
                        scaleFactor++;
                    }
                    while (scaleFactor > 0)
                    {
                        firstFixedSurface /= 10;
                        scaleFactor--;
                    }
                }
                else
                {
                    firstFixedSurface = coda_NaN();
                }
                typeOfSecondFixedSurface = buffer[21];
                if (typeOfSecondFixedSurface != 255)
                {
                    int8_t scaleFactor = ((int8_t *)buffer)[22];

                    secondFixedSurface = (((uint32_t)buffer[23] * 256 + buffer[24]) * 256 + buffer[25]) * 256 +
                        buffer[26];
                    while (scaleFactor < 0)
                    {
                        secondFixedSurface *= 10;
                        scaleFactor++;
                    }
                    while (scaleFactor > 0)
                    {
                        secondFixedSurface /= 10;
                        scaleFactor--;
                    }
                }
                else
                {
                    secondFixedSurface = coda_NaN();
                }
                file_offset += 25;
                coordinate_values_offset = num_coordinate_values > 0 ? file_offset : -1;
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unsupported Product Definition Template (%d)",
                               productDefinitionTemplate);
                return -1;
            }

            if (section_size > 34)
            {
                file_offset += section_size - 34;
            }

            prev_section = 4;
        }
        else if (*buffer == 5)
        {
            uint16_t dataRepresentationTemplate;

            /* Section 5: Data Representation Section */
            if (prev_section != 4)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (read_bytes(product->raw_product, file_offset, 6, buffer) < 0)
            {
                return -1;
            }
            num_elements = (((uint32_t)buffer[0] * 256 + buffer[1]) * 256 + buffer[2]) * 256 + buffer[3];
            dataRepresentationTemplate = buffer[4] * 256 + buffer[5];
            file_offset += 6;

            if (dataRepresentationTemplate == 0 || dataRepresentationTemplate == 1)
            {
                if (read_bytes(product->raw_product, file_offset, 4, &referenceValue) < 0)
                {
                    return -1;
                }
#ifndef WORDS_BIGENDIAN
                swap_float(&referenceValue);
#endif
                if (read_bytes(product->raw_product, file_offset + 4, 5, buffer) < 0)
                {
                    return -1;
                }

                binaryScaleFactor = (buffer[0] & 0x80 ? -1 : 1) * ((buffer[0] & 0x7F) * 256 + buffer[1]);
                decimalScaleFactor = (buffer[2] & 0x80 ? -1 : 1) * ((buffer[2] & 0x7F) * 256 + buffer[3]);
                bitsPerValue = buffer[4];
                if (bitsPerValue > 63)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "bitsPerValue (%d) too large", bitsPerValue);
                    return -1;
                }
                file_offset += 9;
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unsupported Data Representation Template (%d)",
                               dataRepresentationTemplate);
                return -1;
            }

            if (section_size > 20)
            {
                file_offset += section_size - 20;
            }

            prev_section = 5;
        }
        else if (*buffer == 6)
        {
            /* Section 6: Bit-Map Section */
            if (prev_section != 5)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (read_bytes(product->raw_product, file_offset, 1, buffer) < 0)
            {
                return -1;
            }
            if (*buffer == 0)
            {
                has_bitmask = 1;
                bitmask_offset = file_offset + 1;
                bitmask_length = section_size - 6;
            }
            else if (*buffer == 254)
            {
                has_bitmask = 1;
                if (bitmask_offset == -1)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "no previously defined Bit Map found");
                    return -1;
                }
            }
            else if (*buffer == 255)
            {
                has_bitmask = 0;
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "pre-defined Bit Maps not supported");
                return -1;
            }
            file_offset++;

            if (section_size > 6)
            {
                file_offset += section_size - 6;
            }

            prev_section = 6;
        }
        else if (*buffer == 7)
        {
            coda_mem_record *data;
            uint8_t *bitmask = NULL;

            /* Section 7: Data Section */
            if (prev_section != 5 && prev_section != 6)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            data = coda_mem_record_new((coda_type_record *)grib_type[grib2_data], NULL);

            gtype = grib_type[grib2_gridRecordIndex];
            type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct,
                                                            gridSectionIndex);
            coda_mem_record_add_field(data, "gridRecordIndex", type, 0);

            gtype = grib_type[grib2_parameterCategory];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           parameterCategory);
            coda_mem_record_add_field(data, "parameterCategory", type, 0);

            gtype = grib_type[grib2_parameterNumber];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, parameterNumber);
            coda_mem_record_add_field(data, "parameterNumber", type, 0);

            if (has_constituentType)
            {
                gtype = grib_type[grib2_constituentType];
                type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                                constituentType);
                coda_mem_record_add_field(data, "constituentType", type, 0);
            }

            gtype = grib_type[grib2_typeOfGeneratingProcess];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           typeOfGeneratingProcess);
            coda_mem_record_add_field(data, "typeOfGeneratingProcess", type, 0);

            gtype = grib_type[grib2_backgroundProcess];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           backgroundProcess);
            coda_mem_record_add_field(data, "backgroundProcess", type, 0);

            gtype = grib_type[grib2_generatingProcessIdentifier];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           generatingProcessIdentifier);
            coda_mem_record_add_field(data, "generatingProcessIdentifier", type, 0);

            gtype = grib_type[grib2_hoursAfterDataCutoff];
            type = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)gtype, NULL, cproduct,
                                                            hoursAfterDataCutoff);
            coda_mem_record_add_field(data, "hoursAfterDataCutoff", type, 0);

            gtype = grib_type[grib2_minutesAfterDataCutoff];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           minutesAfterDataCutoff);
            coda_mem_record_add_field(data, "minutesAfterDataCutoff", type, 0);

            gtype = grib_type[grib2_indicatorOfUnitOfTimeRange];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           indicatorOfUnitOfTimeRange);
            coda_mem_record_add_field(data, "indicatorOfUnitOfTimeRange", type, 0);

            gtype = grib_type[grib2_forecastTime];
            type = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)gtype, NULL, cproduct, forecastTime);
            coda_mem_record_add_field(data, "forecastTime", type, 0);

            gtype = grib_type[grib2_typeOfFirstFixedSurface];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           typeOfFirstFixedSurface);
            coda_mem_record_add_field(data, "typeOfFirstFixedSurface", type, 0);

            gtype = grib_type[grib2_firstFixedSurface];
            type = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)gtype, NULL, cproduct,
                                                            firstFixedSurface);
            coda_mem_record_add_field(data, "firstFixedSurface", type, 0);

            gtype = grib_type[grib2_typeOfSecondFixedSurface];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct,
                                                           typeOfSecondFixedSurface);
            coda_mem_record_add_field(data, "typeOfSecondFixedSurface", type, 0);

            gtype = grib_type[grib2_secondFixedSurface];
            type = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)gtype, NULL, cproduct,
                                                            secondFixedSurface);
            coda_mem_record_add_field(data, "secondFixedSurface", type, 0);

            if (num_coordinate_values > 0)
            {
                gtype = grib_type[grib2_coordinateValues_array];
                type = (coda_dynamic_type *)coda_grib_value_array_new((coda_type_array *)gtype, num_coordinate_values,
                                                                      coordinate_values_offset);
                coda_mem_record_add_field(data, "coordinateValues", type, 0);
            }

            gtype = grib_type[grib2_bitsPerValue];
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)gtype, NULL, cproduct, bitsPerValue);
            coda_mem_record_add_field(data, "bitsPerValue", type, 0);

            gtype = grib_type[grib2_decimalScaleFactor];
            type = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)gtype, NULL, cproduct,
                                                           decimalScaleFactor);
            coda_mem_record_add_field(data, "decimalScaleFactor", type, 0);

            gtype = grib_type[grib2_binaryScaleFactor];
            type = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)gtype, NULL, cproduct,
                                                           binaryScaleFactor);
            coda_mem_record_add_field(data, "binaryScaleFactor", type, 0);

            gtype = grib_type[grib2_referenceValue];
            type = (coda_dynamic_type *)coda_mem_float_new((coda_type_number *)gtype, NULL, cproduct, referenceValue);
            coda_mem_record_add_field(data, "referenceValue", type, 0);

            if (has_bitmask)
            {
                /* read bitmask array */
                bitmask = malloc((size_t)bitmask_length * sizeof(uint8_t));
                if (bitmask == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)(bitmask_length * sizeof(uint8_t)), __FILE__, __LINE__);
                    return -1;
                }
                if (read_bytes(product->raw_product, bitmask_offset, bitmask_length, bitmask) < 0)
                {
                    free(bitmask);
                    return -1;
                }
            }

            gtype = grib_type[grib2_values];
            type = (coda_dynamic_type *)coda_grib_value_array_simple_packing_new((coda_type_array *)gtype, num_elements,
                                                                                 file_offset, bitsPerValue,
                                                                                 decimalScaleFactor, binaryScaleFactor,
                                                                                 referenceValue, bitmask);
            if (bitmask != NULL)
            {
                free(bitmask);
            }
            coda_mem_record_add_field(data, "values", type, 0);
            coda_mem_array_add_element(dataArray, (coda_dynamic_type *)data);

            if (section_size > 5)
            {
                file_offset += section_size - 5;
            }

            prev_section = 7;
        }
        else
        {
            char file_offset_str[21];

            coda_str64(file_offset, file_offset_str);
            coda_set_error(CODA_ERROR_PRODUCT, "invalid Section Number (%d) at offset %s", *buffer - '0',
                           file_offset_str);
            return -1;
        }

        /* read first four bytes of next section */
        if (read_bytes(product->raw_product, file_offset, 4, buffer) < 0)
        {
            return -1;
        }
        file_offset += 4;
    }

    if (prev_section != 7)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_PRODUCT, "Message contains no data at offset %s", file_offset_str);
        return -1;
    }

    return 0;
}

int coda_grib_reopen(coda_product **product)
{
    coda_dynamic_type *type;
    coda_grib_product *product_file;
    long message_number;
    uint8_t buffer[28];
    int64_t message_size;
    int64_t file_offset = 0;

    if (grib_init() != 0)
    {
        coda_close(*product);
        return -1;
    }

    product_file = (coda_grib_product *)malloc(sizeof(coda_grib_product));

    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_grib_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_grib;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;

    product_file->raw_product = *product;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_grib_close((coda_product *)product_file);
        return -1;
    }
    product_file->root_type = (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)grib_type[grib_root], NULL);

    message_number = 0;
    while (file_offset < product_file->file_size - 1)
    {
        coda_mem_record *message_union;
        coda_mem_record *message;
        int grib_version;

        /* find start of Indicator Section */
        buffer[0] = '\0';
        while (file_offset < product_file->file_size - 1 && buffer[0] != 'G')
        {
            if (read_bytes_in_bounds(product_file->raw_product, file_offset, 1, buffer) < 0)
            {
                coda_grib_close((coda_product *)product_file);
                return -1;
            }
            file_offset++;
        }
        if (file_offset >= product_file->file_size - 1)
        {
            /* there is only filler data at the end of the file, but no new message */
            break;
        }
        file_offset--;

        /* Section 0: Indicator Section */
        if (read_bytes(product_file->raw_product, file_offset, 8, buffer) < 0)
        {
            coda_grib_close((coda_product *)product_file);
            return -1;
        }
        if (buffer[0] != 'G' || buffer[1] != 'R' || buffer[2] != 'I' || buffer[3] != 'B')
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid indicator for message %ld", message_number);
            coda_grib_close((coda_product *)product_file);
            return -1;
        }

        grib_version = buffer[7];
        if (grib_version != 1 && grib_version != 2)
        {
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "unsupported GRIB format version (%d) for message %ld",
                           grib_version, message_number);
            coda_grib_close((coda_product *)product_file);
            return -1;
        }

        message_union = coda_mem_record_new((coda_type_record *)grib_type[grib_message], NULL);
        if (grib_version == 1)
        {
            /* read message based on GRIB Edition Number 1 specification */
            message_size = ((buffer[4] * 256) + buffer[5]) * 256 + buffer[6];

            message = coda_mem_record_new((coda_type_record *)grib_type[grib1_message], NULL);
            message_union->field_type[0] = (coda_dynamic_type *)message;
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)grib_type[grib1_editionNumber], NULL,
                                                           (coda_product *)product_file, 1);
            coda_mem_record_add_field(message, "editionNumber", type, 0);
            if (read_grib1_message(product_file, message, file_offset + 8) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)message_union);
                coda_grib_close((coda_product *)product_file);
                return -1;
            }
        }
        else
        {
            /* read message based on GRIB Edition Number 2 specification */
            if (read_bytes(product_file->raw_product, file_offset + 8, 8, &message_size) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)message_union);
                coda_grib_close((coda_product *)product_file);
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap_int64(&message_size);
#endif
            message = coda_mem_record_new((coda_type_record *)grib_type[grib2_message], NULL);
            message_union->field_type[1] = (coda_dynamic_type *)message;
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)grib_type[grib2_editionNumber], NULL,
                                                           (coda_product *)product_file, 2);
            coda_mem_record_add_field(message, "editionNumber", type, 0);
            type = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)grib_type[grib2_discipline], NULL,
                                                           (coda_product *)product_file, buffer[6]);
            coda_mem_record_add_field(message, "discipline", type, 0);

            if (read_grib2_message(product_file, message, file_offset + 16) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)message_union);
                coda_grib_close((coda_product *)product_file);
                return -1;
            }
        }

        if (coda_mem_array_add_element((coda_mem_array *)product_file->root_type, (coda_dynamic_type *)message_union) !=
            0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)message_union);
            coda_grib_close((coda_product *)product_file);
            return -1;
        }

        file_offset += message_size;
        message_number++;
    }

    *product = (coda_product *)product_file;
    return 0;
}

int coda_grib_close(coda_product *product)
{
    coda_grib_product *product_file = (coda_grib_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete(product_file->root_type);
    }
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
    }
    if (product_file->raw_product != NULL)
    {
        coda_bin_close((coda_product *)product_file->raw_product);
    }

    free(product_file);

    return 0;
}
