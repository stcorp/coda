ACLOCAL_AMFLAGS = -I m4

.PHONY: doc

# *** general build settings ****

DOXYGEN = @DOXYGEN@
INDENT = @INDENT@

PYTHON_SWIGFLAGS = -w119,451 -python -Ipython -I$(srcdir)/python -DPRINTF_ATTR= @SWIGFLAGS@
JAVA_SWIGFLAGS = -w119,451 -java -Ijava -I$(srcdir)/java -package nl.stcorp.coda -outdir java/nl/stcorp/coda -DPRINTF_ATTR= @SWIGFLAGS@
JNICPPFLAGS = @JNICPPFLAGS@
JNIEXT = @JNIEXT@
JAVAROOT = java

AM_CPPFLAGS = -Ilibcoda -I$(srcdir)/libcoda
AM_YFLAGS = -d

HDF4LIBS = @HDF4LIBS@
HDF5LIBS = @HDF5LIBS@

SUFFIXES = .i

BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
INDENTFILES =

INSTALL_DATA_HOOK_TARGETS =
UNINSTALL_HOOK_TARGETS =
install-data-hook: $(INSTALL_DATA_HOOK_TARGETS)
uninstall-hook: $(UNINSTALL_HOOK_TARGETS)

CLEAN_LOCAL_TARGETS =
clean-local: $(CLEAN_LOCAL_TARGETS)

ALL_LOCAL_TARGETS =
all-local: $(ALL_LOCAL_TARGETS)

# *** targets ***

if SUBPACKAGE_MODE

noinst_PROGRAMS = codadd
noinst_LTLIBRARIES = libcoda_internal.la libexpat_internal.la libpcre_internal.la libz_internal.la

else

# programs

bin_PROGRAMS = codacheck codacmp codadd codadump codaeval codafind
noinst_PROGRAMS = generate-finc findtypedef

# libraries (+ related files)

lib_LTLIBRARIES = libcoda.la
noinst_LTLIBRARIES = libcoda_internal.la libexpat_internal.la libpcre_internal.la libz_internal.la

if BUILD_IDL
idldlmexec_LTLIBRARIES = coda-idl.la
endif

if BUILD_MATLAB
matlabmexexec_LTLIBRARIES = coda_matlab.la
endif

if BUILD_PYTHON
codapython_PYTHON = \
	$(srcdir)/python/__init__.py \
	$(srcdir)/python/codapython.py \
	python/codac.py
codapython_LTLIBRARIES = _codac.la
endif

if BUILD_JAVA
java_LTLIBRARIES = libcoda_jni.la
endif

# empty directory for .codadef definition files

install_codadefdir:
	test -z "$(DESTDIR)$(definitionsdir)" || $(MKDIR_P) "$(DESTDIR)$(definitionsdir)"
INSTALL_DATA_HOOK_TARGETS += install_codadefdir

# headers

nodist_include_HEADERS = coda.h

endif

# *** SWIG dependencies ***

@AMDEP_TRUE@DEP_FILES = ./$(DEPDIR)/codac.Pc ./$(DEPDIR)/coda_jni.Pc
@AMDEP_TRUE@@am__include@ @am__quote@./$(DEPDIR)/codac.Pc@am__quote@
@AMDEP_TRUE@@am__include@ @am__quote@./$(DEPDIR)/coda_jni.Pc@am__quote@

# *** build rules ***

# expat

libexpat_internal_la_SOURCES = \
	libcoda/expat/xmlparse.c \
	libcoda/expat/xmlrole.c \
	libcoda/expat/xmltok.c \
	libcoda/expat/ascii.h \
	libcoda/expat/asciitab.h \
	libcoda/expat/coda_expat_mangle.h \
	libcoda/expat/expat.h \
	libcoda/expat/expat_external.h \
	libcoda/expat/iasciitab.h \
	libcoda/expat/internal.h \
	libcoda/expat/latin1tab.h \
	libcoda/expat/nametab.h \
	libcoda/expat/utf8tab.h \
	libcoda/expat/xmlrole.h \
	libcoda/expat/xmltok.h \
	libcoda/expat/xmltok_impl.h
libexpat_internal_la_CPPFLAGS = -Ilibcoda/expat -I$(srcdir)/libcoda/expat $(AM_CPPFLAGS)
EXTRA_DIST += libcoda/expat/xmltok_impl.c libcoda/expat/xmltok_ns.c
EXTRA_DIST += libcoda/expat/COPYING libcoda/expat/README

# pcre

libpcre_internal_la_SOURCES = \
	libcoda/pcre/coda_pcre_config.h \
	libcoda/pcre/coda_pcre_mangle.h \
	libcoda/pcre/pcre.h \
	libcoda/pcre/pcre_chartables.c \
	libcoda/pcre/pcre_compile.c \
	libcoda/pcre/pcre_config.c \
	libcoda/pcre/pcre_dfa_exec.c \
	libcoda/pcre/pcre_exec.c \
	libcoda/pcre/pcre_fullinfo.c \
	libcoda/pcre/pcre_get.c \
	libcoda/pcre/pcre_globals.c \
	libcoda/pcre/pcre_info.c \
	libcoda/pcre/pcre_internal.h \
	libcoda/pcre/pcre_maketables.c \
	libcoda/pcre/pcre_newline.c \
	libcoda/pcre/pcre_ord2utf8.c \
	libcoda/pcre/pcre_printint.src \
	libcoda/pcre/pcre_refcount.c \
	libcoda/pcre/pcre_study.c \
	libcoda/pcre/pcre_tables.c \
	libcoda/pcre/pcre_try_flipped.c \
	libcoda/pcre/pcre_ucd.c \
	libcoda/pcre/pcre_valid_utf8.c \
	libcoda/pcre/pcre_version.c \
	libcoda/pcre/pcre_xclass.c \
	libcoda/pcre/ucp.h
libpcre_internal_la_CPPFLAGS = -Ilibcoda/pcre -I$(srcdir)/libcoda/pcre $(AM_CPPFLAGS)
EXTRA_DIST += libcoda/pcre/COPYING libcoda/pcre/README

# zlib

libz_internal_la_SOURCES = \
	libcoda/zlib/adler32.c \
	libcoda/zlib/coda_zlib_mangle.h \
	libcoda/zlib/crc32.c \
	libcoda/zlib/crc32.h \
	libcoda/zlib/inffast.c \
	libcoda/zlib/inffast.h \
	libcoda/zlib/inffixed.h \
	libcoda/zlib/inflate.c \
	libcoda/zlib/inflate.h \
	libcoda/zlib/inftrees.c \
	libcoda/zlib/inftrees.h \
	libcoda/zlib/zconf.h \
	libcoda/zlib/zlib.h \
	libcoda/zlib/zutil.c \
	libcoda/zlib/zutil.h
libz_internal_la_CPPFLAGS = -Ilibcoda/zlib -I$(srcdir)/libcoda/zlib $(AM_CPPFLAGS)
EXTRA_DIST += libcoda/zlib/COPYING libcoda/zlib/README

# libcoda

libcoda_la_SOURCES = \
	libcoda/coda-ascbin-cursor.c \
	libcoda/coda-ascbin-internal.h \
	libcoda/coda-ascbin.c \
	libcoda/coda-ascbin.h \
	libcoda/coda-ascii-cursor.c \
	libcoda/coda-ascii-internal.h \
	libcoda/coda-ascii.c \
	libcoda/coda-ascii.h \
	libcoda/coda-bin-cursor.c \
	libcoda/coda-bin-internal.h \
	libcoda/coda-bin.c \
	libcoda/coda-bin.h \
	libcoda/coda-cdf-cursor.c \
	libcoda/coda-cdf-internal.h \
	libcoda/coda-cdf-type.c \
	libcoda/coda-cdf.c \
	libcoda/coda-cdf.h \
	libcoda/coda-check.c \
	libcoda/coda-cursor-read.c \
	libcoda/coda-cursor.c \
	libcoda/coda-definition-parse.c \
	libcoda/coda-definition.c \
	libcoda/coda-definition.h \
	libcoda/coda-errno.c \
	libcoda/coda-expr-parser.y \
	libcoda/coda-expr-tokenizer.l \
	libcoda/coda-expr.c \
	libcoda/coda-expr.h \
	libcoda/coda-filefilter.c \
	libcoda/coda-filefilter.h \
	libcoda/coda-grib-cursor.c \
	libcoda/coda-grib-internal.h \
	libcoda/coda-grib-type.c \
	libcoda/coda-grib.c \
	libcoda/coda-grib.h \
	libcoda/coda-internal.h \
	libcoda/coda-mem-cursor.c \
	libcoda/coda-mem-internal.h \
	libcoda/coda-mem-type.c \
	libcoda/coda-mem.c \
	libcoda/coda-mem.h \
	libcoda/coda-netcdf-cursor.c \
	libcoda/coda-netcdf-internal.h \
	libcoda/coda-netcdf-type.c \
	libcoda/coda-netcdf.c \
	libcoda/coda-netcdf.h \
	libcoda/coda-path.h \
	libcoda/coda-product.c \
	libcoda/coda-read-array.h \
	libcoda/coda-read-bits.h \
	libcoda/coda-read-bytes.h \
	libcoda/coda-read-bytes-in-bounds.h \
	libcoda/coda-read-partial-array.h \
	libcoda/coda-rinex.c \
	libcoda/coda-rinex.h \
	libcoda/coda-sp3.c \
	libcoda/coda-sp3.h \
	libcoda/coda-swap2.h \
	libcoda/coda-swap4.h \
	libcoda/coda-swap8.h \
	libcoda/coda-time.c \
	libcoda/coda-transpose-array.h \
	libcoda/coda-tree.c \
	libcoda/coda-tree.h \
	libcoda/coda-type.c \
	libcoda/coda-type.h \
	libcoda/coda-utils.c \
	libcoda/coda-xml-internal.h \
	libcoda/coda-xml-parser.c \
	libcoda/coda-xml.c \
	libcoda/coda-xml.h \
	libcoda/coda.c \
	libcoda/hashtable.c \
	libcoda/hashtable.h \
	libcoda/ipow.h \
	libcoda/ziparchive.c \
	libcoda/ziparchive.h
libcoda_hdf4_files = \
	libcoda/coda-hdf4-cursor.c \
	libcoda/coda-hdf4-internal.h \
	libcoda/coda-hdf4-type.c \
	libcoda/coda-hdf4.c \
	libcoda/coda-hdf4.h
libcoda_hdf5_files = \
	libcoda/coda-hdf5-cursor.c \
	libcoda/coda-hdf5-internal.h \
	libcoda/coda-hdf5-type.c \
	libcoda/coda-hdf5.c \
	libcoda/coda-hdf5.h
if WITH_HDF4
libcoda_la_SOURCES += $(libcoda_hdf4_files)
else
INDENTFILES += $(libcoda_hdf4_files)
endif
if WITH_HDF5
libcoda_la_SOURCES += $(libcoda_hdf5_files)
else
INDENTFILES += $(libcoda_hdf5_files)
endif
libcoda_la_CPPFLAGS = -Ilibcoda/expat -I$(srcdir)/libcoda/expat -Ilibcoda/pcre -I$(srcdir)/libcoda/pcre -Ilibcoda/zlib -I$(srcdir)/libcoda/zlib $(AM_CPPFLAGS)
libcoda_la_LDFLAGS = -no-undefined -version-info $(LIBCODA_CURRENT):$(LIBCODA_REVISION):$(LIBCODA_AGE)
libcoda_la_LIBADD = @LTLIBOBJS@ libexpat_internal.la libpcre_internal.la libz_internal.la $(HDF4LIBS) $(HDF5LIBS)
INDENTFILES += $(libcoda_la_SOURCES) libcoda/coda.h.in
BUILT_SOURCES += libcoda/coda-expr-parser.h

libcoda_internal_la_SOURCES = \
	libcoda/coda-ascbin-cursor.c \
	libcoda/coda-ascbin-internal.h \
	libcoda/coda-ascbin.c \
	libcoda/coda-ascbin.h \
	libcoda/coda-ascii-cursor.c \
	libcoda/coda-ascii-internal.h \
	libcoda/coda-ascii.c \
	libcoda/coda-ascii.h \
	libcoda/coda-bin-cursor.c \
	libcoda/coda-bin-internal.h \
	libcoda/coda-bin.c \
	libcoda/coda-bin.h \
	libcoda/coda-cdf-cursor.c \
	libcoda/coda-cdf-internal.h \
	libcoda/coda-cdf-type.c \
	libcoda/coda-cdf.c \
	libcoda/coda-cdf.h \
	libcoda/coda-check.c \
	libcoda/coda-cursor-read.c \
	libcoda/coda-cursor.c \
	libcoda/coda-definition-parse.c \
	libcoda/coda-definition.c \
	libcoda/coda-definition.h \
	libcoda/coda-errno.c \
	libcoda/coda-expr-parser.c \
	libcoda/coda-expr-parser.h \
	libcoda/coda-expr-tokenizer.l \
	libcoda/coda-expr.c \
	libcoda/coda-expr.h \
	libcoda/coda-filefilter.c \
	libcoda/coda-filefilter.h \
	libcoda/coda-grib-cursor.c \
	libcoda/coda-grib-internal.h \
	libcoda/coda-grib-type.c \
	libcoda/coda-grib.c \
	libcoda/coda-grib.h \
	libcoda/coda-internal.h \
	libcoda/coda-mem-cursor.c \
	libcoda/coda-mem-internal.h \
	libcoda/coda-mem-type.c \
	libcoda/coda-mem.c \
	libcoda/coda-mem.h \
	libcoda/coda-netcdf-cursor.c \
	libcoda/coda-netcdf-internal.h \
	libcoda/coda-netcdf-type.c \
	libcoda/coda-netcdf.c \
	libcoda/coda-netcdf.h \
	libcoda/coda-path.h \
	libcoda/coda-product.c \
	libcoda/coda-read-array.h \
	libcoda/coda-read-bits.h \
	libcoda/coda-read-bytes.h \
	libcoda/coda-read-bytes-in-bounds.h \
	libcoda/coda-read-partial-array.h \
	libcoda/coda-rinex.c \
	libcoda/coda-rinex.h \
	libcoda/coda-sp3.c \
	libcoda/coda-sp3.h \
	libcoda/coda-swap2.h \
	libcoda/coda-swap4.h \
	libcoda/coda-swap8.h \
	libcoda/coda-time.c \
	libcoda/coda-transpose-array.h \
	libcoda/coda-tree.c \
	libcoda/coda-tree.h \
	libcoda/coda-type.c \
	libcoda/coda-type.h \
	libcoda/coda-utils.c \
	libcoda/coda-xml-internal.h \
	libcoda/coda-xml-parser.c \
	libcoda/coda-xml.c \
	libcoda/coda-xml.h \
	libcoda/coda.c \
	libcoda/hashtable.c \
	libcoda/hashtable.h \
	libcoda/ipow.h \
	libcoda/ziparchive.c \
	libcoda/ziparchive.h
if WITH_HDF4
libcoda_internal_la_SOURCES += $(libcoda_hdf4_files)
endif
if WITH_HDF5
libcoda_internal_la_SOURCES += $(libcoda_hdf5_files)
endif
libcoda_internal_la_CPPFLAGS = -Ilibcoda/expat -I$(srcdir)/libcoda/expat -Ilibcoda/pcre -I$(srcdir)/libcoda/pcre -Ilibcoda/zlib -I$(srcdir)/libcoda/zlib $(AM_CPPFLAGS)
libcoda_internal_la_LIBADD = $(libcoda_la_LIBADD)

# tools/codacheck

codacheck_SOURCES = tools/codacheck/codacheck.c
codacheck_LDADD = libcoda.la
codacheck_LDFLAGS = -static
INDENTFILES += $(codacheck_SOURCES)

# tools/codacmp

codacmp_SOURCES = tools/codacmp/codacmp.c
codacmp_LDADD = libcoda.la
codacmp_LDFLAGS = -static
INDENTFILES += $(codacmp_SOURCES)

# tools/codadd

codadd_SOURCES = \
	tools/codadd/codadd.c \
	tools/codadd/codadd-definition.c \
	tools/codadd/codadd-detect.c \
	tools/codadd/codadd-doc.c \
	tools/codadd/codadd-list.c \
	tools/codadd/codadd-xmlschema.c
codadd_LDFLAGS = -static
# keep this linked to libcoda_internal.la because of SUBPACKAGE_MODE
codadd_LDADD = libcoda_internal.la
INDENTFILES += $(codadd_SOURCES)

# tools/codadump

codadump_SOURCES = \
	tools/codadump/codadump-ascii.c \
	tools/codadump/codadump-debug.c \
	tools/codadump/codadump-dim.c \
	tools/codadump/codadump-filter.c \
	tools/codadump/codadump-json.c \
	tools/codadump/codadump-traverse.c \
	tools/codadump/codadump-yaml.c \
	tools/codadump/codadump.c \
	tools/codadump/codadump-filter.h \
	tools/codadump/codadump.h
codadump_hdf4_files = tools/codadump/codadump-hdf4.c
if WITH_HDF4
codadump_SOURCES += $(codadump_hdf4_files)
else
INDENTFILES += $(codadump_hdf4_files)
endif
codadump_CPPFLAGS = -Itools/codadump -I$(srcdir)/tools/codadump $(AM_CPPFLAGS)
codadump_LDADD = libcoda.la $(HDF4LIBS)
codadump_LDFLAGS = -static
INDENTFILES += $(codadump_SOURCES)

# tools/codaeval

codaeval_SOURCES = \
	tools/codaeval/codaeval.c
codaeval_LDFLAGS = -static
codaeval_LDADD = libcoda.la
INDENTFILES += $(codaeval_SOURCES)

# tools/codafind

codafind_SOURCES = \
	tools/codafind/codafind.c
codafind_LDFLAGS = -static
codafind_LDADD = libcoda.la
INDENTFILES += $(codafind_SOURCES)

# fortran

if !SUBPACKAGE_MODE

fortran_DATA = \
	fortran/Makefile \
	fortran/coda.inc \
	fortran/coda_fortran.c \
	fortran/example.f

fortran_SCRIPTS = \
	fortran/checkf77.sh

endif

EXTRA_DIST += \
	fortran/Makefile.in \
	fortran/coda_fortran.c \
	fortran/checkf77.sh \
	fortran/example.f

generate_finc_SOURCES = fortran/generate-finc.c
generate_finc_LDADD = @LIBOBJS@
generate_finc_LDFLAGS = -static

INDENTFILES += $(generate_finc_SOURCES)

fortran/coda.inc: generate-finc$(EXEEXT)
	./generate-finc > fortran/coda.inc
CLEANFILES += fortran/coda.inc

# idl

CODA_IDL_SOURCE_FILES = idl/coda-idl.c
INDENTFILES += $(CODA_IDL_SOURCE_FILES)

if BUILD_IDL
coda_idl_la_SOURCES = $(CODA_IDL_SOURCE_FILES)
coda_idl_la_LDFLAGS = -module -shared -avoid-version
coda_idl_la_LIBADD = libcoda_internal.la
coda_idl_la_DEPENDENCIES = libcoda_internal.la
endif

# matlab

CODA_MATLAB_SOURCE_FILES = \
	matlab/coda-matlab.h \
	matlab/coda-matlab-getdata.c \
	matlab/coda-matlab-traverse.c \
	matlab/coda_matlab.c
INDENTFILES += $(CODA_MATLAB_SOURCE_FILES)

if BUILD_MATLAB
coda_matlab_la_SOURCES = $(CODA_MATLAB_SOURCE_FILES)
coda_matlab_la_CPPFLAGS = -DMATLAB_MEX_FILE
coda_matlab_la_LDFLAGS = -module -shared -avoid-version -export-symbols-regex mexFunction -shrext @MEXEXT@
coda_matlab_la_LIBADD = libcoda_internal.la
coda_matlab_la_DEPENDENCIES = libcoda_internal.la
endif

# python

python/codac.c: libcoda/coda.h.in $(srcdir)/python/codac.i \
    $(srcdir)/python/coda_typemaps.i \
    $(srcdir)/python/coda_ignore.i \
    $(srcdir)/python/coda_rename.i 
	@$(MKDIR_P) python
@AMDEP_TRUE@	source='$(srcdir)/python/codac.i' object='python/codac.c' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@	depfile='$(DEPDIR)/codac.Pc' tmpdepfile='$(DEPDIR)/codac.TPc' @AMDEPBACKSLASH@
@AMDEP_TRUE@	depmode='none' $(depcomp) @AMDEPBACKSLASH@
@AMDEP_TRUE@	$(SWIG) $(PYTHON_SWIGFLAGS) -M $(srcdir)/python/codac.i > $(DEPDIR)/codac.Pc
	$(SWIG) $(PYTHON_SWIGFLAGS) -o python/codac.c $(srcdir)/python/codac.i

EXTRA_DIST += \
	python/codac.c \
	python/codac.py \
	$(srcdir)/python/codac.i \
	$(srcdir)/python/coda_typemaps.i \
	$(srcdir)/python/coda_ignore.i \
	$(srcdir)/python/coda_rename.i

if BUILD_PYTHON
_codac_la_SOURCES = python/codac.c
_codac_la_LDFLAGS = -module -shared -avoid-version
_codac_la_LIBADD = libcoda_internal.la
_codac_la_DEPENDENCIES = libcoda_internal.la
endif

# java

java/coda_jni.c: libcoda/coda.h.in $(srcdir)/java/codac.i \
    $(srcdir)/java/coda_ignore.i \
    $(srcdir)/java/coda_rename.i 
	@$(MKDIR_P) java/nl/stcorp/coda
@AMDEP_TRUE@	source='$(srcdir)/java/codac.i' object='java/coda_jni.c' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@	depfile='$(DEPDIR)/coda_jni.Pc' tmpdepfile='$(DEPDIR)/coda_jni.TPc' @AMDEPBACKSLASH@
@AMDEP_TRUE@	depmode='none' $(depcomp) @AMDEPBACKSLASH@
@AMDEP_TRUE@	$(SWIG) $(JAVA_SWIGFLAGS) -M $(srcdir)/java/codac.i > $(DEPDIR)/coda_jni.Pc
	$(SWIG) $(JAVA_SWIGFLAGS) -o java/coda_jni.c  $(srcdir)/java/codac.i

java_files = \
	$(srcdir)/java/nl/stcorp/coda/CodaException.java \
	$(srcdir)/java/nl/stcorp/coda/Product.java \
	$(srcdir)/java/nl/stcorp/coda/Type.java \
	$(srcdir)/java/nl/stcorp/coda/Cursor.java \
	$(srcdir)/java/nl/stcorp/coda/Expression.java \
	$(srcdir)/java/nl/stcorp/coda/Coda.java \
	java/nl/stcorp/coda/ArrayOrderingEnum.java \
	java/nl/stcorp/coda/FilefilterStatusEnum.java \
	java/nl/stcorp/coda/FormatEnum.java \
	java/nl/stcorp/coda/NativeTypeEnum.java \
	java/nl/stcorp/coda/SpecialTypeEnum.java \
	java/nl/stcorp/coda/ExpressionTypeEnum.java \
	java/nl/stcorp/coda/TypeClassEnum.java \
	java/nl/stcorp/coda/codacConstants.java \
	java/nl/stcorp/coda/codac.java \
	java/nl/stcorp/coda/codacJNI.java \
	java/nl/stcorp/coda/SWIGTYPE_p_coda_product_struct.java \
	java/nl/stcorp/coda/SWIGTYPE_p_coda_expression_struct.java \
	java/nl/stcorp/coda/SWIGTYPE_p_coda_cursor_struct.java \
	java/nl/stcorp/coda/SWIGTYPE_p_coda_type_struct.java

EXTRA_DIST += \
	java/coda_jni.c \
	$(java_files) \
	java/CodaTest.java \
	$(srcdir)/java/codac.i \
	$(srcdir)/java/coda_ignore.i \
	$(srcdir)/java/coda_rename.i


if BUILD_JAVA
noinst_JAVA = $(java_files)

java_class_files = \
	nl/stcorp/coda/CodaException.class \
	nl/stcorp/coda/Product.class \
	nl/stcorp/coda/Type.class \
	nl/stcorp/coda/Cursor.class \
	nl/stcorp/coda/Expression.class \
	nl/stcorp/coda/Coda.class \
	nl/stcorp/coda/ArrayOrderingEnum.class \
	nl/stcorp/coda/FilefilterStatusEnum.class \
	nl/stcorp/coda/FormatEnum.class \
	nl/stcorp/coda/NativeTypeEnum.class \
	nl/stcorp/coda/SpecialTypeEnum.class \
	nl/stcorp/coda/ExpressionTypeEnum.class \
	nl/stcorp/coda/TypeClassEnum.class \
	nl/stcorp/coda/codacConstants.class \
	nl/stcorp/coda/codac.class \
	nl/stcorp/coda/codacJNI.class \
	nl/stcorp/coda/SWIGTYPE_p_coda_product_struct.class \
	nl/stcorp/coda/SWIGTYPE_p_coda_expression_struct.class \
	nl/stcorp/coda/SWIGTYPE_p_coda_cursor_struct.class \
	nl/stcorp/coda/SWIGTYPE_p_coda_type_struct.class \
	nl/stcorp/coda/ArrayOrderingEnum\$$SwigNext.class \
	nl/stcorp/coda/FilefilterStatusEnum\$$SwigNext.class \
	nl/stcorp/coda/FormatEnum\$$SwigNext.class \
	nl/stcorp/coda/ExpressionTypeEnum\$$SwigNext.class \
	nl/stcorp/coda/NativeTypeEnum\$$SwigNext.class \
	nl/stcorp/coda/SpecialTypeEnum\$$SwigNext.class \
	nl/stcorp/coda/TypeClassEnum\$$SwigNext.class

java/coda.jar: classnoinst.stamp
	cd java && $(JAR) -cf coda.jar $(java_class_files)

java_DATA = \
	java/CodaTest.java \
	java/build.xml \
	java/coda.jar

libcoda_jni_la_SOURCES = java/coda_jni.c
libcoda_jni_la_CPPFLAGS = $(JNICPPFLAGS) $(AM_CPPFLAGS) -fno-strict-aliasing
libcoda_jni_la_LDFLAGS = -module -shared -avoid-version -shrext $(JNIEXT)
libcoda_jni_la_LIBADD = libcoda_internal.la
libcoda_jni_la_DEPENDENCIES = libcoda_internal.la

CLEANFILES += java/coda.jar

CLEAN_LOCAL_TARGETS += clean_java_class_files
clean_java_class_files:
	cd java && rm -f $(java_class_files)

endif

# *** misc distribution files ***

EXTRA_DIST += \
	CHANGES \
	COPYING \
	FAQ \
	INSTALL \
	LICENSES \
	README \
	bootstrap \
	codadef.sh

# *** idl data ***

CODA_IDL_DLMFILES = idl/coda-idl.dlm

EXTRA_DIST += $(CODA_IDL_DLMFILES) idl/coda-idl.dlm.in

if BUILD_IDL
idldlmexec_DATA = $(CODA_IDL_DLMFILES)
endif

# *** matlab data ***

CODA_MATLAB_MFILES = \
	matlab/coda_attributes.m \
	matlab/coda_class.m \
	matlab/coda_clearall.m \
	matlab/coda_close.m \
	matlab/coda_description.m \
	matlab/coda_eval.m \
	matlab/coda_fetch.m \
	matlab/coda_fieldavailable.m \
	matlab/coda_fieldcount.m \
	matlab/coda_fieldnames.m \
	matlab/coda_getopt.m \
	matlab/coda_open.m \
	matlab/coda_open_as.m \
	matlab/coda_options.m \
	matlab/coda_param.m \
	matlab/coda_product_class.m \
	matlab/coda_product_type.m \
	matlab/coda_product_version.m \
	matlab/coda_setopt.m \
	matlab/coda_size.m \
	matlab/coda_time_to_string.m \
	matlab/coda_unit.m \
	matlab/coda_version.m

EXTRA_DIST += $(CODA_MATLAB_MFILES)

if BUILD_MATLAB
matlabmexexec_DATA = $(CODA_MATLAB_MFILES)
endif

# *** documentation ***

if INSTALL_DOCS
include doclibcoda.mk
include docjava.mk

DOCFILES = \
	doc/html/index.html \
	doc/html/css/codadef.css \
	doc/html/css/codadoc.css \
	doc/html/images/coda-large.png \
	$(LIBCODA_DOCFILES) \
	$(JAVA_DOCFILES) \
	doc/html/codadef/codadef-cdf.html \
	doc/html/codadef/codadef-expressions.html \
	doc/html/codadef/codadef-format.html \
	doc/html/codadef/codadef-grib.html \
	doc/html/codadef/codadef-hdf4.html \
	doc/html/codadef/codadef-hdf5.html \
	doc/html/codadef/codadef-netcdf.html \
	doc/html/codadef/codadef-rinex.html \
	doc/html/codadef/codadef-sp3.html \
	doc/html/codadef/codadef-xml.html \
	doc/html/codacheck/index.html \
	doc/html/codacmp/index.html \
	doc/html/codadd/index.html \
	doc/html/codadump/index.html \
	doc/html/codaeval/index.html \
	doc/html/codafind/index.html \
	doc/html/fortran/index.html \
	doc/html/idl/index.html \
	doc/html/matlab/index.html \
	doc/html/python/index.html

EXTRA_DIST += \
	$(DOCFILES) \
	libcoda/doc/Doxyfile.in \
	libcoda/doc/extrapages.txt \
	libcoda/doc/footer.html \
	libcoda/doc/header.html \
	java/doc/Doxyfile.in \
	java/doc/extrapages.txt \
	java/doc/footer.html \
	java/doc/header.html \
	doclibcoda.mk \
	docjava.mk

if !SUBPACKAGE_MODE
nobase_pkgdata_DATA = $(DOCFILES)
endif

doc: libcoda_doc java_doc

docclean: libcoda_docclean java_docclean

libcoda_doc: coda.h libcoda/doc/Doxyfile
	@if test "x$(DOXYGEN)" = x ; then \
	  echo "Doxygen not available. Documentation was not (re)created." ; \
	else \
	  $(MAKE) libcoda_docclean ; \
	  echo "creating libcoda documentation" ; \
	  test -d doc/html || $(MKDIR_P) doc/html ; \
	  cd libcoda/doc ; \
	  $(DOXYGEN) Doxyfile ; \
	  cd ../.. ; \
	  rm -rf doc/html/libcoda/doxygen.* doc/html/libcoda/tab* ; \
	  echo "creating doclibcoda.mk" ; \
	  $(MAKE) doclibcoda_include ; \
	fi

libcoda_docclean:
	@echo "removing libcoda documentation"
	@rm -rf doc/html/libcoda

doclibcoda_include:
	@find doc/html/libcoda -type f | LANG= sort | $(AWK) 'BEGIN {printf("LIBCODA_DOCFILES =")}; {printf(" \\\n\t%s", $$0)}' > doclibcoda.mk2
	@echo "" >> doclibcoda.mk2
	@find doc/html/libcoda -type f | LANG= sort | $(AWK) '{printf("%s:\n\t$$(MAKE) libcoda_doc\n", $$0)}' >> doclibcoda.mk2
	@if diff $(srcdir)/doclibcoda.mk doclibcoda.mk2 >/dev/null 2>&1 ; then \
	  echo "doclibcoda.mk is unchanged" ; \
	  rm -f doclibcoda.mk2 ; \
	else \
	  mv doclibcoda.mk2 $(srcdir)/doclibcoda.mk ; \
	fi

java_doc: $(java_files) java/doc/Doxyfile
	@if test "x$(DOXYGEN)" = x ; then \
	  echo "Doxygen not available. Documentation was not (re)created." ; \
	else \
	  $(MAKE) java_docclean ; \
	  echo "creating coda java documentation" ; \
	  test -d doc/html || $(MKDIR_P) doc/html ; \
	  cd java/doc ; \
	  $(DOXYGEN) Doxyfile ; \
	  cd ../.. ; \
	  rm -rf doc/html/java/doxygen.* doc/html/java/tab* doc/html/java/dir_*.html ; \
	  echo "creating docjava.mk" ; \
	  $(MAKE) docjava_include ; \
	fi

java_docclean:
	@echo "removing coda java documentation"
	@rm -rf doc/html/java

docjava_include:
	@find doc/html/java -type f | LANG= sort | $(AWK) 'BEGIN {printf("JAVA_DOCFILES =")}; {printf(" \\\n\t%s", $$0)}' > docjava.mk2
	@echo "" >> docjava.mk2
	@find doc/html/java -type f | LANG= sort | $(AWK) '{printf("%s:\n\t$$(MAKE) java_doc\n", $$0)}' >> docjava.mk2
	@if diff $(srcdir)/docjava.mk docjava.mk2 >/dev/null 2>&1 ; then \
	  echo "docjava.mk is unchanged" ; \
	  rm -f docjava.mk2 ; \
	else \
	  mv docjava.mk2 $(srcdir)/docjava.mk ; \
	fi
endif

# *** indent ***

findtypedef_SOURCES = findtypedef.l
findtypedef_LDADD = @LIBOBJS@

EXTRA_DIST += .indent.pro.in

.indent.pro: $(srcdir)/.indent.pro.in findtypedef$(EXEEXT) $(INDENTFILES)
	@echo creating .indent.pro
	@$(RM) indent.types
	@touch indent.types
	@for file in $(INDENTFILES) ; do \
	  ./findtypedef `test -f $$file || echo '$(srcdir)/'`$$file ; \
	done >> indent.types
	@cp $(srcdir)/.indent.pro.in .indent.pro
	@sort -u indent.types | sed "s/^/-T /" >> .indent.pro
	@$(RM) indent.types

indent:
	@$(RM) .indent.pro
	@$(MAKE) .indent.pro
	@echo Indenting
	@test "x$(INDENTFILES)" = x || for file in $(INDENTFILES) ; do \
	  $(INDENT) `test -f $$file || echo '$(srcdir)/'`$$file ; \
	done

# *** CMake-specific ***

EXTRA_DIST += \
	CMakeLists.txt \
	CMakeModules/FindNumPy.cmake \
	CMakeModules/FindHDF4.cmake \
	CMakeModules/FindHDF5.cmake \
	CMakeModules/FindZLIB.cmake \
	CMakeModules/FindJPEG.cmake \
	CMakeModules/FindSZIP.cmake \
	cmakeify-dot-infiles.pl \
	config.h.cmake.in \
	libcoda/coda.h.cmake.in 

config.h.cmake.in: config.h.in cmakeify-dot-infiles.pl
	$(srcdir)/cmakeify-dot-infiles.pl $(srcdir)/config.h.in config.h.cmake.in2
	@if diff $(srcdir)/config.h.cmake.in config.h.cmake.in2 >/dev/null 2>&1 ; then \
	  echo "config.h.cmake.in is unchanged" ; \
	  rm -f config.h.cmake.in2 ; \
	else \
	  mv config.h.cmake.in2 $(srcdir)/config.h.cmake.in ; \
	fi

libcoda/coda.h.cmake.in: libcoda/coda.h.in cmakeify-dot-infiles.pl
	$(srcdir)/cmakeify-dot-infiles.pl $(srcdir)/libcoda/coda.h.in libcoda/coda.h.cmake.in2
	@if diff $(srcdir)/libcoda/coda.h.cmake.in libcoda/coda.h.cmake.in2 >/dev/null 2>&1 ; then \
	  echo "libcoda/coda.h.cmake.in is unchanged" ; \
	  rm -f libcoda/coda.h.cmake.in2 ; \
	else \
	  mv libcoda/coda.h.cmake.in2 $(srcdir)/libcoda/coda.h.cmake.in ; \
	fi

# *** windows-specific ***

EXTRA_DIST += \
	unix2dos \
	windows/clean_dir.bat \
	windows/coda.h \
	windows/codawin32.iss \
	windows/codawin32.iss.in \
	windows/codawin64.iss \
	windows/codawin64.iss.in \
	windows/coda.sln \
	windows/codacheck.vcproj \
	windows/codacmp.vcproj \
	windows/codadd.vcproj \
	windows/codadll.vcproj \
	windows/codadump.vcproj \
	windows/codaeval.vcproj \
	windows/codafind.vcproj \
	windows/codaidl.vcproj \
	windows/codajava.vcproj \
	windows/codalib.vcproj \
	windows/codamatlab.vcproj \
	windows/codapython.vcproj \
	windows/config.h \
	windows/config.h.in \
	windows/create_installer.bat \
	windows/make_coda_all.bat \
	windows/make_coda_idl.bat \
	windows/make_coda_java.bat \
	windows/make_coda_matlab.bat \
	windows/make_coda_python.bat \
	windows/make_coda_libraries.bat \
	windows/make_coda_tools.bat \
	windows/modifypath.iss \
	windows/removepath.iss \
	windows/setup.py \
	windows/setup.py.in \
	windows/build.xml \
	windows/readme.txt

windows/coda.h: $(srcdir)/libcoda/coda.h.in
	cp $(srcdir)/libcoda/coda.h.in windows/coda.h

windows/readme.txt: $(srcdir)/README
	$(srcdir)/unix2dos $(srcdir)/README > windows/readme.txt

windows/build.xml: $(srcdir)/java/build.xml.in
	cat $(srcdir)/java/build.xml.in | sed 's/\@JAVA\@/C:\\jdk1.6.0_07\\bin\\java/' | sed 's/\@JAVAC\@/C:\\jdk1.6.0_07\\bin\\javac/' > windows/build.xml

# *** development install targets to avoid long installs ***

install-python:
	$(MAKE) install-codapythonLTLIBRARIES install-codapythonPYTHON

install-matlab:
	$(MAKE) install-nodist_matlabmexexecDATA

install-idl:
	@$(MAKE) install-idldlmexecLTLIBRARIES install-idldlmexecDATA

# *** cppcheck ***
cppcheck:
	cppcheck -q $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libcoda_la_CPPFLAGS) $(CPPFLAGS) libcoda/*.c libcoda/*.h

