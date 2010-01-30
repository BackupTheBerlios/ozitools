AM_CPPFLAGS = -I${top_builddir} -I${top_srcdir}

bin_PROGRAMS = ozf2tiff
ozf2tiff_SOURCES = ozf2tiff.c

bin_SCRIPTS = map2geotiff
CLEANFILES = $(bin_SCRIPTS) map2geotiff.pl map2geotiff.tmp
EXTRA_DIST = map2geotiff.pl.in

do_subst = $(SED) \
	-e 's|@PKGDATADIR[@]|$(pkgdatadir)|g' \
	-e 's|@BINDIR[@]|$(bindir)|g'

#map2geotiff: map2geotiff.in.pl Makefile
#	$(do_subst) < ${srcdir}/map2geotiff.in.pl > ${builddir}/map2geotiff
#	$(CHMOD) 775 ${builddir}/map2geotiff

map2geotiff: map2geotiff.pl Makefile
	$(RM) -f $@ $@.tmp
	$(do_subst) < $@.pl > $@.tmp
	$(CP) $@.tmp $@
	$(CHMOD) 775 $@
	