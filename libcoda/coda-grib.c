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

static struct
{
    coda_grib_basic_type *localRecordIndex;
    coda_grib_basic_type *gridRecordIndex;
    coda_grib_basic_type *table2Version;
    coda_grib_basic_type *editionNumber;
    coda_grib_basic_type *grib1_centre;
    coda_grib_basic_type *grib2_centre;
    coda_grib_basic_type *generatingProcessIdentifier;
    coda_grib_basic_type *gridDefinition;
    coda_grib_basic_type *indicatorOfParameter;
    coda_grib_basic_type *indicatorOfTypeOfLevel;
    coda_grib_basic_type *level;
    coda_grib_basic_type *yearOfCentury;
    coda_grib_basic_type *year;
    coda_grib_basic_type *month;
    coda_grib_basic_type *day;
    coda_grib_basic_type *hour;
    coda_grib_basic_type *minute;
    coda_grib_basic_type *second;
    coda_grib_basic_type *unitOfTimeRange;
    coda_grib_basic_type *P1;
    coda_grib_basic_type *P2;
    coda_grib_basic_type *timeRangeIndicator;
    coda_grib_basic_type *numberIncludedInAverage;
    coda_grib_basic_type *numberMissingFromAveragesOrAccumulations;
    coda_grib_basic_type *centuryOfReferenceTimeOfData;
    coda_grib_basic_type *grib1_subCentre;
    coda_grib_basic_type *grib2_subCentre;
    coda_grib_basic_type *decimalScaleFactor;
    coda_grib_basic_type *discipline;
    coda_grib_basic_type *masterTablesVersion;
    coda_grib_basic_type *localTablesVersion;
    coda_grib_basic_type *significanceOfReferenceTime;
    coda_grib_basic_type *productionStatusOfProcessedData;
    coda_grib_basic_type *typeOfProcessedData;
    coda_grib_basic_type *local;
    coda_grib_basic_type *numberOfVerticalCoordinateValues;
    coda_grib_basic_type *dataRepresentationType;
    coda_grib_basic_type *shapeOfTheEarth;
    coda_grib_basic_type *scaleFactorOfRadiusOfSphericalEarth;
    coda_grib_basic_type *scaledValueOfRadiusOfSphericalEarth;
    coda_grib_basic_type *scaleFactorOfEarthMajorAxis;
    coda_grib_basic_type *scaledValueOfEarthMajorAxis;
    coda_grib_basic_type *scaleFactorOfEarthMinorAxis;
    coda_grib_basic_type *scaledValueOfEarthMinorAxis;
    coda_grib_basic_type *grib1_Ni;
    coda_grib_basic_type *grib1_Nj;
    coda_grib_basic_type *grib2_Ni;
    coda_grib_basic_type *grib2_Nj;
    coda_grib_basic_type *basicAngleOfTheInitialProductionDomain;
    coda_grib_basic_type *subdivisionsOfBasicAngle;
    coda_grib_basic_type *latitudeOfFirstGridPoint;
    coda_grib_basic_type *longitudeOfFirstGridPoint;
    coda_grib_basic_type *resolutionAndComponentFlags;
    coda_grib_basic_type *latitudeOfLastGridPoint;
    coda_grib_basic_type *longitudeOfLastGridPoint;
    coda_grib_basic_type *grib1_iDirectionIncrement;
    coda_grib_basic_type *grib1_jDirectionIncrement;
    coda_grib_basic_type *grib2_iDirectionIncrement;
    coda_grib_basic_type *grib2_jDirectionIncrement;
    coda_grib_basic_type *grib1_N;
    coda_grib_basic_type *grib2_N;
    coda_grib_basic_type *scanningMode;
    coda_grib_basic_type *pv;
    coda_grib_array *pv_array;
    coda_grib_basic_type *sourceOfGridDefinition;
    coda_grib_basic_type *numberOfDataPoints;
    coda_grib_basic_type *gridDefinitionTemplateNumber;
    coda_grib_basic_type *bitsPerValue;
    coda_grib_basic_type *binaryScaleFactor;
    coda_grib_basic_type *referenceValue;
    coda_grib_array *values;
    coda_grib_record *grib1_grid;
    coda_grib_record *grib2_grid;
    coda_grib_record *grib1_data;
    coda_grib_record *grib2_data;
    coda_grib_array *grib2_local_array;
    coda_grib_array *grib2_grid_array;
    coda_grib_array *grib2_data_array;
    coda_grib_record *grib1_message;
    coda_grib_record *grib2_message;
    coda_grib_array *grib1_root;
    coda_grib_array *grib2_root;
} grib_types;

void coda_grib_release_type(coda_grib_type *T);

int grib_types_init(void)
{
    coda_grib_basic_type *basic_type;
    coda_grib_record_field *field;

    grib_types.localRecordIndex = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.localRecordIndex, coda_native_type_int32);

    grib_types.gridRecordIndex = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.gridRecordIndex, coda_native_type_uint32);

    grib_types.table2Version = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.table2Version, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.table2Version,
                              "Parameter Table Version number, currently 3 for international exchange. "
                              "(Parameter table version numbers 128-254 are reserved for local use.)");

    grib_types.editionNumber = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.editionNumber, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.editionNumber, "GRIB edition number");

    grib_types.grib1_centre = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_centre, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.grib1_centre, "Identification of center");

    grib_types.grib2_centre = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_centre, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib2_centre, "Identification of originating/generating centre");

    grib_types.generatingProcessIdentifier = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.generatingProcessIdentifier, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.generatingProcessIdentifier, "Generating process ID number");

    grib_types.gridDefinition = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.gridDefinition, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.gridDefinition, "Grid Identification");

    grib_types.indicatorOfParameter = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.indicatorOfParameter, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.indicatorOfParameter, "Indicator of parameter and units");

    grib_types.indicatorOfTypeOfLevel = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.indicatorOfTypeOfLevel, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.indicatorOfTypeOfLevel, "Indicator of type of level or layer");

    grib_types.level = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.level, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.level, "Height, pressure, etc. of the level or layer");

    grib_types.yearOfCentury = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.yearOfCentury, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.yearOfCentury, "Year of century");

    grib_types.year = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.year, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.year, "Year");

    grib_types.month = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.month, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.month, "Month of year");

    grib_types.day = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.day, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.day, "Day of month");

    grib_types.hour = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.hour, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.hour, "Hour of day");

    grib_types.minute = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.minute, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.minute, "Minute of hour");

    grib_types.second = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.second, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.second, "Second of minute");

    grib_types.unitOfTimeRange = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.unitOfTimeRange, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.unitOfTimeRange, "Forecast time unit");

    grib_types.P1 = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.P1, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.P1, "Period of time (Number of time units)");

    grib_types.P2 = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.P2, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.P2, "Period of time (Number of time units)");

    grib_types.timeRangeIndicator = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.timeRangeIndicator, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.timeRangeIndicator, "Time range indicator");

    grib_types.numberIncludedInAverage = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.numberIncludedInAverage, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.numberIncludedInAverage, "Number included in average, when "
                              "timeRangeIndicator indicates an average or accumulation; otherwise set to zero.");

    grib_types.numberMissingFromAveragesOrAccumulations = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.numberMissingFromAveragesOrAccumulations, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.numberMissingFromAveragesOrAccumulations, "Number Missing from "
                              "averages or accumulations.");

    grib_types.centuryOfReferenceTimeOfData = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.centuryOfReferenceTimeOfData, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.centuryOfReferenceTimeOfData, "Century of Initial (Reference) "
                              "time (=20 until Jan. 1, 2001)");

    grib_types.grib1_subCentre = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_subCentre, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.grib1_subCentre,
                              "Identification of sub-center (allocated by the originating center; See Table C)");

    grib_types.grib2_subCentre = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_subCentre, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib2_subCentre, "Identification of originating/generating "
                              "sub-centre (allocated by originating/generating centre)");

    grib_types.decimalScaleFactor = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.decimalScaleFactor, coda_native_type_int16);
    coda_type_set_description((coda_type *)grib_types.decimalScaleFactor, "The decimal scale factor D");

    grib_types.discipline = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.discipline, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.discipline, "GRIB Master Table Number");

    grib_types.masterTablesVersion = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.masterTablesVersion, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.masterTablesVersion, "GRIB Master Tables Version Number");

    grib_types.localTablesVersion = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.localTablesVersion, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.localTablesVersion, "GRIB Local Tables Version Number");

    grib_types.significanceOfReferenceTime = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.significanceOfReferenceTime, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.significanceOfReferenceTime, "Significance of Reference Time");

    grib_types.productionStatusOfProcessedData = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.productionStatusOfProcessedData, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.productionStatusOfProcessedData,
                              "Production status of processed data in this GRIB message");

    grib_types.typeOfProcessedData = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.typeOfProcessedData, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.typeOfProcessedData,
                              "Type of processed data in this GRIB message");

    grib_types.local = coda_grib_basic_type_new(coda_raw_class);
    coda_type_set_description((coda_type *)grib_types.local, "Reserved for originating center use");

    grib_types.numberOfVerticalCoordinateValues = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.numberOfVerticalCoordinateValues, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.numberOfVerticalCoordinateValues,
                              "NV, the number of vertical coordinate parameter");

    grib_types.dataRepresentationType = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.dataRepresentationType, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.dataRepresentationType, "Data representation type");

    grib_types.shapeOfTheEarth = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.shapeOfTheEarth, coda_native_type_uint8);

    grib_types.scaleFactorOfRadiusOfSphericalEarth = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaleFactorOfRadiusOfSphericalEarth, coda_native_type_uint8);

    grib_types.scaledValueOfRadiusOfSphericalEarth = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaledValueOfRadiusOfSphericalEarth, coda_native_type_uint32);

    grib_types.scaleFactorOfEarthMajorAxis = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaleFactorOfEarthMajorAxis, coda_native_type_uint8);

    grib_types.scaledValueOfEarthMajorAxis = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaledValueOfEarthMajorAxis, coda_native_type_uint32);

    grib_types.scaleFactorOfEarthMinorAxis = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaleFactorOfEarthMinorAxis, coda_native_type_uint8);

    grib_types.scaledValueOfEarthMinorAxis = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scaledValueOfEarthMinorAxis, coda_native_type_uint32);

    grib_types.grib1_Ni = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_Ni, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib1_Ni, "No. of points along a latitude circle");

    grib_types.grib1_Nj = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_Nj, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib1_Nj, "No. of points along a longitude meridian");

    grib_types.grib2_Ni = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_Ni, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.grib2_Ni, "No. of points along a latitude circle");

    grib_types.grib2_Nj = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_Nj, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.grib2_Nj, "No. of points along a latitude circle");

    grib_types.basicAngleOfTheInitialProductionDomain = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.basicAngleOfTheInitialProductionDomain, coda_native_type_uint32);

    grib_types.subdivisionsOfBasicAngle = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.subdivisionsOfBasicAngle, coda_native_type_uint32);

    grib_types.latitudeOfFirstGridPoint = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.latitudeOfFirstGridPoint, coda_native_type_int32);
    coda_type_set_description((coda_type *)grib_types.latitudeOfFirstGridPoint,
                              "La1 - latitude of first grid point, units: millidegrees (degrees x 1000), "
                              "values limited to range 0 - 90,000");

    grib_types.longitudeOfFirstGridPoint = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.longitudeOfFirstGridPoint, coda_native_type_int32);
    coda_type_set_description((coda_type *)grib_types.longitudeOfFirstGridPoint,
                              "Lo1 - longitude of first grid point, units: millidegrees (degrees x 1000), "
                              "values limited to range 0 - 360,000");

    grib_types.resolutionAndComponentFlags = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.resolutionAndComponentFlags, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.resolutionAndComponentFlags, "Resolution and component flags");

    grib_types.latitudeOfLastGridPoint = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.latitudeOfLastGridPoint, coda_native_type_int32);
    coda_type_set_description((coda_type *)grib_types.latitudeOfLastGridPoint, "La2 - Latitude of last grid "
                              "point (same units and value range as latitudeOfFirstGridPoint)");

    grib_types.longitudeOfLastGridPoint = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.longitudeOfLastGridPoint, coda_native_type_int32);
    coda_type_set_description((coda_type *)grib_types.longitudeOfLastGridPoint, "Lo2 - Longitude of last grid "
                              "point (same units and value range as longitudeOfFirstGridPoint)");

    grib_types.grib1_iDirectionIncrement = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_iDirectionIncrement, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib1_iDirectionIncrement, "Di - Longitudinal Direction "
                              "Increment (same units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_types.grib1_jDirectionIncrement = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_jDirectionIncrement, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib1_jDirectionIncrement, "Dj - Latitudinal Direction Increment "
                              "(same units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_types.grib2_iDirectionIncrement = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_iDirectionIncrement, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.grib2_iDirectionIncrement, "Di - Longitudinal Direction "
                              "Increment (same units as longitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_types.grib2_jDirectionIncrement = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_jDirectionIncrement, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.grib2_jDirectionIncrement, "Dj - Latitudinal Direction Increment "
                              "(same units as latitudeOfFirstGridPoint) (if not given, all bits set = 1)");

    grib_types.grib1_N = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib1_N, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.grib1_N, "N - number of latitude circles between a pole and the "
                              "equator, Mandatory if Gaussian Grid specified");

    grib_types.grib2_N = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.grib2_N, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.grib2_N, "N - number of latitude circles between a pole and the "
                              "equator, Mandatory if Gaussian Grid specified");

    grib_types.scanningMode = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.scanningMode, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.scanningMode, "Scanning mode flags");

    grib_types.pv = coda_grib_basic_type_new(coda_real_class);
    coda_grib_basic_type_set_read_type(grib_types.pv, coda_native_type_float);
    grib_types.pv_array = coda_grib_array_new();
    coda_type_set_description((coda_type *)grib_types.pv_array, "List of vertical coordinate parameters");
    coda_grib_array_set_base_type(grib_types.pv_array, (coda_grib_type *)grib_types.pv);
    coda_grib_array_add_variable_dimension(grib_types.pv_array, NULL);

    grib_types.sourceOfGridDefinition = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.sourceOfGridDefinition, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.sourceOfGridDefinition, "Source of grid definition");

    grib_types.numberOfDataPoints = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.numberOfDataPoints, coda_native_type_uint32);
    coda_type_set_description((coda_type *)grib_types.numberOfDataPoints, "Number of data points");

    grib_types.gridDefinitionTemplateNumber = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.gridDefinitionTemplateNumber, coda_native_type_uint16);
    coda_type_set_description((coda_type *)grib_types.gridDefinitionTemplateNumber, "Grid Definition Template Number");

    grib_types.bitsPerValue = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.bitsPerValue, coda_native_type_uint8);
    coda_type_set_description((coda_type *)grib_types.bitsPerValue,
                              "Number of bits into which a datum point is packed.");

    grib_types.binaryScaleFactor = coda_grib_basic_type_new(coda_integer_class);
    coda_grib_basic_type_set_read_type(grib_types.binaryScaleFactor, coda_native_type_int16);
    coda_type_set_description((coda_type *)grib_types.binaryScaleFactor, "The binary scale factor (E).");

    grib_types.referenceValue = coda_grib_basic_type_new(coda_real_class);
    coda_grib_basic_type_set_read_type(grib_types.referenceValue, coda_native_type_float);
    coda_type_set_description((coda_type *)grib_types.referenceValue, "Reference value (minimum value). "
                              "This is the overall or 'global' minimum that has been subtracted from all the values.");

    grib_types.values = coda_grib_array_new();
    basic_type = coda_grib_basic_type_new(coda_real_class);
    coda_grib_basic_type_set_read_type(basic_type, coda_native_type_float);
    coda_grib_array_set_base_type(grib_types.values, (coda_grib_type *)basic_type);
    coda_release_type((coda_type *)basic_type);
    coda_grib_array_add_variable_dimension(grib_types.values, NULL);

    grib_types.grib1_grid = coda_grib_record_new();
    field = coda_grib_record_field_new("numberOfVerticalCoordinateValues");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.numberOfVerticalCoordinateValues);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("dataRepresentationType");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.dataRepresentationType);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("Ni");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_Ni);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("Nj");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_Nj);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("latitudeOfFirstGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.latitudeOfFirstGridPoint);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("longitudeOfFirstGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.longitudeOfFirstGridPoint);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("resolutionAndComponentFlags");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.resolutionAndComponentFlags);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("latitudeOfLastGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.latitudeOfLastGridPoint);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("longitudeOfLastGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.longitudeOfLastGridPoint);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("iDirectionIncrement");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_iDirectionIncrement);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("jDirectionIncrement");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_jDirectionIncrement);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("N");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_N);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("scanningMode");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scanningMode);
    coda_grib_record_add_field(grib_types.grib1_grid, field);
    field = coda_grib_record_field_new("pv");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.pv_array);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib1_grid, field);

    grib_types.grib2_grid = coda_grib_record_new();
    field = coda_grib_record_field_new("localRecordIndex");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.localRecordIndex);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("sourceOfGridDefinition");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.sourceOfGridDefinition);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("numberOfDataPoints");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.numberOfDataPoints);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("gridDefinitionTemplateNumber");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.gridDefinitionTemplateNumber);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("shapeOfTheEarth");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.shapeOfTheEarth);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaleFactorOfRadiusOfSphericalEarth");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaleFactorOfRadiusOfSphericalEarth);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaledValueOfRadiusOfSphericalEarth");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaledValueOfRadiusOfSphericalEarth);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaleFactorOfEarthMajorAxis");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaleFactorOfEarthMajorAxis);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaledValueOfEarthMajorAxis");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaledValueOfEarthMajorAxis);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaleFactorOfEarthMinorAxis");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaleFactorOfEarthMinorAxis);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scaledValueOfEarthMinorAxis");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scaledValueOfEarthMinorAxis);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("Ni");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_Ni);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("Nj");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_Nj);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("basicAngleOfTheInitialProductionDomain");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.basicAngleOfTheInitialProductionDomain);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("subdivisionsOfBasicAngle");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.subdivisionsOfBasicAngle);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("latitudeOfFirstGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.latitudeOfFirstGridPoint);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("longitudeOfFirstGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.longitudeOfFirstGridPoint);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("resolutionAndComponentFlags");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.resolutionAndComponentFlags);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("latitudeOfLastGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.latitudeOfLastGridPoint);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("longitudeOfLastGridPoint");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.longitudeOfLastGridPoint);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("iDirectionIncrement");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_iDirectionIncrement);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("jDirectionIncrement");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_jDirectionIncrement);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("N");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_N);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib2_grid, field);
    field = coda_grib_record_field_new("scanningMode");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.scanningMode);
    coda_grib_record_add_field(grib_types.grib2_grid, field);

    grib_types.grib1_data = coda_grib_record_new();
    field = coda_grib_record_field_new("bitsPerValue");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.bitsPerValue);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib1_data, field);
    field = coda_grib_record_field_new("binaryScaleFactor");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.binaryScaleFactor);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib1_data, field);
    field = coda_grib_record_field_new("referenceValue");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.referenceValue);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib1_data, field);
    field = coda_grib_record_field_new("values");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.values);
    coda_grib_record_add_field(grib_types.grib1_data, field);

    grib_types.grib2_data = coda_grib_record_new();
    field = coda_grib_record_field_new("gridRecordIndex");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.gridRecordIndex);
    coda_grib_record_add_field(grib_types.grib2_data, field);
    field = coda_grib_record_field_new("bitsPerValue");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.bitsPerValue);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib2_data, field);
    field = coda_grib_record_field_new("decimalScaleFactor");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.decimalScaleFactor);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib2_data, field);
    field = coda_grib_record_field_new("binaryScaleFactor");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.binaryScaleFactor);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib2_data, field);
    field = coda_grib_record_field_new("referenceValue");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.referenceValue);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib2_data, field);
    field = coda_grib_record_field_new("values");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.values);
    coda_grib_record_add_field(grib_types.grib2_data, field);

    grib_types.grib2_local_array = coda_grib_array_new();
    coda_grib_array_add_variable_dimension(grib_types.grib2_local_array, NULL);
    coda_grib_array_set_base_type(grib_types.grib2_local_array, (coda_grib_type *)grib_types.local);

    grib_types.grib2_grid_array = coda_grib_array_new();
    coda_grib_array_add_variable_dimension(grib_types.grib2_grid_array, NULL);
    coda_grib_array_set_base_type(grib_types.grib2_grid_array, (coda_grib_type *)grib_types.grib2_grid);

    grib_types.grib2_data_array = coda_grib_array_new();
    coda_grib_array_add_variable_dimension(grib_types.grib2_data_array, NULL);
    coda_grib_array_set_base_type(grib_types.grib2_data_array, (coda_grib_type *)grib_types.grib2_data);

    grib_types.grib1_message = coda_grib_record_new();
    field = coda_grib_record_field_new("editionNumber");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.editionNumber);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("table2Version");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.table2Version);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("centre");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_centre);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("generatingProcessIdentifier");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.generatingProcessIdentifier);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("gridDefinition");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.gridDefinition);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("indicatorOfParameter");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.indicatorOfParameter);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("indicatorOfTypeOfLevel");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.indicatorOfTypeOfLevel);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("level");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.level);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("yearOfCentury");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.yearOfCentury);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("month");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.month);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("day");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.day);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("hour");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.hour);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("minute");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.minute);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("unitOfTimeRange");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.unitOfTimeRange);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("P1");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.P1);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("P2");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.P2);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("timeRangeIndicator");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.timeRangeIndicator);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("numberIncludedInAverage");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.numberIncludedInAverage);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("numberMissingFromAveragesOrAccumulations");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.numberMissingFromAveragesOrAccumulations);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("centuryOfReferenceTimeOfData");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.centuryOfReferenceTimeOfData);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("subCentre");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_subCentre);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("decimalScaleFactor");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.decimalScaleFactor);
    coda_grib_record_field_set_hidden(field);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("local");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.local);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("grid");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_grid);
    coda_grib_record_field_set_optional(field);
    coda_grib_record_add_field(grib_types.grib1_message, field);
    field = coda_grib_record_field_new("data");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib1_data);
    coda_grib_record_add_field(grib_types.grib1_message, field);

    grib_types.grib2_message = coda_grib_record_new();
    field = coda_grib_record_field_new("editionNumber");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.editionNumber);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("discipline");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.discipline);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("centre");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_centre);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("subCentre");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_subCentre);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("masterTablesVersion");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.masterTablesVersion);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("localTablesVersion");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.localTablesVersion);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("significanceOfReferenceTime");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.significanceOfReferenceTime);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("year");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.year);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("month");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.month);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("day");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.day);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("hour");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.hour);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("minute");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.minute);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("second");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.second);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("productionStatusOfProcessedData");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.productionStatusOfProcessedData);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("typeOfProcessedData");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.typeOfProcessedData);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("local");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_local_array);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("grid");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_grid_array);
    coda_grib_record_add_field(grib_types.grib2_message, field);
    field = coda_grib_record_field_new("data");
    coda_grib_record_field_set_type(field, (coda_grib_type *)grib_types.grib2_data_array);
    coda_grib_record_add_field(grib_types.grib2_message, field);

    grib_types.grib1_root = coda_grib_array_new();
    coda_grib_array_add_variable_dimension(grib_types.grib1_root, NULL);
    coda_grib_array_set_base_type(grib_types.grib1_root, (coda_grib_type *)grib_types.grib1_message);

    grib_types.grib2_root = coda_grib_array_new();
    coda_grib_array_add_variable_dimension(grib_types.grib2_root, NULL);
    coda_grib_array_set_base_type(grib_types.grib2_root, (coda_grib_type *)grib_types.grib2_message);

    return 0;
}

void grib_types_done(void)
{
    if (grib_types.localRecordIndex != NULL)
    {
        coda_release_type((coda_type *)grib_types.localRecordIndex);
    }
    if (grib_types.gridRecordIndex != NULL)
    {
        coda_release_type((coda_type *)grib_types.gridRecordIndex);
    }
    if (grib_types.table2Version != NULL)
    {
        coda_release_type((coda_type *)grib_types.table2Version);
    }
    if (grib_types.editionNumber != NULL)
    {
        coda_release_type((coda_type *)grib_types.editionNumber);
    }
    if (grib_types.grib1_centre != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_centre);
    }
    if (grib_types.grib2_centre != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_centre);
    }
    if (grib_types.generatingProcessIdentifier != NULL)
    {
        coda_release_type((coda_type *)grib_types.generatingProcessIdentifier);
    }
    if (grib_types.gridDefinition != NULL)
    {
        coda_release_type((coda_type *)grib_types.gridDefinition);
    }
    if (grib_types.indicatorOfParameter != NULL)
    {
        coda_release_type((coda_type *)grib_types.indicatorOfParameter);
    }
    if (grib_types.indicatorOfTypeOfLevel != NULL)
    {
        coda_release_type((coda_type *)grib_types.indicatorOfTypeOfLevel);
    }
    if (grib_types.level != NULL)
    {
        coda_release_type((coda_type *)grib_types.level);
    }
    if (grib_types.yearOfCentury != NULL)
    {
        coda_release_type((coda_type *)grib_types.yearOfCentury);
    }
    if (grib_types.year != NULL)
    {
        coda_release_type((coda_type *)grib_types.year);
    }
    if (grib_types.month != NULL)
    {
        coda_release_type((coda_type *)grib_types.month);
    }
    if (grib_types.day != NULL)
    {
        coda_release_type((coda_type *)grib_types.day);
    }
    if (grib_types.hour != NULL)
    {
        coda_release_type((coda_type *)grib_types.hour);
    }
    if (grib_types.minute != NULL)
    {
        coda_release_type((coda_type *)grib_types.minute);
    }
    if (grib_types.second != NULL)
    {
        coda_release_type((coda_type *)grib_types.second);
    }
    if (grib_types.unitOfTimeRange != NULL)
    {
        coda_release_type((coda_type *)grib_types.unitOfTimeRange);
    }
    if (grib_types.P1 != NULL)
    {
        coda_release_type((coda_type *)grib_types.P1);
    }
    if (grib_types.P2 != NULL)
    {
        coda_release_type((coda_type *)grib_types.P2);
    }
    if (grib_types.timeRangeIndicator != NULL)
    {
        coda_release_type((coda_type *)grib_types.timeRangeIndicator);
    }
    if (grib_types.numberIncludedInAverage != NULL)
    {
        coda_release_type((coda_type *)grib_types.numberIncludedInAverage);
    }
    if (grib_types.numberMissingFromAveragesOrAccumulations != NULL)
    {
        coda_release_type((coda_type *)grib_types.numberMissingFromAveragesOrAccumulations);
    }
    if (grib_types.centuryOfReferenceTimeOfData != NULL)
    {
        coda_release_type((coda_type *)grib_types.centuryOfReferenceTimeOfData);
    }
    if (grib_types.grib1_subCentre != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_subCentre);
    }
    if (grib_types.grib2_subCentre != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_subCentre);
    }
    if (grib_types.decimalScaleFactor != NULL)
    {
        coda_release_type((coda_type *)grib_types.decimalScaleFactor);
    }
    if (grib_types.discipline != NULL)
    {
        coda_release_type((coda_type *)grib_types.discipline);
    }
    if (grib_types.masterTablesVersion != NULL)
    {
        coda_release_type((coda_type *)grib_types.masterTablesVersion);
    }
    if (grib_types.localTablesVersion != NULL)
    {
        coda_release_type((coda_type *)grib_types.localTablesVersion);
    }
    if (grib_types.significanceOfReferenceTime != NULL)
    {
        coda_release_type((coda_type *)grib_types.significanceOfReferenceTime);
    }
    if (grib_types.productionStatusOfProcessedData != NULL)
    {
        coda_release_type((coda_type *)grib_types.productionStatusOfProcessedData);
    }
    if (grib_types.typeOfProcessedData != NULL)
    {
        coda_release_type((coda_type *)grib_types.typeOfProcessedData);
    }
    if (grib_types.local != NULL)
    {
        coda_release_type((coda_type *)grib_types.local);
    }
    if (grib_types.numberOfVerticalCoordinateValues != NULL)
    {
        coda_release_type((coda_type *)grib_types.numberOfVerticalCoordinateValues);
    }
    if (grib_types.dataRepresentationType != NULL)
    {
        coda_release_type((coda_type *)grib_types.dataRepresentationType);
    }
    if (grib_types.shapeOfTheEarth != NULL)
    {
        coda_release_type((coda_type *)grib_types.shapeOfTheEarth);
    }
    if (grib_types.scaleFactorOfRadiusOfSphericalEarth != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaleFactorOfRadiusOfSphericalEarth);
    }
    if (grib_types.scaledValueOfRadiusOfSphericalEarth != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaledValueOfRadiusOfSphericalEarth);
    }
    if (grib_types.scaleFactorOfEarthMajorAxis != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaleFactorOfEarthMajorAxis);
    }
    if (grib_types.scaledValueOfEarthMajorAxis != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaledValueOfEarthMajorAxis);
    }
    if (grib_types.scaleFactorOfEarthMinorAxis != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaleFactorOfEarthMinorAxis);
    }
    if (grib_types.scaledValueOfEarthMinorAxis != NULL)
    {
        coda_release_type((coda_type *)grib_types.scaledValueOfEarthMinorAxis);
    }
    if (grib_types.grib1_Ni != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_Ni);
    }
    if (grib_types.grib1_Nj != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_Nj);
    }
    if (grib_types.grib2_Ni != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_Ni);
    }
    if (grib_types.grib2_Nj != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_Nj);
    }
    if (grib_types.basicAngleOfTheInitialProductionDomain != NULL)
    {
        coda_release_type((coda_type *)grib_types.basicAngleOfTheInitialProductionDomain);
    }
    if (grib_types.subdivisionsOfBasicAngle != NULL)
    {
        coda_release_type((coda_type *)grib_types.subdivisionsOfBasicAngle);
    }
    if (grib_types.latitudeOfFirstGridPoint != NULL)
    {
        coda_release_type((coda_type *)grib_types.latitudeOfFirstGridPoint);
    }
    if (grib_types.longitudeOfFirstGridPoint != NULL)
    {
        coda_release_type((coda_type *)grib_types.longitudeOfFirstGridPoint);
    }
    if (grib_types.resolutionAndComponentFlags != NULL)
    {
        coda_release_type((coda_type *)grib_types.resolutionAndComponentFlags);
    }
    if (grib_types.latitudeOfLastGridPoint != NULL)
    {
        coda_release_type((coda_type *)grib_types.latitudeOfLastGridPoint);
    }
    if (grib_types.longitudeOfLastGridPoint != NULL)
    {
        coda_release_type((coda_type *)grib_types.longitudeOfLastGridPoint);
    }
    if (grib_types.grib1_iDirectionIncrement != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_iDirectionIncrement);
    }
    if (grib_types.grib1_jDirectionIncrement != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_jDirectionIncrement);
    }
    if (grib_types.grib2_iDirectionIncrement != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_iDirectionIncrement);
    }
    if (grib_types.grib2_jDirectionIncrement != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_jDirectionIncrement);
    }
    if (grib_types.grib1_N != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_N);
    }
    if (grib_types.grib2_N != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_N);
    }
    if (grib_types.scanningMode != NULL)
    {
        coda_release_type((coda_type *)grib_types.scanningMode);
    }
    if (grib_types.pv != NULL)
    {
        coda_release_type((coda_type *)grib_types.pv);
    }
    if (grib_types.pv_array != NULL)
    {
        coda_release_type((coda_type *)grib_types.pv_array);
    }
    if (grib_types.sourceOfGridDefinition != NULL)
    {
        coda_release_type((coda_type *)grib_types.sourceOfGridDefinition);
    }
    if (grib_types.numberOfDataPoints != NULL)
    {
        coda_release_type((coda_type *)grib_types.numberOfDataPoints);
    }
    if (grib_types.gridDefinitionTemplateNumber != NULL)
    {
        coda_release_type((coda_type *)grib_types.gridDefinitionTemplateNumber);
    }
    if (grib_types.bitsPerValue != NULL)
    {
        coda_release_type((coda_type *)grib_types.bitsPerValue);
    }
    if (grib_types.binaryScaleFactor != NULL)
    {
        coda_release_type((coda_type *)grib_types.binaryScaleFactor);
    }
    if (grib_types.referenceValue != NULL)
    {
        coda_release_type((coda_type *)grib_types.referenceValue);
    }
    if (grib_types.values != NULL)
    {
        coda_release_type((coda_type *)grib_types.values);
    }
    if (grib_types.grib1_grid != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_grid);
    }
    if (grib_types.grib2_grid != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_grid);
    }
    if (grib_types.grib1_data != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_data);
    }
    if (grib_types.grib2_data != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_data);
    }
    if (grib_types.grib2_local_array != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_local_array);
    }
    if (grib_types.grib2_grid_array != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_grid_array);
    }
    if (grib_types.grib2_data_array != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_data_array);
    }
    if (grib_types.grib1_message != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_message);
    }
    if (grib_types.grib2_message != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_message);
    }
    if (grib_types.grib1_root != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib1_root);
    }
    if (grib_types.grib2_root != NULL)
    {
        coda_release_type((coda_type *)grib_types.grib2_root);
    }
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

static coda_grib_dynamic_record *empty_attributes_singleton = NULL;

coda_grib_dynamic_record *coda_grib_empty_attribute_record()
{
    if (empty_attributes_singleton == NULL)
    {
        empty_attributes_singleton = coda_grib_empty_dynamic_record();
    }

    return empty_attributes_singleton;
}

int coda_grib_init(void)
{
    if (grib_types_init() != 0)
    {
        return -1;
    }

    return 0;
}

void coda_grib_done(void)
{
    grib_types_done();
    if (empty_attributes_singleton != NULL)
    {
        coda_grib_release_type((coda_grib_type *)empty_attributes_singleton);
        empty_attributes_singleton = NULL;
    }
}

static int read_grib1_message(coda_grib_product *product, coda_grib_dynamic_record *message, int64_t file_offset)
{
    coda_grib_dynamic_type *type;
    coda_grib_dynamic_record *bds;
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

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.table2Version, buffer[3]);
    coda_grib_dynamic_record_set_field(message, "table2Version", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_centre, buffer[4]);
    coda_grib_dynamic_record_set_field(message, "centre", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.generatingProcessIdentifier, buffer[5]);
    coda_grib_dynamic_record_set_field(message, "generatingProcessIdentifier", type);
    coda_grib_release_dynamic_type(type);

    gridDefinition = buffer[6];
    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.gridDefinition, gridDefinition);
    coda_grib_dynamic_record_set_field(message, "gridDefinition", type);
    coda_grib_release_dynamic_type(type);

    has_gds = buffer[7] & 0x80 ? 1 : 0;
    has_bms = buffer[7] & 0x40 ? 1 : 0;

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.indicatorOfParameter, buffer[8]);
    coda_grib_dynamic_record_set_field(message, "indicatorOfParameter", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.indicatorOfTypeOfLevel, buffer[9]);
    coda_grib_dynamic_record_set_field(message, "indicatorOfTypeOfLevel", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.level, buffer[10] * 256 + buffer[11]);
    coda_grib_dynamic_record_set_field(message, "level", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.yearOfCentury, buffer[12]);
    coda_grib_dynamic_record_set_field(message, "yearOfCentury", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.month, buffer[13]);
    coda_grib_dynamic_record_set_field(message, "month", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.day, buffer[14]);
    coda_grib_dynamic_record_set_field(message, "day", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.hour, buffer[15]);
    coda_grib_dynamic_record_set_field(message, "hour", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.minute, buffer[16]);
    coda_grib_dynamic_record_set_field(message, "minute", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.unitOfTimeRange, buffer[17]);
    coda_grib_dynamic_record_set_field(message, "unitOfTimeRange", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.P1, buffer[18]);
    coda_grib_dynamic_record_set_field(message, "P1", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.P2, buffer[19]);
    coda_grib_dynamic_record_set_field(message, "P2", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.timeRangeIndicator, buffer[20]);
    coda_grib_dynamic_record_set_field(message, "timeRangeIndicator", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.numberIncludedInAverage,
                                                                   buffer[21] * 256 + buffer[22]);
    coda_grib_dynamic_record_set_field(message, "numberIncludedInAverage", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.numberMissingFromAveragesOrAccumulations,
                                                                   buffer[23]);
    coda_grib_dynamic_record_set_field(message, "numberMissingFromAveragesOrAccumulations", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.centuryOfReferenceTimeOfData, buffer[24]);
    coda_grib_dynamic_record_set_field(message, "centuryOfReferenceTimeOfData", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_subCentre, buffer[25]);
    coda_grib_dynamic_record_set_field(message, "subCentre", type);
    coda_grib_release_dynamic_type(type);

    decimalScaleFactor = (buffer[26] & 0x80 ? -1 : 1) * ((buffer[26] & 0x7F) * 256 + buffer[27]);
    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.decimalScaleFactor, decimalScaleFactor);
    coda_grib_dynamic_record_set_field(message, "decimalScaleFactor", type);
    coda_grib_release_dynamic_type(type);

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
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_raw_new(grib_types.local, section_size - 40, raw_data);
            coda_grib_dynamic_record_set_field(message, "local", type);
            coda_grib_release_dynamic_type(type);
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
        coda_grib_dynamic_record *gds;

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
            gds = coda_grib_dynamic_record_new(grib_types.grib1_grid);

            NV = buffer[3];
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.numberOfVerticalCoordinateValues,
                                                                           NV);
            coda_grib_dynamic_record_set_field(gds, "numberOfVerticalCoordinateValues", type);
            coda_grib_release_dynamic_type(type);

            PVL = buffer[4];

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.dataRepresentationType,
                                                                           buffer[5]);
            coda_grib_dynamic_record_set_field(gds, "dataRepresentationType", type);
            coda_grib_release_dynamic_type(type);

            if (read(product->fd, buffer, 26) < 0)
            {
                coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
                return -1;
            }

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_Ni,
                                                                           buffer[0] * 256 + buffer[1]);
            coda_grib_dynamic_record_set_field(gds, "Ni", type);
            coda_grib_release_dynamic_type(type);
            num_elements = buffer[0] * 256 + buffer[1];

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_Nj,
                                                                           buffer[2] * 256 + buffer[3]);
            coda_grib_dynamic_record_set_field(gds, "Nj", type);
            coda_grib_release_dynamic_type(type);
            num_elements *= buffer[2] * 256 + buffer[3];

            intvalue = (buffer[4] & 0x80 ? -1 : 1) * (((buffer[4] & 0x7F) * 256 + buffer[5]) * 256 + buffer[6]);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.latitudeOfFirstGridPoint,
                                                                           intvalue);
            coda_grib_dynamic_record_set_field(gds, "latitudeOfFirstGridPoint", type);
            coda_grib_release_dynamic_type(type);

            intvalue = (buffer[7] & 0x80 ? -1 : 1) * (((buffer[7] & 0x7F) * 256 + buffer[8]) * 256 + buffer[9]);
            type =
                (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.longitudeOfFirstGridPoint, intvalue);
            coda_grib_dynamic_record_set_field(gds, "longitudeOfFirstGridPoint", type);
            coda_grib_release_dynamic_type(type);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.resolutionAndComponentFlags,
                                                                           buffer[10]);
            coda_grib_dynamic_record_set_field(gds, "resolutionAndComponentFlags", type);
            coda_grib_release_dynamic_type(type);

            intvalue = (buffer[11] & 0x80 ? -1 : 1) * (((buffer[11] & 0x7F) * 256 + buffer[12]) * 256 + buffer[13]);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.latitudeOfLastGridPoint,
                                                                           intvalue);
            coda_grib_dynamic_record_set_field(gds, "latitudeOfLastGridPoint", type);
            coda_grib_release_dynamic_type(type);

            intvalue = (buffer[14] & 0x80 ? -1 : 1) * (((buffer[14] & 0x7F) * 256 + buffer[15]) * 256 + buffer[16]);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.longitudeOfLastGridPoint,
                                                                           intvalue);
            coda_grib_dynamic_record_set_field(gds, "longitudeOfLastGridPoint", type);
            coda_grib_release_dynamic_type(type);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_iDirectionIncrement,
                                                                           buffer[17] * 256 + buffer[18]);
            coda_grib_dynamic_record_set_field(gds, "iDirectionIncrement", type);
            coda_grib_release_dynamic_type(type);

            if (is_gaussian)
            {
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_N,
                                                                               buffer[19] * 256 + buffer[20]);
                coda_grib_dynamic_record_set_field(gds, "N", type);
            }
            else
            {
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib1_jDirectionIncrement,
                                                                               buffer[19] * 256 + buffer[20]);
                coda_grib_dynamic_record_set_field(gds, "jDirectionIncrement", type);
            }
            coda_grib_release_dynamic_type(type);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scanningMode, buffer[21]);
            coda_grib_dynamic_record_set_field(gds, "scanningMode", type);
            coda_grib_release_dynamic_type(type);

            file_offset += 26;

            if (PVL != 255)
            {
                PVL--;  /* make offset zero based */
                file_offset += PVL - 32;
                if (lseek(product->fd, (off_t)(PVL - 32), SEEK_CUR) < 0)
                {
                    char file_offset_str[21];

                    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
                if (NV > 0)
                {
                    coda_grib_dynamic_array *pvArray;
                    int i;

                    pvArray = coda_grib_dynamic_array_new(grib_types.pv_array);
                    for (i = 0; i < NV; i++)
                    {
                        if (read(product->fd, buffer, 4) < 0)
                        {
                            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)pvArray);
                            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
                            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                           strerror(errno));
                            return -1;
                        }
                        type = (coda_grib_dynamic_type *)coda_grib_dynamic_real_new(grib_types.pv,
                                                                                    ibmfloat_to_iee754(buffer));
                        coda_grib_dynamic_array_add_element(pvArray, type);
                        coda_grib_release_dynamic_type(type);

                        file_offset += 4;
                    }
                    coda_grib_dynamic_record_set_field(gds, "pv", (coda_grib_dynamic_type *)pvArray);
                    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)pvArray);
                }
                if (section_size > PVL + NV * 4)
                {
                    file_offset += section_size - (PVL + NV * 4);
                    if (lseek(product->fd, (off_t)(section_size - (PVL + 4 * NV)), SEEK_CUR) < 0)
                    {
                        char file_offset_str[21];

                        coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
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

                    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
                    coda_str64(file_offset, file_offset_str);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                   file_offset_str, product->filename, strerror(errno));
                    return -1;
                }
            }

            coda_grib_dynamic_record_set_field(message, "grid", (coda_grib_dynamic_type *)gds);
            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gds);
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
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
            return -1;
        }

        section_size = ((buffer[0] * 256) + buffer[1]) * 256 + buffer[2];
        if ((buffer[4] * 256) + buffer[5] != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "Bit Map Section with predefined bit map not supported");
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
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename, strerror(errno));
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
    bds = coda_grib_dynamic_record_new(grib_types.grib1_data);
    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.bitsPerValue, bitsPerValue);
    coda_grib_dynamic_record_set_field(bds, "bitsPerValue", type);
    coda_grib_release_dynamic_type(type);
    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.binaryScaleFactor, binaryScaleFactor);
    coda_grib_dynamic_record_set_field(bds, "binaryScaleFactor", type);
    coda_grib_release_dynamic_type(type);
    type = (coda_grib_dynamic_type *)coda_grib_dynamic_real_new(grib_types.referenceValue, referenceValue);
    coda_grib_dynamic_record_set_field(bds, "referenceValue", type);
    coda_grib_release_dynamic_type(type);

    file_offset += 11;

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_value_array_new(grib_types.values, num_elements,
                                                                       file_offset, bitsPerValue, decimalScaleFactor,
                                                                       binaryScaleFactor, referenceValue, bitmask);
    coda_grib_dynamic_record_set_field(bds, "values", type);
    coda_grib_release_dynamic_type(type);

    coda_grib_dynamic_record_set_field(message, "data", (coda_grib_dynamic_type *)bds);
    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)bds);

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

static int read_grib2_message(coda_grib_product *product, coda_grib_dynamic_record *message, int64_t file_offset)
{
    coda_grib_dynamic_array *localArray;
    coda_grib_dynamic_array *gridArray;
    coda_grib_dynamic_array *dataArray;
    coda_grib_dynamic_type *type;
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

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_centre,
                                                                   buffer[5] * 256 + buffer[6]);
    coda_grib_dynamic_record_set_field(message, "centre", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_subCentre,
                                                                   buffer[7] * 256 + buffer[8]);
    coda_grib_dynamic_record_set_field(message, "subCentre", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.masterTablesVersion, buffer[9]);
    coda_grib_dynamic_record_set_field(message, "masterTablesVersion", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.localTablesVersion, buffer[10]);
    coda_grib_dynamic_record_set_field(message, "localTablesVersion", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.significanceOfReferenceTime, buffer[11]);
    coda_grib_dynamic_record_set_field(message, "significanceOfReferenceTime", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.year, buffer[12] * 256 + buffer[13]);
    coda_grib_dynamic_record_set_field(message, "year", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.month, buffer[14]);
    coda_grib_dynamic_record_set_field(message, "month", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.day, buffer[15]);
    coda_grib_dynamic_record_set_field(message, "day", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.hour, buffer[16]);
    coda_grib_dynamic_record_set_field(message, "hour", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.minute, buffer[17]);
    coda_grib_dynamic_record_set_field(message, "minute", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.second, buffer[18]);
    coda_grib_dynamic_record_set_field(message, "second", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.productionStatusOfProcessedData,
                                                                   buffer[19]);
    coda_grib_dynamic_record_set_field(message, "productionStatusOfProcessedData", type);
    coda_grib_release_dynamic_type(type);

    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.typeOfProcessedData, buffer[20]);
    coda_grib_dynamic_record_set_field(message, "typeOfProcessedData", type);
    coda_grib_release_dynamic_type(type);

    localArray = coda_grib_dynamic_array_new(grib_types.grib2_local_array);
    coda_grib_dynamic_record_set_field(message, "local", (coda_grib_dynamic_type *)localArray);
    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)localArray);

    gridArray = coda_grib_dynamic_array_new(grib_types.grib2_grid_array);
    coda_grib_dynamic_record_set_field(message, "grid", (coda_grib_dynamic_type *)gridArray);
    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)gridArray);

    dataArray = coda_grib_dynamic_array_new(grib_types.grib2_data_array);
    coda_grib_dynamic_record_set_field(message, "data", (coda_grib_dynamic_type *)dataArray);
    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)dataArray);

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
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_raw_new(grib_types.local, section_size - 5,
                                                                           raw_data);
                coda_grib_dynamic_array_add_element(localArray, type);
                coda_grib_release_dynamic_type(type);
                file_offset += section_size - 5;
                localRecordIndex++;
            }
            prev_section = 2;
        }
        else if (*buffer == 3)
        {
            coda_grib_dynamic_record *grid;
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

            grid = coda_grib_dynamic_record_new(grib_types.grib2_grid);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.localRecordIndex,
                                                                           localRecordIndex);
            coda_grib_dynamic_record_set_field(grid, "localRecordIndex", type);
            coda_grib_release_dynamic_type(type);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.sourceOfGridDefinition,
                                                                           buffer[0]);
            coda_grib_dynamic_record_set_field(grid, "sourceOfGridDefinition", type);
            coda_grib_release_dynamic_type(type);

            num_data_points = *((uint32_t *)&buffer[1]);
#ifndef WORDS_BIGENDIAN
            swap4(&num_data_points);
#endif
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.numberOfDataPoints,
                                                                           num_data_points);
            coda_grib_dynamic_record_set_field(grid, "numberOfDataPoints", type);
            coda_grib_release_dynamic_type(type);

            template_number = buffer[7] * 256 + buffer[8];
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.gridDefinitionTemplateNumber,
                                                                           template_number);
            coda_grib_dynamic_record_set_field(grid, "gridDefinitionTemplateNumber", type);
            coda_grib_release_dynamic_type(type);

            file_offset += 9;

            if (buffer[0] == 0 && (template_number <= 3 || (template_number >= 40 && template_number <= 43)))
            {
                uint32_t intvalue;

                if (read(product->fd, buffer, 58) < 0)
                {
                    coda_grib_release_dynamic_type((coda_grib_dynamic_type *)grid);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                                   strerror(errno));
                    return -1;
                }

                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.shapeOfTheEarth, buffer[0]);
                coda_grib_dynamic_record_set_field(grid, "shapeOfTheEarth", type);
                coda_grib_release_dynamic_type(type);

                type =
                    (coda_grib_dynamic_type *)
                    coda_grib_dynamic_integer_new(grib_types.scaleFactorOfRadiusOfSphericalEarth, buffer[1]);
                coda_grib_dynamic_record_set_field(grid, "scaleFactorOfRadiusOfSphericalEarth", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[2] * 256 + buffer[3]) * 256 + buffer[4]) * 256 + buffer[5];
                type =
                    (coda_grib_dynamic_type *)
                    coda_grib_dynamic_integer_new(grib_types.scaledValueOfRadiusOfSphericalEarth, intvalue);
                coda_grib_dynamic_record_set_field(grid, "scaledValueOfRadiusOfSphericalEarth", type);
                coda_grib_release_dynamic_type(type);

                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scaleFactorOfEarthMajorAxis,
                                                                               buffer[6]);
                coda_grib_dynamic_record_set_field(grid, "scaleFactorOfEarthMajorAxis", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[7] * 256 + buffer[8]) * 256 + buffer[9]) * 256 + buffer[10];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scaledValueOfEarthMajorAxis,
                                                                               intvalue);
                coda_grib_dynamic_record_set_field(grid, "scaledValueOfEarthMajorAxis", type);
                coda_grib_release_dynamic_type(type);

                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scaleFactorOfEarthMinorAxis,
                                                                               buffer[11]);
                coda_grib_dynamic_record_set_field(grid, "scaleFactorOfEarthMinorAxis", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[12] * 256 + buffer[13]) * 256 + buffer[14]) * 256 + buffer[15];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scaledValueOfEarthMinorAxis,
                                                                               intvalue);
                coda_grib_dynamic_record_set_field(grid, "scaledValueOfEarthMinorAxis", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[16] * 256 + buffer[17]) * 256 + buffer[18]) * 256 + buffer[19];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_Ni, intvalue);
                coda_grib_dynamic_record_set_field(grid, "Ni", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[20] * 256 + buffer[21]) * 256 + buffer[22]) * 256 + buffer[23];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_Nj, intvalue);
                coda_grib_dynamic_record_set_field(grid, "Nj", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[24] * 256 + buffer[25]) * 256 + buffer[26]) * 256 + buffer[27];
                type =
                    (coda_grib_dynamic_type *)
                    coda_grib_dynamic_integer_new(grib_types.basicAngleOfTheInitialProductionDomain, intvalue);
                coda_grib_dynamic_record_set_field(grid, "basicAngleOfTheInitialProductionDomain", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[28] * 256 + buffer[29]) * 256 + buffer[30]) * 256 + buffer[31];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.subdivisionsOfBasicAngle,
                                                                               intvalue);
                coda_grib_dynamic_record_set_field(grid, "subdivisionsOfBasicAngle", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[32] * 256 + buffer[33]) * 256 + buffer[34]) * 256 + buffer[35];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.latitudeOfFirstGridPoint,
                                                                               (buffer[32] & 0x80 ?
                                                                                -(intvalue - (1 << 31)) : intvalue));
                coda_grib_dynamic_record_set_field(grid, "latitudeOfFirstGridPoint", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[36] * 256 + buffer[37]) * 256 + buffer[38]) * 256 + buffer[39];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.longitudeOfFirstGridPoint,
                                                                               (buffer[36] & 0x80 ?
                                                                                -(intvalue - (1 << 31)) : intvalue));
                coda_grib_dynamic_record_set_field(grid, "longitudeOfFirstGridPoint", type);
                coda_grib_release_dynamic_type(type);

                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.resolutionAndComponentFlags,
                                                                               buffer[40]);
                coda_grib_dynamic_record_set_field(grid, "resolutionAndComponentFlags", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[41] * 256 + buffer[42]) * 256 + buffer[43]) * 256 + buffer[44];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.latitudeOfLastGridPoint,
                                                                               (buffer[41] & 0x80 ?
                                                                                -(intvalue - (1 << 31)) : intvalue));
                coda_grib_dynamic_record_set_field(grid, "latitudeOfLastGridPoint", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[45] * 256 + buffer[46]) * 256 + buffer[47]) * 256 + buffer[48];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.longitudeOfLastGridPoint,
                                                                               (buffer[45] & 0x80 ?
                                                                                -(intvalue - (1 << 31)) : intvalue));
                coda_grib_dynamic_record_set_field(grid, "longitudeOfLastGridPoint", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[49] * 256 + buffer[50]) * 256 + buffer[51]) * 256 + buffer[52];
                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_iDirectionIncrement,
                                                                               intvalue);
                coda_grib_dynamic_record_set_field(grid, "iDirectionIncrement", type);
                coda_grib_release_dynamic_type(type);

                intvalue = ((buffer[53] * 256 + buffer[54]) * 256 + buffer[55]) * 256 + buffer[56];
                if (template_number >= 40 && template_number <= 43)
                {
                    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_N, intvalue);
                    coda_grib_dynamic_record_set_field(grid, "N", type);
                    coda_grib_release_dynamic_type(type);
                }
                else
                {
                    type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.grib2_jDirectionIncrement,
                                                                                   intvalue);
                    coda_grib_dynamic_record_set_field(grid, "jDirectionIncrement", type);
                    coda_grib_release_dynamic_type(type);
                }

                type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.scanningMode, buffer[57]);
                coda_grib_dynamic_record_set_field(grid, "scanningMode", type);
                coda_grib_release_dynamic_type(type);

                file_offset += 58;

                if (section_size > 72)
                {
                    file_offset += section_size - 72;
                    if (lseek(product->fd, (off_t)(section_size - 72), SEEK_CUR) < 0)
                    {
                        char file_offset_str[21];

                        coda_grib_release_dynamic_type((coda_grib_dynamic_type *)grid);
                        coda_str64(file_offset, file_offset_str);
                        coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                                       file_offset_str, product->filename, strerror(errno));
                        return -1;
                    }
                }
            }
            else
            {
                coda_grib_release_dynamic_type((coda_grib_dynamic_type *)grid);
                coda_set_error(CODA_ERROR_PRODUCT, "unsupported grid source/template (%d/%d)", buffer[0],
                               template_number);
                return -1;
            }

            coda_grib_dynamic_array_add_element(gridArray, (coda_grib_dynamic_type *)grid);
            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)grid);

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
            coda_grib_dynamic_record *data;
            uint8_t *bitmask = NULL;

            /* Section 7: Data Section */
            if (prev_section != 5 && prev_section != 6)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "unexpected Section Number (%d after %d)", *buffer - '0',
                               prev_section);
                return -1;
            }

            data = coda_grib_dynamic_record_new(grib_types.grib2_data);

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.gridRecordIndex,
                                                                           gridSectionIndex);
            coda_grib_dynamic_record_set_field(data, "gridRecordIndex", type);
            coda_grib_release_dynamic_type(type);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.bitsPerValue, bitsPerValue);
            coda_grib_dynamic_record_set_field(data, "bitsPerValue", type);
            coda_grib_release_dynamic_type(type);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.decimalScaleFactor,
                                                                           decimalScaleFactor);
            coda_grib_dynamic_record_set_field(data, "decimalScaleFactor", type);
            coda_grib_release_dynamic_type(type);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.binaryScaleFactor,
                                                                           binaryScaleFactor);
            coda_grib_dynamic_record_set_field(data, "binaryScaleFactor", type);
            coda_grib_release_dynamic_type(type);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_real_new(grib_types.referenceValue, referenceValue);
            coda_grib_dynamic_record_set_field(data, "referenceValue", type);
            coda_grib_release_dynamic_type(type);

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

            type = (coda_grib_dynamic_type *)coda_grib_dynamic_value_array_new(grib_types.values, num_elements,
                                                                               file_offset, bitsPerValue,
                                                                               decimalScaleFactor, binaryScaleFactor,
                                                                               referenceValue, bitmask);
            coda_grib_dynamic_record_set_field(data, "values", type);
            coda_grib_release_dynamic_type(type);

            coda_grib_dynamic_array_add_element(dataArray, (coda_grib_dynamic_type *)data);
            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)data);

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
    coda_grib_dynamic_type *type;
    coda_grib_product *grib_product;
    long message_number;
    int open_flags;
    uint8_t buffer[28];
    int64_t message_size;
    int64_t file_offset = 0;

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
        coda_grib_dynamic_record *message;

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
                grib_product->root_type = (coda_dynamic_type *)coda_grib_dynamic_array_new(grib_types.grib1_root);
            }
            message_size = ((buffer[4] * 256) + buffer[5]) * 256 + buffer[6];

            message = coda_grib_dynamic_record_new(grib_types.grib1_message);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.editionNumber, 1);
            coda_grib_dynamic_record_set_field(message, "editionNumber", type);
            coda_grib_release_dynamic_type(type);
            if (read_grib1_message(grib_product, message, file_offset + 8) != 0)
            {
                coda_grib_release_dynamic_type((coda_grib_dynamic_type *)message);
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
                grib_product->root_type = (coda_dynamic_type *)coda_grib_dynamic_array_new(grib_types.grib2_root);
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

            message = coda_grib_dynamic_record_new(grib_types.grib2_message);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.editionNumber, 2);
            coda_grib_dynamic_record_set_field(message, "editionNumber", type);
            coda_grib_release_dynamic_type(type);
            type = (coda_grib_dynamic_type *)coda_grib_dynamic_integer_new(grib_types.discipline, buffer[6]);
            coda_grib_dynamic_record_set_field(message, "discipline", type);
            coda_grib_release_dynamic_type(type);

            if (read_grib2_message(grib_product, message, file_offset + 16) != 0)
            {
                coda_grib_release_dynamic_type((coda_grib_dynamic_type *)message);
                coda_grib_close((coda_product *)grib_product);
                return -1;
            }
        }

        if (coda_grib_dynamic_array_add_element((coda_grib_dynamic_array *)grib_product->root_type,
                                                (coda_grib_dynamic_type *)message) != 0)
        {
            coda_grib_release_dynamic_type((coda_grib_dynamic_type *)message);
            coda_grib_close((coda_product *)grib_product);
            return -1;
        }
        coda_grib_release_dynamic_type((coda_grib_dynamic_type *)message);

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
        coda_release_dynamic_type(grib_product->root_type);
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

int coda_grib_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type, coda_type **type)
{
    *type = (coda_type *)((coda_grib_dynamic_type *)dynamic_type)->definition;
    return 0;
}
