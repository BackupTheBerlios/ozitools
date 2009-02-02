INCLUDES = -I${top_builddir} -I${top_srcdir}

bin_PROGRAMS = ozf2tiff
ozf2tiff_SOURCES = ozf2tiff.c

bin_SCRIPTS = map2geotiff
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = map2geotiff.in.pl

do_subst = $(SED) -e 's,[@]PERL[@],$(PERL),g' \
	-e 's,[@]GDAL_TRANSLATE[@],$(GDAL_TRANSLATE),g' \
	-e 's,[@]GDALWARP[@],$(GDALWARP),g' \
	-e 's,[@]BINDIR[@],$(bindir),g'

map2geotiff: map2geotiff.in.pl Makefile
	$(do_subst) < ${srcdir}/map2geotiff.in.pl > ${builddir}/map2geotiff
	$(CHMOD) 775 ${builddir}/map2geotiff
	