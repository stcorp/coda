/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#include "coda-internal.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-grib-internal.h"
#include "coda-definition.h"
#include "coda-mem-internal.h"

#define bit_size_to_byte_size(x) (((x) >> 3) + ((((uint8_t)(x)) & 0x7) != 0))

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
    grib1_pv,
    grib1_pv_array,
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
    grib1_root,

    grib2_localRecordIndex,
    grib2_gridRecordIndex,
    grib2_table2Version,
    grib2_editionNumber,
    grib2_centre,
    grib2_generatingProcessIdentifier,
    grib2_gridDefinition,
    grib2_indicatorOfParameter,
    grib2_indicatorOfTypeOfLevel,
    grib2_level,
    grib2_yearOfCentury,
    grib2_year,
    grib2_month,
    grib2_day,
    grib2_hour,
    grib2_minute,
    grib2_second,
    grib2_unitOfTimeRange,
    grib2_P1,
    grib2_P2,
    grib2_timeRangeIndicator,
    grib2_numberIncludedInAverage,
    grib2_numberMissingFromAveragesOrAccumulations,
    grib2_centuryOfReferenceTimeOfData,
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
    grib2_pv,
    grib2_pv_array,
    grib2_sourceOfGridDefinition,
    grib2_numberOfDataPoints,
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
    grib2_root,

    num_grib_types
};

static coda_type **grib_type = NULL;

static int grib_init(void)
{
    coda_type_record_field *field;
    coda_type *basic_type;
    int i;

    if (grib_type != NULL)
    {
        return 0;
    }

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

    grib_type[grib1_localRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_localRecordIndex], coda_native_type_int32);

    grib_type[grib1_gridRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_gridRecordIndex], coda_native_type_uint32);

    grib_type[grib1_table2Version] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_table2Version], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_table2Version],
                              "Parameter Table Version number, currently 3 for international exchange. "
                              "(Parameter table version numbers 128-254 are reserved for local use.)");

    grib_type[grib1_editionNumber] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_editionNumber], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_editionNumber], "GRIB edition number");

    grib_type[grib1_centre] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_centre], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_centre], "Identification of center");

    grib_type[grib1_generatingProcessIdentifier] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_generatingProcessIdentifier], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_generatingProcessIdentifier], "Generating process ID number");

    grib_type[grib1_gridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_gridDefinition], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_gridDefinition], "Grid Identification");

    grib_type[grib1_indicatorOfParameter] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_indicatorOfParameter], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_indicatorOfParameter], "Indicator of parameter and units");

    grib_type[grib1_indicatorOfTypeOfLevel] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_indicatorOfTypeOfLevel], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_indicatorOfTypeOfLevel], "Indicator of type of level or layer");

    grib_type[grib1_level] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_level], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_level], "Height, pressure, etc. of the level or layer");

    grib_type[grib1_yearOfCentury] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_yearOfCentury], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_yearOfCentury], "Year of century");

    grib_type[grib1_year] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_year], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_year], "Year");

    grib_type[grib1_month] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_month], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_month], "Month of year");

    grib_type[grib1_day] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_day], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_day], "Day of month");

    grib_type[grib1_hour] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_hour], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_hour], "Hour of day");

    grib_type[grib1_minute] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_minute], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_minute], "Minute of hour");

    grib_type[grib1_second] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_second], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_second], "Second of minute");

    grib_type[grib1_unitOfTimeRange] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_unitOfTimeRange], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_unitOfTimeRange], "Forecast time unit");

    grib_type[grib1_P1] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_P1], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_P1], "Period of time (Number of time units)");

    grib_type[grib1_P2] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_P2], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_P2], "Period of time (Number of time units)");

    grib_type[grib1_timeRangeIndicator] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_timeRangeIndicator], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_timeRangeIndicator], "Time range indicator");

    grib_type[grib1_numberIncludedInAverage] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_numberIncludedInAverage], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_numberIncludedInAverage], "Number included in average, when "
                              "timeRangeIndicator indicates an average or accumulation; otherwise set to zero.");

    grib_type[grib1_numberMissingFromAveragesOrAccumulations] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_numberMissingFromAveragesOrAccumulations], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_numberMissingFromAveragesOrAccumulations], "Number Missing from "
                              "averages or accumulations.");

    grib_type[grib1_centuryOfReferenceTimeOfData] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                      coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_centuryOfReferenceTimeOfData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_centuryOfReferenceTimeOfData], "Century of Initial (Reference) time "
                              "(=20 until Jan. 1, 2001)");

    grib_type[grib1_subCentre] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_subCentre], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_subCentre], "Identification of sub-center (allocated by the originating "
                              "center; See Table C)");

    grib_type[grib1_decimalScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_decimalScaleFactor], coda_native_type_int16);
    coda_type_set_description(grib_type[grib1_decimalScaleFactor], "The decimal scale factor D");

    grib_type[grib1_discipline] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_discipline], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_discipline], "GRIB Master Table Number");

    grib_type[grib1_masterTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_masterTablesVersion], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_masterTablesVersion], "GRIB Master Tables Version Number");

    grib_type[grib1_localTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_localTablesVersion], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_localTablesVersion], "GRIB Local Tables Version Number");

    grib_type[grib1_significanceOfReferenceTime] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_significanceOfReferenceTime], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_significanceOfReferenceTime], "Significance of Reference Time");

    grib_type[grib1_productionStatusOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                         coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_productionStatusOfProcessedData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_productionStatusOfProcessedData],
                              "Production status of processed data in this GRIB message");

    grib_type[grib1_typeOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_typeOfProcessedData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_typeOfProcessedData], "Type of processed data in this GRIB message");

    grib_type[grib1_local] = (coda_type *)coda_type_raw_new(coda_format_grib1);
    coda_type_set_description(grib_type[grib1_local], "Reserved for originating center use");

    grib_type[grib1_numberOfVerticalCoordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                          coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_numberOfVerticalCoordinateValues], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_numberOfVerticalCoordinateValues],
                              "NV, the number of vertical coordinate parameter");

    grib_type[grib1_dataRepresentationType] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_dataRepresentationType], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_dataRepresentationType], "Data representation type");

    grib_type[grib1_shapeOfTheEarth] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_shapeOfTheEarth], coda_native_type_uint8);

    grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                             coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfRadiusOfSphericalEarth], coda_native_type_uint8);

    grib_type[grib1_scaledValueOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                             coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfRadiusOfSphericalEarth], coda_native_type_uint32);

    grib_type[grib1_scaleFactorOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfEarthMajorAxis], coda_native_type_uint8);

    grib_type[grib1_scaledValueOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfEarthMajorAxis], coda_native_type_uint32);

    grib_type[grib1_scaleFactorOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaleFactorOfEarthMinorAxis], coda_native_type_uint8);

    grib_type[grib1_scaledValueOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scaledValueOfEarthMinorAxis], coda_native_type_uint32);

    grib_type[grib1_Ni] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_Ni], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_Ni], "No. of points along a latitude circle");

    grib_type[grib1_Nj] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_Nj], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_Nj], "No. of points along a longitude meridian");

    grib_type[grib1_basicAngleOfTheInitialProductionDomain] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                                coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_basicAngleOfTheInitialProductionDomain], coda_native_type_uint32);

    grib_type[grib1_subdivisionsOfBasicAngle] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_subdivisionsOfBasicAngle], coda_native_type_uint32);

    grib_type[grib1_latitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_latitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib1_latitudeOfFirstGridPoint], "La1 - latitude of first grid point, units: "
                              "millidegrees (degrees x 1000), values limited to range 0 - 90,000");

    grib_type[grib1_longitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                   coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_longitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib1_longitudeOfFirstGridPoint], "Lo1 - longitude of first grid point, "
                              "units: millidegrees (degrees x 1000), values limited to range 0 - 360,000");

    grib_type[grib1_resolutionAndComponentFlags] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_resolutionAndComponentFlags], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_resolutionAndComponentFlags], "Resolution and component flags");

    grib_type[grib1_latitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_latitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib1_latitudeOfLastGridPoint], "La2 - Latitude of last grid point (same "
                              "units and value range as latitudeOfFirstGridPoint)");

    grib_type[grib1_longitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_longitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib1_longitudeOfLastGridPoint], "Lo2 - Longitude of last grid point (same "
                              "units and value range as longitudeOfFirstGridPoint)");

    grib_type[grib1_iDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_iDirectionIncrement], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_iDirectionIncrement], "Di - Longitudinal Direction Increment (same "
                              "units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib1_jDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_jDirectionIncrement], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_jDirectionIncrement], "Dj - Latitudinal Direction Increment (same "
                              "units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib1_N] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_N], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_N], "N - number of latitude circles between a pole and the equator, "
                              "Mandatory if Gaussian Grid specified");

    grib_type[grib1_scanningMode] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_scanningMode], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_scanningMode], "Scanning mode flags");

    grib_type[grib1_pv] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_real_class);
    coda_type_set_read_type(grib_type[grib1_pv], coda_native_type_float);
    grib_type[grib1_pv_array] = (coda_type *)coda_type_array_new(coda_format_grib1);
    coda_type_set_description(grib_type[grib1_pv_array], "List of vertical coordinate parameters");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_pv_array], grib_type[grib1_pv]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_pv_array], NULL);

    grib_type[grib1_sourceOfGridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_sourceOfGridDefinition], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_sourceOfGridDefinition], "Source of grid definition");

    grib_type[grib1_numberOfDataPoints] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_numberOfDataPoints], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib1_numberOfDataPoints], "Number of data points");

    grib_type[grib1_gridDefinitionTemplateNumber] = (coda_type *)coda_type_number_new(coda_format_grib1,
                                                                                      coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_gridDefinitionTemplateNumber], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib1_gridDefinitionTemplateNumber], "Grid Definition Template Number");

    grib_type[grib1_bitsPerValue] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_bitsPerValue], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib1_bitsPerValue], "Number of bits into which a datum point is packed.");

    grib_type[grib1_binaryScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_integer_class);
    coda_type_set_read_type(grib_type[grib1_binaryScaleFactor], coda_native_type_int16);
    coda_type_set_description(grib_type[grib1_binaryScaleFactor], "The binary scale factor (E).");

    grib_type[grib1_referenceValue] = (coda_type *)coda_type_number_new(coda_format_grib1, coda_real_class);
    coda_type_set_read_type(grib_type[grib1_referenceValue], coda_native_type_float);
    coda_type_set_description(grib_type[grib1_referenceValue], "Reference value (minimum value). "
                              "This is the overall or 'global' minimum that has been subtracted from all the values.");

    grib_type[grib1_values] = (coda_type *)coda_type_array_new(coda_format_grib1);
    basic_type = (coda_type *)coda_type_number_new(coda_format_grib1, coda_real_class);
    coda_type_set_read_type(basic_type, coda_native_type_float);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_values], basic_type);
    coda_type_release(basic_type);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_values], NULL);

    grib_type[grib1_grid] = (coda_type *)coda_type_record_new(coda_format_grib1);
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
    field = coda_type_record_field_new("pv");
    coda_type_record_field_set_type(field, grib_type[grib1_pv_array]);
    coda_type_record_field_set_optional(field);
    coda_type_record_add_field((coda_type_record *)grib_type[grib1_grid], field);

    grib_type[grib1_data] = (coda_type *)coda_type_record_new(coda_format_grib1);
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

    grib_type[grib1_message] = (coda_type *)coda_type_record_new(coda_format_grib1);
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

    grib_type[grib1_root] = (coda_type *)coda_type_array_new(coda_format_grib1);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib1_root], grib_type[grib1_message]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib1_root], NULL);


    /* GRIB2 */

    grib_type[grib2_localRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_localRecordIndex], coda_native_type_int32);

    grib_type[grib2_gridRecordIndex] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_gridRecordIndex], coda_native_type_uint32);

    grib_type[grib2_table2Version] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_table2Version], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_table2Version],
                              "Parameter Table Version number, currently 3 for international exchange. "
                              "(Parameter table version numbers 128-254 are reserved for local use.)");

    grib_type[grib2_editionNumber] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_editionNumber], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_editionNumber], "GRIB edition number");

    grib_type[grib2_centre] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_centre], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_centre], "Identification of originating/generating centre");

    grib_type[grib2_generatingProcessIdentifier] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_generatingProcessIdentifier], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_generatingProcessIdentifier], "Generating process ID number");

    grib_type[grib2_gridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_gridDefinition], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_gridDefinition], "Grid Identification");

    grib_type[grib2_indicatorOfParameter] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_indicatorOfParameter], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_indicatorOfParameter], "Indicator of parameter and units");

    grib_type[grib2_indicatorOfTypeOfLevel] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_indicatorOfTypeOfLevel], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_indicatorOfTypeOfLevel], "Indicator of type of level or layer");

    grib_type[grib2_level] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_level], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_level], "Height, pressure, etc. of the level or layer");

    grib_type[grib2_yearOfCentury] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_yearOfCentury], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_yearOfCentury], "Year of century");

    grib_type[grib2_year] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_year], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_year], "Year");

    grib_type[grib2_month] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_month], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_month], "Month of year");

    grib_type[grib2_day] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_day], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_day], "Day of month");

    grib_type[grib2_hour] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_hour], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_hour], "Hour of day");

    grib_type[grib2_minute] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_minute], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_minute], "Minute of hour");

    grib_type[grib2_second] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_second], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_second], "Second of minute");

    grib_type[grib2_unitOfTimeRange] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_unitOfTimeRange], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_unitOfTimeRange], "Forecast time unit");

    grib_type[grib2_P1] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_P1], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_P1], "Period of time (Number of time units)");

    grib_type[grib2_P2] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_P2], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_P2], "Period of time (Number of time units)");

    grib_type[grib2_timeRangeIndicator] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_timeRangeIndicator], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_timeRangeIndicator], "Time range indicator");

    grib_type[grib2_numberIncludedInAverage] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_numberIncludedInAverage], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_numberIncludedInAverage], "Number included in average, when "
                              "timeRangeIndicator indicates an average or accumulation; otherwise set to zero.");

    grib_type[grib2_numberMissingFromAveragesOrAccumulations] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_numberMissingFromAveragesOrAccumulations], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_numberMissingFromAveragesOrAccumulations], "Number Missing from "
                              "averages or accumulations.");

    grib_type[grib2_centuryOfReferenceTimeOfData] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                      coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_centuryOfReferenceTimeOfData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_centuryOfReferenceTimeOfData], "Century of Initial (Reference) time "
                              "(=20 until Jan. 1, 2001)");

    grib_type[grib2_subCentre] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_subCentre], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_subCentre], "Identification of originating/generating sub-centre "
                              "(allocated by originating/generating centre)");

    grib_type[grib2_decimalScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_decimalScaleFactor], coda_native_type_int16);
    coda_type_set_description(grib_type[grib2_decimalScaleFactor], "The decimal scale factor D");

    grib_type[grib2_discipline] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_discipline], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_discipline], "GRIB Master Table Number");

    grib_type[grib2_masterTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_masterTablesVersion], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_masterTablesVersion], "GRIB Master Tables Version Number");

    grib_type[grib2_localTablesVersion] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_localTablesVersion], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_localTablesVersion], "GRIB Local Tables Version Number");

    grib_type[grib2_significanceOfReferenceTime] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_significanceOfReferenceTime], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_significanceOfReferenceTime], "Significance of Reference Time");

    grib_type[grib2_productionStatusOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                         coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_productionStatusOfProcessedData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_productionStatusOfProcessedData],
                              "Production status of processed data in this GRIB message");

    grib_type[grib2_typeOfProcessedData] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_typeOfProcessedData], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_typeOfProcessedData], "Type of processed data in this GRIB message");

    grib_type[grib2_local] = (coda_type *)coda_type_raw_new(coda_format_grib2);
    coda_type_set_description(grib_type[grib2_local], "Reserved for originating center use");

    grib_type[grib2_numberOfVerticalCoordinateValues] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                          coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_numberOfVerticalCoordinateValues], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_numberOfVerticalCoordinateValues],
                              "NV, the number of vertical coordinate parameter");

    grib_type[grib2_dataRepresentationType] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_dataRepresentationType], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_dataRepresentationType], "Data representation type");

    grib_type[grib2_shapeOfTheEarth] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_shapeOfTheEarth], coda_native_type_uint8);

    grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                             coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth], coda_native_type_uint8);

    grib_type[grib2_scaledValueOfRadiusOfSphericalEarth] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                             coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfRadiusOfSphericalEarth], coda_native_type_uint32);

    grib_type[grib2_scaleFactorOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfEarthMajorAxis], coda_native_type_uint8);

    grib_type[grib2_scaledValueOfEarthMajorAxis] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfEarthMajorAxis], coda_native_type_uint32);

    grib_type[grib2_scaleFactorOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaleFactorOfEarthMinorAxis], coda_native_type_uint8);

    grib_type[grib2_scaledValueOfEarthMinorAxis] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scaledValueOfEarthMinorAxis], coda_native_type_uint32);

    grib_type[grib2_Ni] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_Ni], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib2_Ni], "No. of points along a latitude circle");

    grib_type[grib2_Nj] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_Nj], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_Nj], "No. of points along a longitude meridian");

    grib_type[grib2_basicAngleOfTheInitialProductionDomain] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                                coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_basicAngleOfTheInitialProductionDomain], coda_native_type_uint32);

    grib_type[grib2_subdivisionsOfBasicAngle] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_subdivisionsOfBasicAngle], coda_native_type_uint32);

    grib_type[grib2_latitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_latitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib2_latitudeOfFirstGridPoint], "La1 - latitude of first grid point, units: "
                              "millidegrees (degrees x 1000), values limited to range 0 - 90,000");

    grib_type[grib2_longitudeOfFirstGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                   coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_longitudeOfFirstGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib2_longitudeOfFirstGridPoint], "Lo1 - longitude of first grid point, "
                              "units: millidegrees (degrees x 1000), values limited to range 0 - 360,000");

    grib_type[grib2_resolutionAndComponentFlags] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                     coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_resolutionAndComponentFlags], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_resolutionAndComponentFlags], "Resolution and component flags");

    grib_type[grib2_latitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_latitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib2_latitudeOfLastGridPoint], "La2 - Latitude of last grid point (same "
                              "units and value range as latitudeOfFirstGridPoint)");

    grib_type[grib2_longitudeOfLastGridPoint] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                  coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_longitudeOfLastGridPoint], coda_native_type_int32);
    coda_type_set_description(grib_type[grib2_longitudeOfLastGridPoint], "Lo2 - Longitude of last grid point (same "
                              "units and value range as longitudeOfFirstGridPoint)");

    grib_type[grib2_iDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_iDirectionIncrement], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib2_iDirectionIncrement], "Di - Longitudinal Direction Increment (same "
                              "units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib2_jDirectionIncrement] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_jDirectionIncrement], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib2_jDirectionIncrement], "Dj - Latitudinal Direction Increment (same "
                              "units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_type[grib2_N] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_N], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib2_N], "N - number of latitude circles between a pole and the equator, "
                              "Mandatory if Gaussian Grid specified");

    grib_type[grib2_scanningMode] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_scanningMode], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_scanningMode], "Scanning mode flags");

    grib_type[grib2_pv] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_real_class);
    coda_type_set_read_type(grib_type[grib2_pv], coda_native_type_float);
    grib_type[grib2_pv_array] = (coda_type *)coda_type_array_new(coda_format_grib2);
    coda_type_set_description(grib_type[grib2_pv_array], "List of vertical coordinate parameters");
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_pv_array], grib_type[grib2_pv]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_pv_array], NULL);

    grib_type[grib2_sourceOfGridDefinition] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_sourceOfGridDefinition], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_sourceOfGridDefinition], "Source of grid definition");

    grib_type[grib2_numberOfDataPoints] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_numberOfDataPoints], coda_native_type_uint32);
    coda_type_set_description(grib_type[grib2_numberOfDataPoints], "Number of data points");

    grib_type[grib2_gridDefinitionTemplateNumber] = (coda_type *)coda_type_number_new(coda_format_grib2,
                                                                                      coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_gridDefinitionTemplateNumber], coda_native_type_uint16);
    coda_type_set_description(grib_type[grib2_gridDefinitionTemplateNumber], "Grid Definition Template Number");

    grib_type[grib2_bitsPerValue] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_bitsPerValue], coda_native_type_uint8);
    coda_type_set_description(grib_type[grib2_bitsPerValue], "Number of bits into which a datum point is packed.");

    grib_type[grib2_binaryScaleFactor] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_integer_class);
    coda_type_set_read_type(grib_type[grib2_binaryScaleFactor], coda_native_type_int16);
    coda_type_set_description(grib_type[grib2_binaryScaleFactor], "The binary scale factor (E).");

    grib_type[grib2_referenceValue] = (coda_type *)coda_type_number_new(coda_format_grib2, coda_real_class);
    coda_type_set_read_type(grib_type[grib2_referenceValue], coda_native_type_float);
    coda_type_set_description(grib_type[grib2_referenceValue], "Reference value (minimum value). "
                              "This is the overall or 'global' minimum that has been subtracted from all the values.");

    grib_type[grib2_values] = (coda_type *)coda_type_array_new(coda_format_grib2);
    basic_type = (coda_type *)coda_type_number_new(coda_format_grib2, coda_real_class);
    coda_type_set_read_type(basic_type, coda_native_type_float);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_values], basic_type);
    coda_type_release(basic_type);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_values], NULL);

    grib_type[grib2_grid] = (coda_type *)coda_type_record_new(coda_format_grib2);
    field = coda_type_record_field_new("localRecordIndex");
    coda_type_record_field_set_type(field, grib_type[grib2_localRecordIndex]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("sourceOfGridDefinition");
    coda_type_record_field_set_type(field, grib_type[grib2_sourceOfGridDefinition]);
    coda_type_record_add_field((coda_type_record *)grib_type[grib2_grid], field);
    field = coda_type_record_field_new("numberOfDataPoints");
    coda_type_record_field_set_type(field, grib_type[grib2_numberOfDataPoints]);
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

    grib_type[grib2_data] = (coda_type *)coda_type_record_new(coda_format_grib2);
    field = coda_type_record_field_new("gridRecordIndex");
    coda_type_record_field_set_type(field, grib_type[grib2_gridRecordIndex]);
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

    grib_type[grib2_local_array] = (coda_type *)coda_type_array_new(coda_format_grib2);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_local_array], grib_type[grib2_local]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_local_array], NULL);

    grib_type[grib2_grid_array] = (coda_type *)coda_type_array_new(coda_format_grib2);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_grid_array], grib_type[grib2_grid]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_grid_array], NULL);

    grib_type[grib2_data_array] = (coda_type *)coda_type_array_new(coda_format_grib2);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_data_array], grib_type[grib2_data]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_data_array], NULL);

    grib_type[grib2_message] = (coda_type *)coda_type_record_new(coda_format_grib2);
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

    grib_type[grib2_root] = (coda_type *)coda_type_array_new(coda_format_grib2);
    coda_type_array_set_base_type((coda_type_array *)grib_type[grib2_root], grib_type[grib2_message]);
    coda_type_array_add_variable_dimension((coda_type_array *)grib_type[grib2_root], NULL);

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

#ifndef WORDS_BIGENDIAN
static void swap4(void *value)
{
    union
    {
        uint8_t as_bytes[4];
        int32_t as_int32;
    } v1, v2;

    v1.as_int32 = *(int32_t *)value;

    v2.as_bytes[0] = v1.as_bytes[3];
    v2.as_bytes[1] = v1.as_bytes[2];
    v2.as_bytes[2] = v1.as_bytes[1];
    v2.as_bytes[3] = v1.as_bytes[0];

    *(int32_t *)value = v2.as_int32;
}

static void swap8(void *value)
{
    union
    {
        uint8_t as_bytes[8];
        int64_t as_int64;
    } v1, v2;

    v1.as_int64 = *(int64_t *)value;

    v2.as_bytes[0] = v1.as_bytes[7];
    v2.as_bytes[1] = v1.as_bytes[6];
    v2.as_bytes[2] = v1.as_bytes[5];
    v2.as_bytes[3] = v1.as_bytes[4];
    v2.as_bytes[4] = v1.as_bytes[3];
    v2.as_bytes[5] = v1.as_bytes[2];
    v2.as_bytes[6] = v1.as_bytes[1];
    v2.as_bytes[7] = v1.as_bytes[0];

    *(int64_t *)value = v2.as_int64;
}
#endif

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
        return coda_PlusInf();
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
    coda_dynamic_type *type;
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
    int isIntegerData;
    int64_t intvalue;

    /* Section 1: Product Definition Section (PDS) */
    if (read(product->fd, buffer, 28) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
        return -1;
    }

    section_size = ((buffer[0] * 256) + buffer[1]) * 256 + buffer[2];

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_table2Version], buffer[3]);
    coda_mem_record_add_field(message, "table2Version", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_centre], buffer[4]);
    coda_mem_record_add_field(message, "centre", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_generatingProcessIdentifier],
                                                     buffer[5]);
    coda_mem_record_add_field(message, "generatingProcessIdentifier", type, 0);

    gridDefinition = buffer[6];
    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_gridDefinition],
                                                     gridDefinition);
    coda_mem_record_add_field(message, "gridDefinition", type, 0);

    has_gds = buffer[7] & 0x80 ? 1 : 0;
    has_bms = buffer[7] & 0x40 ? 1 : 0;

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_indicatorOfParameter],
                                                     buffer[8]);
    coda_mem_record_add_field(message, "indicatorOfParameter", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_indicatorOfTypeOfLevel],
                                                     buffer[9]);
    coda_mem_record_add_field(message, "indicatorOfTypeOfLevel", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_level],
                                                     buffer[10] * 256 + buffer[11]);
    coda_mem_record_add_field(message, "level", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_yearOfCentury], buffer[12]);
    coda_mem_record_add_field(message, "yearOfCentury", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_month], buffer[13]);
    coda_mem_record_add_field(message, "month", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_day], buffer[14]);
    coda_mem_record_add_field(message, "day", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_hour], buffer[15]);
    coda_mem_record_add_field(message, "hour", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_minute], buffer[16]);
    coda_mem_record_add_field(message, "minute", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_unitOfTimeRange], buffer[17]);
    coda_mem_record_add_field(message, "unitOfTimeRange", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_P1], buffer[18]);
    coda_mem_record_add_field(message, "P1", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_P2], buffer[19]);
    coda_mem_record_add_field(message, "P2", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_timeRangeIndicator],
                                                     buffer[20]);
    coda_mem_record_add_field(message, "timeRangeIndicator", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_numberIncludedInAverage],
                                                     buffer[21] * 256 + buffer[22]);
    coda_mem_record_add_field(message, "numberIncludedInAverage", type, 0);

    type =
        (coda_dynamic_type *)
        coda_mem_integer_new((coda_type_number *)grib_type[grib1_numberMissingFromAveragesOrAccumulations], buffer[23]);
    coda_mem_record_add_field(message, "numberMissingFromAveragesOrAccumulations", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_centuryOfReferenceTimeOfData],
                                                     buffer[24]);
    coda_mem_record_add_field(message, "centuryOfReferenceTimeOfData", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_subCentre], buffer[25]);
    coda_mem_record_add_field(message, "subCentre", type, 0);

    decimalScaleFactor = (buffer[26] & 0x80 ? -1 : 1) * ((buffer[26] & 0x7F) * 256 + buffer[27]);
    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_decimalScaleFactor],
                                                     decimalScaleFactor);
    coda_mem_record_add_field(message, "decimalScaleFactor", type, 0);

    file_offset += 28;

    if (section_size > 28)
    {
        if (section_size > 40)
        {
            uint8_t *raw_data;

            /* skip bytes 29-40 which are reserved */
            file_offset += 12;
            if (lseek(product->fd, 12, SEEK_CUR) < 0)
            {
                char file_offset_str[21];

                coda_str64(file_offset, file_offset_str);
                coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                               file_offset_str, product->filename, strerror(errno));
                return -1;
            }
            raw_data = malloc((section_size - 40) * sizeof(uint8_t));
            if (raw_data == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)((section_size - 40) * sizeof(uint8_t)), __FILE__, __LINE__);
                return -1;
            }
            if (read(product->fd, raw_data, section_size - 40) < 0)
            {
                free(raw_data);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
                return -1;
            }
            type = (coda_dynamic_type *)coda_mem_raw_new((coda_type_raw *)grib_type[grib1_local], section_size - 40,
                                                         raw_data);
            coda_mem_record_add_field(message, "local", type, 0);
            file_offset += section_size - 40;
        }
        else
        {
            file_offset += section_size - 28;
            if (lseek(product->fd, (off_t)(section_size - 28), SEEK_CUR) < 0)
            {
                char file_offset_str[21];

                coda_str64(file_offset + section_size - 28, file_offset_str);
                coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                               file_offset_str, product->filename, strerror(errno));
                return -1;
            }
        }
    }

    if (has_gds)
    {
        coda_mem_record *gds;

        /* Section 2: Grid Description Section (GDS) */
        if (read(product->fd, buffer, 6) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            return -1;
        }

        section_size = ((buffer[0] * 256) + buffer[1]) * 256 + buffer[2];

        file_offset += 6;

        if (buffer[5] == 0 || buffer[5] == 4 || buffer[5] == 10 || buffer[5] == 14 || buffer[5] == 20 ||
            buffer[5] == 24 || buffer[5] == 30 || buffer[5] == 34)
        {
            int is_gaussian = (buffer[5] == 4 || buffer[5] == 14 || buffer[5] == 24 || buffer[5] == 34);
            uint8_t NV;
            uint8_t PVL;

            /* data representation type is Latitude/Longitude Grid */
            gds = coda_mem_record_new((coda_type_record *)grib_type[grib1_grid]);

            NV = buffer[3];
            type =
                (coda_dynamic_type *)
                coda_mem_integer_new((coda_type_number *)grib_type[grib1_numberOfVerticalCoordinateValues], NV);
            coda_mem_record_add_field(gds, "numberOfVerticalCoordinateValues", type, 0);

            PVL = buffer[4];

            type =
                (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_dataRepresentationType],
                                                          buffer[5]);
            coda_mem_record_add_field(gds, "dataRepresentationType", type, 0);

            if (read(product->fd, buffer, 26) < 0)
            {
                coda_grib_type_delete((coda_dynamic_type *)gds);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
                return -1;
            }

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_Ni],
                                                             buffer[0] * 256 + buffer[1]);
            coda_mem_record_add_field(gds, "Ni", type, 0);
            if (buffer[0] * 256 + buffer[1] == 65535)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "grid definition with MISSING value (65535) for Ni not supported");
                return -1;
            }
            num_elements = buffer[0] * 256 + buffer[1];

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_Nj],
                                                             buffer[2] * 256 + buffer[3]);
            coda_mem_record_add_field(gds, "Nj", type, 0);
            if (buffer[2] * 256 + buffer[3] == 65535)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "grid definition with MISSING value (65535) for Nj not supported");
                return -1;
            }
            num_elements *= buffer[2] * 256 + buffer[3];

            intvalue = (buffer[4] & 0x80 ? -1 : 1) * (((buffer[4] & 0x7F) * 256 + buffer[5]) * 256 + buffer[6]);
            type =
                (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_latitudeOfFirstGridPoint],
                                                          intvalue);
            coda_mem_record_add_field(gds, "latitudeOfFirstGridPoint", type, 0);

            intvalue = (buffer[7] & 0x80 ? -1 : 1) * (((buffer[7] & 0x7F) * 256 + buffer[8]) * 256 + buffer[9]);
            type =
                (coda_dynamic_type *)
                coda_mem_integer_new((coda_type_number *)grib_type[grib1_longitudeOfFirstGridPoint], intvalue);
            coda_mem_record_add_field(gds, "longitudeOfFirstGridPoint", type, 0);

            type =
                (coda_dynamic_type *)
                coda_mem_integer_new((coda_type_number *)grib_type[grib1_resolutionAndComponentFlags], buffer[10]);
            coda_mem_record_add_field(gds, "resolutionAndComponentFlags", type, 0);

            intvalue = (buffer[11] & 0x80 ? -1 : 1) * (((buffer[11] & 0x7F) * 256 + buffer[12]) * 256 + buffer[13]);
            type =
                (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_latitudeOfLastGridPoint],
                                                          intvalue);
            coda_mem_record_add_field(gds, "latitudeOfLastGridPoint", type, 0);

            intvalue = (buffer[14] & 0x80 ? -1 : 1) * (((buffer[14] & 0x7F) * 256 + buffer[15]) * 256 + buffer[16]);
            type =
                (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_longitudeOfLastGridPoint],
                                                          intvalue);
            coda_mem_record_add_field(gds, "longitudeOfLastGridPoint", type, 0);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_iDirectionIncrement],
                                                             buffer[17] * 256 + buffer[18]);
            coda_mem_record_add_field(gds, "iDirectionIncrement", type, 0);

            if (is_gaussian)
            {
                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_N],
                                                                 buffer[19] * 256 + buffer[20]);
                coda_mem_record_add_field(gds, "N", type, 0);
            }
            else
            {
                type =
                    (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_jDirectionIncrement],
                                                              buffer[19] * 256 + buffer[20]);
                coda_mem_record_add_field(gds, "jDirectionIncrement", type, 0);
            }

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_scanningMode],
                                                             buffer[21]);
            coda_mem_record_add_field(gds, "scanningMode", type, 0);

            file_offset += 26;

            if (PVL != 255)
            {
                PVL--;  /* make offset zero based */
                file_offset += PVL - 32;
                if (lseek(product->fd, (off_t)(PVL - 32), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_dynamic_type_delete((coda_dynamic_type *)gds);
                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
                if (NV > 0)
                {
                    coda_mem_array *pvArray;
                    int i;

                    pvArray = coda_mem_array_new((coda_type_array *)grib_type[grib1_pv_array]);
                    for (i = 0; i < NV; i++)
                    {
                        if (read(product->fd, buffer, 4) < 0)
                        {
                            coda_dynamic_type_delete((coda_dynamic_type *)pvArray);
                            coda_dynamic_type_delete((coda_dynamic_type *)gds);
                            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                           strerror(errno));
                            return -1;
                        }
                        type = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)grib_type[grib1_pv],
                                                                      ibmfloat_to_iee754(buffer));
                        coda_mem_array_add_element(pvArray, type);

                        file_offset += 4;
                    }
                    coda_mem_record_add_field(gds, "pv", (coda_dynamic_type *)pvArray, 0);
                }
                if (section_size > PVL + NV * 4)
                {
                    file_offset += section_size - (PVL + NV * 4);
                    if (lseek(product->fd, (off_t)(section_size - (PVL + 4 * NV)), SEEK_CUR) < 0)
                    {
                        char file_offset_str[21];

                        coda_dynamic_type_delete((coda_dynamic_type *)gds);
                        coda_str64(file_offset, file_offset_str);
                        coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                       file_offset_str, product->filename, strerror(errno));
                        return -1;
                    }
                }
            }
            else if (section_size > 32)
            {
                file_offset += section_size - 32;
                if (lseek(product->fd, (off_t)(section_size - 32), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_dynamic_type_delete((coda_dynamic_type *)gds);
                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
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
        if (read(product->fd, buffer, 6) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            return -1;
        }

        section_size = ((buffer[0] * 256) + buffer[1]) * 256 + buffer[2];
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
        if (read(product->fd, bitmask, section_size - 6) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            return -1;
        }
        file_offset += section_size;
    }

    /* Section 4: Binary Data Section (BDS) */
    if (read(product->fd, buffer, 11) < 0)
    {
        if (bitmask != NULL)
        {
            free(bitmask);
        }
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
        return -1;
    }

    section_size = ((buffer[0] * 256) + buffer[1]) * 256 + buffer[2];
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
    isIntegerData = (buffer[3] & 0x20 ? 1 : 0);
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
    bds = coda_mem_record_new((coda_type_record *)grib_type[grib1_data]);
    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_bitsPerValue], bitsPerValue);
    coda_mem_record_add_field(bds, "bitsPerValue", type, 0);
    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_binaryScaleFactor],
                                                     binaryScaleFactor);
    coda_mem_record_add_field(bds, "binaryScaleFactor", type, 0);
    type = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)grib_type[grib1_referenceValue], referenceValue);
    coda_mem_record_add_field(bds, "referenceValue", type, 0);

    file_offset += 11;

    type = (coda_dynamic_type *)coda_grib_value_array_new((coda_type_array *)grib_type[grib1_values], num_elements,
                                                          file_offset, bitsPerValue, decimalScaleFactor,
                                                          binaryScaleFactor, referenceValue, bitmask);
    if (bitmask != NULL)
    {
        free(bitmask);
    }
    coda_mem_record_add_field(bds, "values", type, 0);

    coda_mem_record_add_field(message, "data", (coda_dynamic_type *)bds, 0);

    file_offset += section_size - 11;
    if (lseek(product->fd, (off_t)(section_size - 11), SEEK_CUR) < 0)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                       file_offset_str, product->filename, strerror(errno));
        return -1;
    }

    /* Section 5: '7777' (ASCII Characters) */
    if (read(product->fd, buffer, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
        return -1;
    }
    if (memcmp(buffer, "7777", 4) != 0)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_FILE_READ, "invalid GRIB termination section at byte position %s in file %s",
                       file_offset_str, product->filename);
        return -1;
    }
    file_offset += 4;

    return 0;
}

static int read_grib2_message(coda_grib_product *product, coda_mem_record *message, int64_t file_offset)
{
    coda_mem_array *localArray;
    coda_mem_array *gridArray;
    coda_mem_array *dataArray;
    coda_dynamic_type *type;
    int has_bitmask = 0;
    int64_t bitmask_offset = -1;
    int64_t bitmask_length = 0;
    long localRecordIndex = -1;
    long gridSectionIndex = -1;
    int16_t decimalScaleFactor = 0;
    int16_t binaryScaleFactor = 0;
    float referenceValue = 0;
    uint8_t bitsPerValue = 0;
    uint32_t num_elements;
    uint32_t section_size;
    uint8_t buffer[64];
    uint8_t prev_section;

    /* Section 1: Identification Section */
    if (read(product->fd, buffer, 21) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
        return -1;
    }

    section_size = *(uint32_t *)buffer;
#ifndef WORDS_BIGENDIAN
    swap4(&section_size);
#endif

    if (buffer[4] != 1)
    {
        char file_offset_str[21];

        coda_str64(file_offset, file_offset_str);
        coda_set_error(CODA_ERROR_PRODUCT, "wrong Section Number (%d) for Identification Section at offset %s",
                       buffer[4] - '0', file_offset_str);
        return -1;
    }
    prev_section = 1;

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_centre],
                                                     buffer[5] * 256 + buffer[6]);
    coda_mem_record_add_field(message, "centre", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_subCentre],
                                                     buffer[7] * 256 + buffer[8]);
    coda_mem_record_add_field(message, "subCentre", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_masterTablesVersion],
                                                     buffer[9]);
    coda_mem_record_add_field(message, "masterTablesVersion", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_localTablesVersion],
                                                     buffer[10]);
    coda_mem_record_add_field(message, "localTablesVersion", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_significanceOfReferenceTime],
                                                     buffer[11]);
    coda_mem_record_add_field(message, "significanceOfReferenceTime", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_year],
                                                     buffer[12] * 256 + buffer[13]);
    coda_mem_record_add_field(message, "year", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_month], buffer[14]);
    coda_mem_record_add_field(message, "month", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_day], buffer[15]);
    coda_mem_record_add_field(message, "day", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_hour], buffer[16]);
    coda_mem_record_add_field(message, "hour", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_minute], buffer[17]);
    coda_mem_record_add_field(message, "minute", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_second], buffer[18]);
    coda_mem_record_add_field(message, "second", type, 0);

    type =
        (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_productionStatusOfProcessedData],
                                                  buffer[19]);
    coda_mem_record_add_field(message, "productionStatusOfProcessedData", type, 0);

    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_typeOfProcessedData],
                                                     buffer[20]);
    coda_mem_record_add_field(message, "typeOfProcessedData", type, 0);

    localArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_local_array]);
    coda_mem_record_add_field(message, "local", (coda_dynamic_type *)localArray, 0);

    gridArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_grid_array]);
    coda_mem_record_add_field(message, "grid", (coda_dynamic_type *)gridArray, 0);

    dataArray = coda_mem_array_new((coda_type_array *)grib_type[grib2_data_array]);
    coda_mem_record_add_field(message, "data", (coda_dynamic_type *)dataArray, 0);

    file_offset += 21;

    if (section_size > 21)
    {
        file_offset += section_size - 21;
        if (lseek(product->fd, (off_t)(section_size - 21), SEEK_CUR) < 0)
        {
            char file_offset_str[21];

            coda_str64(file_offset, file_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                           file_offset_str, product->filename, strerror(errno));
            return -1;
        }
    }

    /* keep looping over message sections until we find section 8 or until we encounter an error */
    if (read(product->fd, buffer, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
        return -1;
    }
    file_offset += 4;
    while (memcmp(buffer, "7777", 4) != 0)
    {
        section_size = *(uint32_t *)buffer;
#ifndef WORDS_BIGENDIAN
        swap4(&section_size);
#endif

        /* read section number */
        if (read(product->fd, buffer, 1) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
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
                if (read(product->fd, raw_data, section_size - 5) < 0)
                {
                    free(raw_data);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                   strerror(errno));
                    return -1;
                }
                type = (coda_dynamic_type *)coda_mem_raw_new((coda_type_raw *)grib_type[grib1_local],
                                                             section_size - 5, raw_data);
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

            /* Section 3: Grid Definition Section */
            if (prev_section != 1 && prev_section != 2 && prev_section != 7)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (read(product->fd, buffer, 9) < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
                return -1;
            }

            grid = coda_mem_record_new((coda_type_record *)grib_type[grib2_grid]);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_localRecordIndex],
                                                             localRecordIndex);
            coda_mem_record_add_field(grid, "localRecordIndex", type, 0);

            type =
                (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_sourceOfGridDefinition],
                                                          buffer[0]);
            coda_mem_record_add_field(grid, "sourceOfGridDefinition", type, 0);

            num_data_points = ((buffer[1] * 256 + buffer[2]) * 256 + buffer[3]) * 256 + buffer[4];
#ifndef WORDS_BIGENDIAN
            swap4(&num_data_points);
#endif
            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_numberOfDataPoints],
                                                             num_data_points);
            coda_mem_record_add_field(grid, "numberOfDataPoints", type, 0);

            template_number = buffer[7] * 256 + buffer[8];
            type =
                (coda_dynamic_type *)
                coda_mem_integer_new((coda_type_number *)grib_type[grib2_gridDefinitionTemplateNumber],
                                     template_number);
            coda_mem_record_add_field(grid, "gridDefinitionTemplateNumber", type, 0);

            file_offset += 9;

            if (buffer[0] == 0 && (template_number <= 3 || (template_number >= 40 && template_number <= 43)))
            {
                uint32_t intvalue;

                if (read(product->fd, buffer, 58) < 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)grid);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                   strerror(errno));
                    return -1;
                }

                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_shapeOfTheEarth],
                                                                 buffer[0]);
                coda_mem_record_add_field(grid, "shapeOfTheEarth", type, 0);

                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaleFactorOfRadiusOfSphericalEarth],
                                         buffer[1]);
                coda_mem_record_add_field(grid, "scaleFactorOfRadiusOfSphericalEarth", type, 0);

                intvalue = ((buffer[2] * 256 + buffer[3]) * 256 + buffer[4]) * 256 + buffer[5];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaledValueOfRadiusOfSphericalEarth],
                                         intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfRadiusOfSphericalEarth", type, 0);

                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaleFactorOfEarthMajorAxis], buffer[6]);
                coda_mem_record_add_field(grid, "scaleFactorOfEarthMajorAxis", type, 0);

                intvalue = ((buffer[7] * 256 + buffer[8]) * 256 + buffer[9]) * 256 + buffer[10];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaledValueOfEarthMajorAxis], intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfEarthMajorAxis", type, 0);

                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaleFactorOfEarthMinorAxis], buffer[11]);
                coda_mem_record_add_field(grid, "scaleFactorOfEarthMinorAxis", type, 0);

                intvalue = ((buffer[12] * 256 + buffer[13]) * 256 + buffer[14]) * 256 + buffer[15];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_scaledValueOfEarthMinorAxis], intvalue);
                coda_mem_record_add_field(grid, "scaledValueOfEarthMinorAxis", type, 0);

                intvalue = ((buffer[16] * 256 + buffer[17]) * 256 + buffer[18]) * 256 + buffer[19];
                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_Ni], intvalue);
                coda_mem_record_add_field(grid, "Ni", type, 0);

                intvalue = ((buffer[20] * 256 + buffer[21]) * 256 + buffer[22]) * 256 + buffer[23];
                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_Nj], intvalue);
                coda_mem_record_add_field(grid, "Nj", type, 0);

                intvalue = ((buffer[24] * 256 + buffer[25]) * 256 + buffer[26]) * 256 + buffer[27];
                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type
                                                                 [grib2_basicAngleOfTheInitialProductionDomain],
                                                                 intvalue);
                coda_mem_record_add_field(grid, "basicAngleOfTheInitialProductionDomain", type, 0);

                intvalue = ((buffer[28] * 256 + buffer[29]) * 256 + buffer[30]) * 256 + buffer[31];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_subdivisionsOfBasicAngle], intvalue);
                coda_mem_record_add_field(grid, "subdivisionsOfBasicAngle", type, 0);

                intvalue = ((buffer[32] * 256 + buffer[33]) * 256 + buffer[34]) * 256 + buffer[35];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_latitudeOfFirstGridPoint],
                                         (buffer[32] & 0x80 ? -(intvalue - (1 << 31)) : intvalue));
                coda_mem_record_add_field(grid, "latitudeOfFirstGridPoint", type, 0);

                intvalue = ((buffer[36] * 256 + buffer[37]) * 256 + buffer[38]) * 256 + buffer[39];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_longitudeOfFirstGridPoint],
                                         (buffer[36] & 0x80 ? -(intvalue - (1 << 31)) : intvalue));
                coda_mem_record_add_field(grid, "longitudeOfFirstGridPoint", type, 0);

                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_resolutionAndComponentFlags], buffer[40]);
                coda_mem_record_add_field(grid, "resolutionAndComponentFlags", type, 0);

                intvalue = ((buffer[41] * 256 + buffer[42]) * 256 + buffer[43]) * 256 + buffer[44];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_latitudeOfLastGridPoint],
                                         (buffer[41] & 0x80 ? -(intvalue - (1 << 31)) : intvalue));
                coda_mem_record_add_field(grid, "latitudeOfLastGridPoint", type, 0);

                intvalue = ((buffer[45] * 256 + buffer[46]) * 256 + buffer[47]) * 256 + buffer[48];
                type =
                    (coda_dynamic_type *)
                    coda_mem_integer_new((coda_type_number *)grib_type[grib2_longitudeOfLastGridPoint],
                                         (buffer[45] & 0x80 ? -(intvalue - (1 << 31)) : intvalue));
                coda_mem_record_add_field(grid, "longitudeOfLastGridPoint", type, 0);

                intvalue = ((buffer[49] * 256 + buffer[50]) * 256 + buffer[51]) * 256 + buffer[52];
                type =
                    (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_iDirectionIncrement],
                                                              intvalue);
                coda_mem_record_add_field(grid, "iDirectionIncrement", type, 0);

                intvalue = ((buffer[53] * 256 + buffer[54]) * 256 + buffer[55]) * 256 + buffer[56];
                if (template_number >= 40 && template_number <= 43)
                {
                    type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_N], intvalue);
                    coda_mem_record_add_field(grid, "N", type, 0);
                }
                else
                {
                    type =
                        (coda_dynamic_type *)
                        coda_mem_integer_new((coda_type_number *)grib_type[grib2_jDirectionIncrement], intvalue);
                    coda_mem_record_add_field(grid, "jDirectionIncrement", type, 0);
                }

                type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_scanningMode],
                                                                 buffer[57]);
                coda_mem_record_add_field(grid, "scanningMode", type, 0);

                file_offset += 58;

                if (section_size > 72)
                {
                    file_offset += section_size - 72;
                    if (lseek(product->fd, (off_t)(section_size - 72), SEEK_CUR) < 0)
                    {
                        char file_offset_str[21];

                        coda_dynamic_type_delete((coda_dynamic_type *)grid);
                        coda_str64(file_offset, file_offset_str);
                        coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                       file_offset_str, product->filename, strerror(errno));
                        return -1;
                    }
                }
            }
            else
            {
                coda_dynamic_type_delete((coda_dynamic_type *)grid);
                coda_set_error(CODA_ERROR_PRODUCT, "unsupported grid source/template (%d/%d)", buffer[0],
                               template_number);
                return -1;
            }

            coda_mem_array_add_element(gridArray, (coda_dynamic_type *)grid);

            gridSectionIndex++;
            prev_section = 3;
        }
        else if (*buffer == 4)
        {
            /* Section 4: Product Definition Section */
            if (prev_section != 3 && prev_section != 7)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            if (section_size > 5)
            {
                file_offset += section_size - 5;
                if (lseek(product->fd, (off_t)(section_size - 5), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
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

            if (read(product->fd, buffer, 6) < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
                return -1;
            }
            num_elements = *((uint32_t *)buffer);
#ifndef WORDS_BIGENDIAN
            swap4(&num_elements);
#endif
            dataRepresentationTemplate = buffer[4] * 256 + buffer[5];
            file_offset += 6;

            if (dataRepresentationTemplate == 0 || dataRepresentationTemplate == 1)
            {
                if (read(product->fd, buffer, 9) < 0)
                {
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                   strerror(errno));
                    return -1;
                }
                referenceValue = *((float *)buffer);
#ifndef WORDS_BIGENDIAN
                swap4(&referenceValue);
#endif
                binaryScaleFactor = (buffer[4] & 0x80 ? -1 : 1) * ((buffer[4] & 0x7F) * 256 + buffer[5]);
                decimalScaleFactor = (buffer[6] & 0x80 ? -1 : 1) * ((buffer[6] & 0x7F) * 256 + buffer[7]);
                bitsPerValue = buffer[8];
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
                if (lseek(product->fd, (off_t)(section_size - 20), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
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

            if (read(product->fd, buffer, 1) < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
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
                if (lseek(product->fd, (off_t)(section_size - 6), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
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

            data = coda_mem_record_new((coda_type_record *)grib_type[grib2_data]);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_gridRecordIndex],
                                                             gridSectionIndex);
            coda_mem_record_add_field(data, "gridRecordIndex", type, 0);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_bitsPerValue],
                                                             bitsPerValue);
            coda_mem_record_add_field(data, "bitsPerValue", type, 0);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_decimalScaleFactor],
                                                             decimalScaleFactor);
            coda_mem_record_add_field(data, "decimalScaleFactor", type, 0);

            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_binaryScaleFactor],
                                                             binaryScaleFactor);
            coda_mem_record_add_field(data, "binaryScaleFactor", type, 0);

            type = (coda_dynamic_type *)coda_mem_real_new((coda_type_number *)grib_type[grib2_referenceValue],
                                                          referenceValue);
            coda_mem_record_add_field(data, "referenceValue", type, 0);

            if (has_bitmask)
            {
                /* read bitmask array */
                bitmask = malloc(bitmask_length * sizeof(uint8_t));
                if (bitmask == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)(bitmask_length * sizeof(uint8_t)), __FILE__, __LINE__);
                    return -1;
                }
                if (lseek(product->fd, (off_t)bitmask_offset, SEEK_SET) < 0)
                {
                    char file_offset_str[21];

                    free(bitmask);
                    coda_str64(bitmask_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
                if (read(product->fd, bitmask, bitmask_length) < 0)
                {
                    free(bitmask);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                   strerror(errno));
                    return -1;
                }
                if (lseek(product->fd, (off_t)file_offset, SEEK_SET) < 0)
                {
                    char file_offset_str[21];

                    free(bitmask);
                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
            }

            type = (coda_dynamic_type *)coda_grib_value_array_new((coda_type_array *)grib_type[grib2_values],
                                                                  num_elements, file_offset, bitsPerValue,
                                                                  decimalScaleFactor, binaryScaleFactor, referenceValue,
                                                                  bitmask);
            if (bitmask != NULL)
            {
                free(bitmask);
            }
            coda_mem_record_add_field(data, "values", type, 0);
            coda_mem_array_add_element(dataArray, (coda_dynamic_type *)data);

            if (section_size > 5)
            {
                file_offset += section_size - 5;
                if (lseek(product->fd, (off_t)(section_size - 5), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
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
        if (read(product->fd, buffer, 4) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
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

int coda_grib_open(const char *filename, int64_t file_size, coda_product **product)
{
    coda_dynamic_type *type;
    coda_grib_product *grib_product;
    long message_number;
    int open_flags;
    uint8_t buffer[28];
    int64_t message_size;
    int64_t file_offset = 0;

    if (grib_init() != 0)
    {
        return -1;
    }

    grib_product = (coda_grib_product *)malloc(sizeof(coda_grib_product));
    if (grib_product == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_grib_product), __FILE__, __LINE__);
        return -1;
    }
    grib_product->filename = NULL;
    grib_product->file_size = file_size;
    grib_product->format = coda_format_grib1;
    grib_product->root_type = NULL;
    grib_product->product_definition = NULL;
    grib_product->product_variable_size = NULL;
    grib_product->product_variable = NULL;
    grib_product->use_mmap = 0;
    grib_product->fd = -1;
    grib_product->mmap_ptr = NULL;
#ifdef WIN32
    grib_product->file_mapping = INVALID_HANDLE_VALUE;
    grib_product->file = INVALID_HANDLE_VALUE;
#endif
    grib_product->grib_version = -1;
    grib_product->record_size = 0;

    grib_product->filename = strdup(filename);
    if (grib_product->filename == NULL)
    {
        coda_grib_close((coda_product *)product);
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        return -1;
    }

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    grib_product->fd = open(filename, open_flags);
    if (grib_product->fd < 0)
    {
        coda_grib_close((coda_product *)grib_product);
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", filename, strerror(errno));
        return -1;
    }

    message_number = 0;
    while (file_offset < file_size - 1)
    {
        coda_mem_record *message;

        /* find start of Indicator Section */
        buffer[0] = '\0';
        while (file_offset < file_size - 1 && buffer[0] != 'G')
        {
            if (read(grib_product->fd, buffer, 1) < 0)
            {
                coda_grib_close((coda_product *)grib_product);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", grib_product->filename,
                               strerror(errno));
                return -1;
            }
            file_offset++;
        }
        if (file_offset >= file_size - 1)
        {
            /* there is only filler data at the end of the file, but no new message */
            break;
        }
        file_offset--;

        /* Section 0: Indicator Section */
        /* we already read the 'G' character, now read the rest of the section */
        if (read(grib_product->fd, &buffer[1], 7) < 0)
        {
            coda_grib_close((coda_product *)grib_product);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", filename, strerror(errno));
            return -1;
        }
        if (buffer[0] != 'G' || buffer[1] != 'R' || buffer[2] != 'I' || buffer[3] != 'B')
        {
            coda_grib_close((coda_product *)grib_product);
            coda_set_error(CODA_ERROR_PRODUCT, "invalid indicator for message %ld in %s", message_number, filename);
            return -1;
        }
        if (buffer[7] != 1 && buffer[7] != 2)
        {
            coda_grib_close((coda_product *)grib_product);
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "unsupported GRIB format version (%d) for message %ld for "
                           "file %s", (int)buffer[7], message_number, filename);
            return -1;
        }
        if (grib_product->grib_version < 0)
        {
            grib_product->grib_version = buffer[7];
        }
        else if (grib_product->grib_version != buffer[7])
        {
            coda_grib_close((coda_product *)grib_product);
            coda_set_error(CODA_ERROR_PRODUCT, "mixed GRIB versions within a single file not supported for file %s",
                           filename);
            return -1;
        }

        if (grib_product->grib_version == 1)
        {
            /* read product based on GRIB Edition Number 1 specification */
            if (grib_product->root_type == NULL)
            {
                grib_product->root_type =
                    (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)grib_type[grib1_root]);
            }
            message_size = ((buffer[4] * 256) + buffer[5]) * 256 + buffer[6];

            message = coda_mem_record_new((coda_type_record *)grib_type[grib1_message]);
            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib1_editionNumber], 1);
            coda_mem_record_add_field(message, "editionNumber", type, 0);
            if (read_grib1_message(grib_product, message, file_offset + 8) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)message);
                coda_grib_close((coda_product *)grib_product);
                return -1;
            }
        }
        else
        {
            /* read product based on GRIB Edition Number 2 specification */
            if (grib_product->root_type == NULL)
            {
                grib_product->format = coda_format_grib2;
                grib_product->root_type =
                    (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)grib_type[grib2_root]);
            }

            if (read(grib_product->fd, &message_size, 8) < 0)
            {
                coda_grib_close((coda_product *)grib_product);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", filename, strerror(errno));
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap8(&message_size);
#endif

            message = coda_mem_record_new((coda_type_record *)grib_type[grib2_message]);
            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_editionNumber], 2);
            coda_mem_record_add_field(message, "editionNumber", type, 0);
            type = (coda_dynamic_type *)coda_mem_integer_new((coda_type_number *)grib_type[grib2_discipline],
                                                             buffer[6]);
            coda_mem_record_add_field(message, "discipline", type, 0);

            if (read_grib2_message(grib_product, message, file_offset + 16) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)message);
                coda_grib_close((coda_product *)grib_product);
                return -1;
            }
        }

        if (coda_mem_array_add_element((coda_mem_array *)grib_product->root_type, (coda_dynamic_type *)message) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)message);
            coda_grib_close((coda_product *)grib_product);
            return -1;
        }

        file_offset += message_size;
        if (lseek(grib_product->fd, (off_t)file_offset, SEEK_SET) < 0)
        {
            char file_offset_str[21];

            coda_grib_close((coda_product *)grib_product);
            coda_str64(file_offset, file_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)", file_offset_str,
                           filename, strerror(errno));
            return -1;
        }

        message_number++;
    }


    if (coda_option_use_mmap)
    {
        /* Perform an mmap() of the file, filling the following fields:
         *   product->use_mmap = 1
         *   product->file         (windows only )
         *   product->file_mapping (windows only )
         *   product->mmap_ptr     (windows, *nix)
         */
        grib_product->use_mmap = 1;
#ifdef WIN32
        close(grib_product->fd);
        grib_product->file = CreateFile(grib_product->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, NULL);
        if (grib_product->file == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                coda_set_error(CODA_ERROR_FILE_NOT_FOUND, "could not find %s", grib_product->filename);
            }
            else
            {
                LPVOID lpMsgBuf;

                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
                {
                    /* Set error without additional information */
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", grib_product->filename);
                }
                else
                {
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", grib_product->filename,
                                   (LPCTSTR) lpMsgBuf);
                    LocalFree(lpMsgBuf);
                }
            }
            coda_grib_close((coda_product *)grib_product);
            return -1;  /* indicate failure */
        }

        /* Try to do file mapping */
        grib_product->file_mapping = CreateFileMapping(grib_product->file, NULL, PAGE_READONLY, 0,
                                                       (int32_t)grib_product->file_size, 0);
        if (grib_product->file_mapping == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", grib_product->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", grib_product->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_grib_close((coda_product *)grib_product);
            return -1;
        }

        grib_product->mmap_ptr = (uint8_t *)MapViewOfFile(grib_product->file_mapping, FILE_MAP_READ, 0, 0, 0);
        if (grib_product->mmap_ptr == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", grib_product->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", grib_product->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_grib_close((coda_product *)grib_product);
            return -1;
        }
#else
        grib_product->mmap_ptr = (uint8_t *)mmap(0, grib_product->file_size, PROT_READ, MAP_SHARED, grib_product->fd,
                                                 0);
        if (grib_product->mmap_ptr == (uint8_t *)MAP_FAILED)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", grib_product->filename,
                           strerror(errno));
            grib_product->mmap_ptr = NULL;
            close(grib_product->fd);
            coda_grib_close((coda_product *)grib_product);
            return -1;
        }

        /* close file descriptor (the file handle is not needed anymore) */
        close(grib_product->fd);
#endif
    }

    *product = (coda_product *)grib_product;
    return 0;
}

int coda_grib_close(coda_product *product)
{
    coda_grib_product *grib_product = (coda_grib_product *)product;

    if (grib_product->root_type != NULL)
    {
        coda_dynamic_type_delete(grib_product->root_type);
    }

    if (grib_product->use_mmap)
    {
#ifdef WIN32
        if (grib_product->mmap_ptr != NULL)
        {
            UnmapViewOfFile(grib_product->mmap_ptr);
        }
        if (grib_product->file_mapping != INVALID_HANDLE_VALUE)
        {
            CloseHandle(grib_product->file_mapping);
        }
        if (grib_product->file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(grib_product->file);
        }
#else
        if (grib_product->mmap_ptr != NULL)
        {
            munmap((void *)grib_product->mmap_ptr, grib_product->file_size);
        }
#endif
    }
    else
    {
        if (grib_product->fd >= 0)
        {
            close(grib_product->fd);
        }
    }

    if (grib_product->filename != NULL)
    {
        free(grib_product->filename);
    }

    free(grib_product);

    return 0;
}
