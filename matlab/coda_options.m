% CODA_OPTIONS A description of each of the CODA MATLAB interface options.
%
%   ConvertNumbersToDouble:
%     If set to 0 then the CODA MATLAB interface will, when reading data
%     from a product file, use a matlab class that best matches the
%     datatype of the data element in the product file (i.e. use 'int8'
%     when the data is a one byte signed integer).
%     If set to 1 then the CODA MATLAB interface will use the matlab class
%     'double' for all numbers that are read from a product file. 
%     The default value for this option is: 1
%
%   FilterRecordFields:
%     Some records contain fields that have a fixed value or are spare
%     fields. If this option is set to 1 then these kinds of fields will be
%     filtered out when retrieving a record from a product file. If this
%     option is set to 0 then all fields will be returned.
%     The default value for this option is: 1
%
%   PerformConversions:
%     The CODA library has a global option that allows you to switch
%     between reading data from a productfile in the way that it was
%     exactly stored or in a more convenient way. For instance,
%     sometimes floating point values with one digit precision are first
%     multiplied by 10 and stored as an integer value in the product. If
%     PerformConversions is set to 0 then CODA will read this integer
%     value, but if it is set to 1 then CODA will first convert the
%     integer back to a floating point value and divide it by 10 again.
%     To see which fields of a product are effected by the
%     PerformConversions option and what the conversion factor is look
%     at the corresponding CODA Product Format Definition documentation.
%     You should also be aware that changing this option not only
%     effects the result of coda_fetch, but also the result of
%     coda_class (since this may, for instance, return 'single' instead
%     of 'int16' if PerformConversions is set to 1) and coda_unit.
%     The default value for this option is: 1
%
%   Use64bitInteger:
%     Some data elements in a product file are stored as 64bit integers.
%     The CODA MATLAB interface is able to read this data and return them
%     to matlab with the not fully supported matlab classes 'int64' and
%     'uint64'. If you set Use64bitInteger to 0 then the CODA MATLAB
%     interface will convert the 64bit integer to a double and return the
%     data with a matlab class 'double'.
%     Note that if ConvertNumbersToDouble is set to 1 then all integers
%     will already be converted to doubles so in that case this option
%     won't have any effect.
%     The default value for this option is: 0
%
%   UseMMap:
%     By default CODA uses a technique called 'memory mapping' to open
%     and access data from product files. Using mmap greatly outperforms
%     the default approach of reading data using the open()/read()
%     combination. The downside of mapping a file into memory is that it
%     takes away valuable address space. When you run a 32-bit Operating
%     System your maximum addressable memory range is 4GB and if you
%     simultaneously try to keep a few large product files open your
%     memory space can quickly become full. Opening additional files will
%     then produce 'out of memory' errors. Note that this 'out of memory'
%     situation has nothing to do with the amount of RAM you have
%     installed in your computer. It is only related to the size of a
%     memory pointer on your system, which is limited to 4GB.
%     If you are using CODA in a situation where you need to have multiple
%     large product files open at the same time you can turn of the use of
%     memory mapping by disabling this option. If you change the memory
%     mapping option, the new setting will only be applicable for files
%     that will be opened after you changed the option. Any files that
%     were already open will keep using the mechanism with which they were
%     opened.
%
%   UseSpecialTypes:
%     The CODA type system contains a series of special types that were
%     introduced to make it easier for the user to read certain types of
%     information. Examples of special types are the 'time', 'complex', and
%     'no data' types. Each special data type is an abstraction on top of
%     another non-special data type.
%     Sometimes you want to access a file using just the non-special data
%     types (e.g. if you want to get to the raw time data in a file).
%     If you disable this option, CODA will use the base type of a special
%     type (and not the special type itself) when reading data or
%     retrieving information about a data item.
%     See the CODA Product Format Definition documentation for more
%     information about special types.
%     The default value for this option is: 1
%
%   See also CODA_GETOPT, CODA_SETOPT
%
