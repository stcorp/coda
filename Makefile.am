ACLOCAL_AMFLAGS = -I m4

.PHONY: doc

# *** general build settings ****

DOXYGEN = @DOXYGEN@
INDENT = @INDENT@

DISOPT_FLAG = @DISOPT_FLAG@
SWIGFLAGS = -w119,451 -Ipython -I$(srcdir)/python -DPRINTF_ATTR= @SWIGFLAGS@
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

ALL_LOCAL_TARGETS =
all-local: $(ALL_LOCAL_TARGETS)

# *** targets ***

if SUBPACKAGE_MODE

noinst_PROGRAMS = codadd
noinst_LTLIBRARIES = libcoda_internal.la libexpat_internal.la libz_internal.la

else

# programs

bin_PROGRAMS = codacheck codacmp codadump codafind codadd
noinst_PROGRAMS = generate-finc findtypedef

# libraries (+ related files)

lib_LTLIBRARIES = libcoda.la
noinst_LTLIBRARIES = libcoda_internal.la libexpat_internal.la libz_internal.la

if BUILD_IDL
idldlmexec_LTLIBRARIES = coda-idl.la
endif

if BUILD_MATLAB
ALL_LOCAL_TARGETS += coda_matlab.@MEXEXT@
nodist_matlabmexexec_DATA = coda_matlab.@MEXEXT@
endif

if BUILD_PYTHON
codapython_PYTHON = \
	$(srcdir)/python/__init__.py \
	$(srcdir)/python/codapython.py \
	python/codac.py
codapyexec_LTLIBRARIES = _codac.la
endif

# empty dir for .codadef definition files

definitions_DATA =

# headers

nodist_include_HEADERS = coda.h

fortran_DATA = \
	fortran/coda.inc
endif

# *** SWIG dependencies ***

@AMDEP_TRUE@DEP_FILES = ./$(DEPDIR)/codac.Pc
@AMDEP_TRUE@@am__include@ @am__quote@./$(DEPDIR)/codac.Pc@am__quote@

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
EXTRA_DIST += libcoda/expat/xmltok_impl.c libcoda/expat/xmltok_ns.c
EXTRA_DIST += libcoda/expat/COPYING libcoda/expat/README

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
EXTRA_DIST += libcoda/zlib/COPYING libcoda/zlib/README

# libcoda

libcoda_la_SOURCES = \
	libcoda/coda-ascbin-cursor.c \
	libcoda/coda-ascbin-definition.c \
	libcoda/coda-ascbin-definition.h \
	libcoda/coda-ascbin-internal.h \
	libcoda/coda-ascbin-type.c \
	libcoda/coda-ascbin.c \
	libcoda/coda-ascbin.h \
	libcoda/coda-ascii-cursor.c \
	libcoda/coda-ascii-definition.c \
	libcoda/coda-ascii-definition.h \
	libcoda/coda-ascii-internal.h \
	libcoda/coda-ascii-type.c \
	libcoda/coda-ascii.c \
	libcoda/coda-ascii.h \
	libcoda/coda-bin-cursor.c \
	libcoda/coda-bin-definition.c \
	libcoda/coda-bin-definition.h \
	libcoda/coda-bin-internal.h \
	libcoda/coda-bin-type.c \
	libcoda/coda-bin.c \
	libcoda/coda-bin.h \
	libcoda/coda-cursor.c \
	libcoda/coda-definition-parse.c \
	libcoda/coda-definition.c \
	libcoda/coda-definition.h \
	libcoda/coda-errno.c \
	libcoda/coda-expr-internal.h \
	libcoda/coda-expr-parser.y \
	libcoda/coda-expr-tokenizer.l \
	libcoda/coda-expr.c \
	libcoda/coda-expr.h \
	libcoda/coda-filefilter.c \
	libcoda/coda-filefilter.h \
	libcoda/coda-internal.h \
	libcoda/coda-netcdf-cursor.c \
	libcoda/coda-netcdf-dynamic.c \
	libcoda/coda-netcdf-internal.h \
	libcoda/coda-netcdf-type.c \
	libcoda/coda-netcdf.c \
	libcoda/coda-netcdf.h \
	libcoda/coda-path.h \
	libcoda/coda-product.c \
	libcoda/coda-type.c \
	libcoda/coda-utils.c \
	libcoda/coda-xml-cursor.c \
	libcoda/coda-xml-definition.c \
	libcoda/coda-xml-definition.h \
	libcoda/coda-xml-dynamic.c \
	libcoda/coda-xml-dynamic.h \
	libcoda/coda-xml-internal.h \
	libcoda/coda-xml-parser.c \
	libcoda/coda-xml-type.c \
	libcoda/coda-xml.c \
	libcoda/coda-xml.h \
	libcoda/coda.c \
	libcoda/hashtable.c \
	libcoda/hashtable.h \
	libcoda/ziparchive.c \
	libcoda/ziparchive.h
libcoda_hdf4_files = \
	libcoda/coda-hdf4-cursor.c \
	libcoda/coda-hdf4-definition.c \
	libcoda/coda-hdf4-internal.h \
	libcoda/coda-hdf4-type.c \
	libcoda/coda-hdf4.h
libcoda_hdf5_files = \
	libcoda/coda-hdf5-cursor.c \
	libcoda/coda-hdf5-definition.c \
	libcoda/coda-hdf5-internal.h \
	libcoda/coda-hdf5-type.c \
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
libcoda_la_CPPFLAGS = -Ilibcoda/expat -I$(srcdir)/libcoda/expat -Ilibcoda/zlib -I$(srcdir)/libcoda/zlib $(AM_CPPFLAGS)
libcoda_la_LDFLAGS = -no-undefined -version-info $(LIBCODA_CURRENT):$(LIBCODA_REVISION):$(LIBCODA_AGE)
libcoda_la_LIBADD = @LTLIBOBJS@ libexpat_internal.la libz_internal.la $(HDF4LIBS) $(HDF5LIBS)
INDENTFILES += $(libcoda_la_SOURCES) libcoda/coda.h.in
BUILT_SOURCES += coda-expr-parser.h

libcoda_internal_la_SOURCES = \
	libcoda/coda-ascbin-cursor.c \
	libcoda/coda-ascbin-definition.c \
	libcoda/coda-ascbin-definition.h \
	libcoda/coda-ascbin-internal.h \
	libcoda/coda-ascbin-type.c \
	libcoda/coda-ascbin.c \
	libcoda/coda-ascbin.h \
	libcoda/coda-ascii-cursor.c \
	libcoda/coda-ascii-definition.c \
	libcoda/coda-ascii-definition.h \
	libcoda/coda-ascii-internal.h \
	libcoda/coda-ascii-type.c \
	libcoda/coda-ascii.c \
	libcoda/coda-ascii.h \
	libcoda/coda-bin-cursor.c \
	libcoda/coda-bin-definition.c \
	libcoda/coda-bin-definition.h \
	libcoda/coda-bin-internal.h \
	libcoda/coda-bin-type.c \
	libcoda/coda-bin.c \
	libcoda/coda-bin.h \
	libcoda/coda-cursor.c \
	libcoda/coda-definition-parse.c \
	libcoda/coda-definition.c \
	libcoda/coda-definition.h \
	libcoda/coda-errno.c \
	libcoda/coda-expr-internal.h \
	coda-expr-parser.c \
	coda-expr-parser.h \
	libcoda/coda-expr-tokenizer.l \
	libcoda/coda-expr.c \
	libcoda/coda-expr.h \
	libcoda/coda-filefilter.c \
	libcoda/coda-filefilter.h \
	libcoda/coda-internal.h \
	libcoda/coda-netcdf-cursor.c \
	libcoda/coda-netcdf-dynamic.c \
	libcoda/coda-netcdf-internal.h \
	libcoda/coda-netcdf-type.c \
	libcoda/coda-netcdf.c \
	libcoda/coda-netcdf.h \
	libcoda/coda-path.h \
	libcoda/coda-product.c \
	libcoda/coda-type.c \
	libcoda/coda-utils.c \
	libcoda/coda-xml-cursor.c \
	libcoda/coda-xml-definition.c \
	libcoda/coda-xml-definition.h \
	libcoda/coda-xml-dynamic.c \
	libcoda/coda-xml-dynamic.h \
	libcoda/coda-xml-internal.h \
	libcoda/coda-xml-parser.c \
	libcoda/coda-xml-type.c \
	libcoda/coda-xml.c \
	libcoda/coda-xml.h \
	libcoda/coda.c \
	libcoda/hashtable.c \
	libcoda/hashtable.h \
	libcoda/ziparchive.c \
	libcoda/ziparchive.h
if WITH_HDF4
libcoda_internal_la_SOURCES += $(libcoda_hdf4_files)
endif
if WITH_HDF5
libcoda_internal_la_SOURCES += $(libcoda_hdf5_files)
endif
libcoda_internal_la_CPPFLAGS = -Ilibcoda/expat -I$(srcdir)/libcoda/expat -Ilibcoda/zlib -I$(srcdir)/libcoda/zlib $(AM_CPPFLAGS)
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

# tools/codadump

codadump_SOURCES = \
	tools/codadump/codadump-ascii.c \
	tools/codadump/codadump-debug.c \
	tools/codadump/codadump-dim.c \
	tools/codadump/codadump-filter.c \
	tools/codadump/codadump-traverse.c \
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

# tools/codafind

codafind_SOURCES = \
	tools/codafind/codafind.c
codafind_LDFLAGS = -static
codafind_LDADD = libcoda.la
INDENTFILES += $(codafind_SOURCES)

# tools/codadd

codadd_SOURCES = \
	tools/codadd/codadd.c \
	tools/codadd/codadd-doc.c \
	tools/codadd/codadd-list.c
codadd_LDFLAGS = -static
codadd_LDADD = libcoda_internal.la
INDENTFILES += $(codadd_SOURCES)

# fortran

if !SUBPACKAGE_MODE

fortran_DATA += \
	fortran/Makefile \
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

if BUILD_IDL
coda_idl_la_SOURCES = idl/coda-idl.c
coda_idl_la_LDFLAGS = -module -shared -avoid-version
coda_idl_la_LIBADD = libcoda.la
coda_idl_la_DEPENDENCIES = libcoda.la
INDENTFILES += $(coda_idl_la_SOURCES)
endif

# matlab

CODA_MATLAB_SOURCE_FILES = \
	$(srcdir)/matlab/coda-matlab.h \
	$(srcdir)/matlab/coda-matlab-getdata.c \
	$(srcdir)/matlab/coda-matlab-traverse.c \
	$(srcdir)/matlab/coda_matlab.c \
	$(srcdir)/matlab/mxCreateDoubleScalar.c \
	$(srcdir)/matlab/mxCreateNumericMatrix.c
EXTRA_DIST += $(CODA_MATLAB_SOURCE_FILES)
INDENTFILES += $(CODA_MATLAB_SOURCE_FILES)

if BUILD_MATLAB
MEXCOMPILE = $(MEX) $(DEFS) $(DEFAULT_INCLUDES) $(AM_CPPFLAGS) $(MEXFLAGS)

CODA_MATLAB_OBJECTS = coda-matlab-getdata.o coda-matlab-traverse.o @MATLAB_LIBOBJS@

CLEANFILES += coda_matlab.@MEXEXT@ $(CODA_MATLAB_OBJECTS)

coda-matlab-getdata.o: $(srcdir)/matlab/coda-matlab-getdata.c $(srcdir)/matlab/coda-matlab.h coda.h config.h
	$(MEXCOMPILE) -c $(srcdir)/matlab/coda-matlab-getdata.c

coda-matlab-traverse.o: $(srcdir)/matlab/coda-matlab-traverse.c $(srcdir)/matlab/coda-matlab.h coda.h config.h
	$(MEXCOMPILE) -c $(srcdir)/matlab/coda-matlab-traverse.c

mxCreateDoubleScalar.o: $(srcdir)/matlab/mxCreateDoubleScalar.c
	$(MEXCOMPILE) -c $(srcdir)/matlab/mxCreateDoubleScalar.c

mxCreateNumericMatrix.o: $(srcdir)/matlab/mxCreateNumericMatrix.c
	$(MEXCOMPILE) -c $(srcdir)/matlab/mxCreateNumericMatrix.c

coda_matlab.@MEXEXT@: $(srcdir)/matlab/coda_matlab.c $(CODA_MATLAB_OBJECTS) libcoda_internal.la $(srcdir)/matlab/coda-matlab.h coda.h config.h
	$(MEXCOMPILE) $(LDFLAGS) $(srcdir)/matlab/coda_matlab.c $(CODA_MATLAB_OBJECTS) .libs/libcoda_internal.a $(HDF4LIBS) $(HDF5LIBS) $(LIBS)
endif

# python

python/codac.c: libcoda/coda.h.in $(srcdir)/python/codac.i \
    $(srcdir)/python/coda_typemaps.i \
    $(srcdir)/python/coda_ignore.i \
    $(srcdir)/python/coda_rename.i 
	@$(mkinstalldirs) python
@AMDEP_TRUE@	source='$(srcdir)/python/codac.i' object='python/codac.c' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@	depfile='$(DEPDIR)/codac.Pc' tmpdepfile='$(DEPDIR)/codac.TPc' @AMDEPBACKSLASH@
@AMDEP_TRUE@	depmode='none' $(depcomp) @AMDEPBACKSLASH@
@AMDEP_TRUE@	$(SWIG) -python $(SWIGFLAGS) -M $(srcdir)/python/codac.i > $(DEPDIR)/codac.Pc
	$(SWIG) -python $(SWIGFLAGS) -o python/codac.c $(srcdir)/python/codac.i

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

if !SUBPACKAGE_MODE
if BUILD_IDL
idldlmexec_DATA = $(CODA_IDL_DLMFILES)
endif
endif

# *** matlab data ***

CODA_MATLAB_MFILES = \
	matlab/coda_attributes.m \
	matlab/coda_class.m \
	matlab/coda_clearall.m \
	matlab/coda_close.m \
	matlab/coda_description.m \
	matlab/coda_fetch.m \
	matlab/coda_fieldavailable.m \
	matlab/coda_fieldcount.m \
	matlab/coda_fieldnames.m \
	matlab/coda_getopt.m \
	matlab/coda_open.m \
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

if !SUBPACKAGE_MODE
if BUILD_MATLAB
matlabmexexec_DATA = $(CODA_MATLAB_MFILES)
endif
endif

# *** documentation ***

include doclibcoda.mk

DOCFILES = \
	doc/html/index.html \
	doc/html/css/codadef.css \
	doc/html/css/codadoc.css \
	$(LIBCODA_DOCFILES) \
	doc/html/codadef/codadef-expressions.html \
	doc/html/codadef/codadef-format.html \
	doc/html/codadef/codadef-hdf4.html \
	doc/html/codadef/codadef-hdf5.html \
	doc/html/codadef/codadef-xml.html \
	doc/html/coda-fortran/index.html \
	doc/html/coda-idl/index.html \
	doc/html/coda-matlab/index.html \
	doc/html/coda-python/index.html \
	doc/html/codacheck/index.html \
	doc/html/codacmp/index.html \
	doc/html/codadd/index.html \
	doc/html/codadump/index.html \
	doc/html/codafind/index.html

EXTRA_DIST += \
	$(DOCFILES) \
	libcoda/doc/Doxyfile.in \
	libcoda/doc/extrapages \
	libcoda/doc/libcoda_footer.html \
	libcoda/doc/libcoda_header.html \
	doclibcoda.mk

if !SUBPACKAGE_MODE
nobase_pkgdata_DATA = $(DOCFILES)
endif

doc: libcoda_doc

docclean: libcoda_docclean

libcoda_doc: coda.h libcoda/doc/Doxyfile
	@if test "x$(DOXYGEN)" = x ; then \
	  echo "Doxygen not available. Documentation was not (re)created." ; \
	else \
	  $(MAKE) libcoda_docclean ; \
	  echo "creating libcoda documentation" ; \
	  test -d doc/html || $(mkinstalldirs) doc/html ; \
	  cd libcoda/doc ; \
	  $(DOXYGEN) Doxyfile ; \
	  cd ../.. ; \
	  rm -rf doc/html/libcoda/doxygen.* doc/html/libcoda/pages.html ; \
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
	CMakeModules/CheckTypeExists.cmake \
	CMakeModules/FindPython.cmake \
	CMakeModules/FindNumPy.cmake \
	CMakeModules/FindHDF4.cmake \
	CMakeModules/FindHDF5.cmake \
	CMakeModules/FindZLIB.cmake \
	CMakeModules/FindJPEG.cmake \
	CMakeModules/FindSZIP.cmake \
	cmakeify-dot-infiles.pl \
	config.h.cmake.in \
	libcoda/coda.h.cmake.in 

# *** windows-specific ***

EXTRA_DIST += \
	unix2dos \
	win32/ModifyPath.iss \
	win32/coda.dsw \
	win32/coda.h \
	win32/coda.iss \
	win32/coda.iss.in \
	win32/coda-idl.def \
	win32/coda_idl.bat \
	win32/coda_matlab.bat \
	win32/codadll.dsp \
	win32/codalib.dsp \
	win32/codapython.dsp \
	win32/config.h \
	win32/config.h.in \
	win32/makedist.bat \
	win32/codacheck.dsp \
	win32/codacmp.dsp \
	win32/codadump.dsp \
	win32/codafind.dsp \
	win32/codadd.dsp \
	win32/removepath.iss \
	win32/setup.py \
	win32/setup.py.in \
	win32/readme.txt

win32/coda.h: $(srcdir)/libcoda/coda.h.in
	cp $(srcdir)/libcoda/coda.h.in win32/coda.h

win32/readme.txt: $(srcdir)/README
	$(srcdir)/unix2dos $(srcdir)/README > win32/readme.txt

# *** development install targets to avoid long installs ***

install-python:
	$(MAKE) install-codapyexecLTLIBRARIES install-codapythonPYTHON

install-matlab:
	$(MAKE) install-nodist_matlabmexexecDATA

install-idl:
	@$(MAKE) install-idldlmexecLTLIBRARIES install-idldlmexecDATA
