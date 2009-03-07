otmandir = $(mandir)/man1

otman_DATA = ozf2tiff.1.gz map2geotiff.1.gz
CLEANFILES = $(otman_DATA)
EXTRA_DIST = ozf2tiff.1 map2geotiff.1

SUFFIXES = .1 .1.gz

.1.1.gz:
	$(GZIP) -c < $< > $@
