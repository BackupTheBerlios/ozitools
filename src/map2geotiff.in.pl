#!@PERL@ -w

################################################################################
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
##
## Mikhail Rumyantsev <dev@beonway.ru>
##
################################################################################

use Getopt::Long;
use Geo::OSR;
use Geo::GDAL;

%opts = ();

Getopt::Long::Configure("bundling");
GetOptions(\%opts, 'lzw|l',
	'tiled|t',
	'output-wgs84|w',
	'output-reproject|p',
	'tile-size-x=i',
	'tile-size-y=i',
	'use-epsg|e');

$projData = new ProjData (correct => $opts{'use-epsg'}, wgs84 => $opts{'output-wgs84'});
$projData->init();

exit(main(@ARGV));

sub main {
	my $file = $ARGV[0];
	
	print "file is " . $file . "\n";
	
	my $mapData = MapData::parse($file);

	#dump_map($mapData);
		
	$mapData = to_latlong($mapData);

	#dump_map($mapData);
	
	$mapData = prepare_raster($mapData);

	#dump_map($mapData);

	$mapData = to_tagged_gtiff($mapData);
	$mapData = warp($mapData);
}

sub dump_map {
	my $mapData = shift;

	$mapData->dump;
	print "\n";
	
	foreach $p (@{$mapData->points}) {
		print $p->dump;
		print "\n";
	}
}

#sub map_proj_data {
#	my $map = shift;
#
#	my $datum = $datumParams{$map->{KEY_MAP_DATUM}};
#	die "Unknown datum " .  $map->{KEY_MAP_DATUM} unless $datum;
#	
#	my $projData = $map->{KEY_PROJECTION};
#	my $proj = $projParams{$projData->{KEY_PROJECTION_NAME}};
#	die "Unknown projection " .  $projData->{KEY_PROJECTION_NAME} unless $proj;
#
#	$proj = sprintf($proj,
#		$projData->{KEY_PROJECTION_LAT_ORIGIN},
#		$projData->{KEY_PROJECTION_LON_ORIGIN},
#		$projData->{KEY_PROJECTION_SCALE_FACTOR},
#		$projData->{KEY_PROJECTION_FALSE_EAST},
#		$projData->{KEY_PROJECTION_FALSE_NORTH},
#		$projData->{KEY_PROJECTION_LAT1},
#		$projData->{KEY_PROJECTION_LAT2});
#		
#	return ($datum, $proj);
#}

sub to_latlong {
	my $map = shift;
	
	my $from = new Geo::OSR::SpatialReference();
	$from->ImportFromProj4($projData->projected($map));

	my $to = new Geo::OSR::SpatialReference();
	$to->ImportFromProj4($projData->latlong($map));

	foreach $p (@{$map->points}) {
		# Again we assume we have both lat/long values
		# if lat is defined.
		next if defined($p->getLat);
		
		$from->setUTM($p->getUTMZone,
			($p-getUTMHemisphere eq "S") ? 0 : 1)
				if defined($p->getUTMZone);

		my $trans = new Geo::OSR::CoordinateTransformation($from, $to);
		
		my $coords = $trans->TransformPoint(
			$p->getGridX,
			$p->getGridY,
			0);
	
		$p->setGCoords($coords);
	}

	return $map;	
}

sub prepare_raster {
	my $map = shift;
	
	die "Map raster file name is not defined!" unless $map->file;
	
	my $filename = $map->filename;
	my $ext = $map->fileext;
	
	die "ozf3 is not supported!" if $ext =~ /^ozf3/;
	
	if ($ext eq "ozf2") {
		my @args = ("@BINDIR@/ozf2tiff", $filename . "." . $ext, $filename . ".tif");
		system(@args);
		$ext = "tif";	
	}
	
	$map->setFileext($ext);
	
	my $data = Geo::GDAL::Dataset::Open($map->file);
	die "Unable to open map raster data" unless $data;

	my ($width, $height) = $data->Size();
	$map->setWidth($width);
	$map->setHeight($height);
	
	return $map;
}

sub to_tagged_gtiff {
	my $map = shift;
	
	my @args = ("@GDAL_TRANSLATE@", "-of", "GTiff");

	my $a_srs = $projData->latlong($map);
	
	push @args, "-a_srs";
	push @args, $a_srs;

	foreach $p (@{$map->points}) {
		push @args, "-gcp";

		push @args, $p->getX;
		push @args, $p->getY;
		push @args, $p->getLon;
		push @args, $p->getLat;
	}

	push @args, $map->file;
	push @args, $map->filename . ".gtf";
	
	system(@args);
	
	$map->setFileext("gtf");

	return $map;	
}

sub warp {
	my $map = shift;
	
	my @args = ("@GDALWARP@", "-of", "GTiff");

	push @args, ("-co", "TILED=YES") if exists($opts{'tiled'});
	push @args, ("-co", "COMPRESS=LZW") if exists($opts{'lzw'});
	push @args, ("-co", "BLOCKXSIZE=". $opts{'tile-size-x'}) if exists($opts{'tile-size-x'});
	push @args, ("-co", "BLOCKYSIZE=". $opts{'tile-size-y'}) if exists($opts{'tile-size-y'});

	push @args, "-tps";
	
	push @args, "-ts";
	push @args, $map->width;
	push @args, $map->height;

	my $s_srs = $projData->latlong($map);
	
	my $t_srs = exists($opts{"output-reproject"}) ? $projData->projected($map) : $projData->latlong($map);
	
	push @args, "-s_srs";
	push @args, $s_srs;
	push @args, "-t_srs";
	push @args, $t_srs;


	push @args, $map->file;
	push @args, $map->filename . ".gtiff";
	
	system(@args);
	
	$map->setFileext("gtiff");

	return $map;	
}

package Point;

use constant KEY_POINT_X => "pointX";
use constant KEY_POINT_Y => "pointY";
use constant KEY_POINT_LAT => "pointLat";
use constant KEY_POINT_LON => "pointLon";
use constant KEY_POINT_GRID_X => "pointGridX";
use constant KEY_POINT_GRID_Y => "pointGridY";
use constant KEY_POINT_UTM_ZONE => "pointUTMZone";
use constant KEY_POINT_UTM_HEMISPHERE => "pointUTMHem";

sub parse {
	my $recs = shift;
	my $self = {};
	
	$self->{KEY_POINT_X} = $recs->[2] if ($recs->[2]);
	$self->{KEY_POINT_Y} = $recs->[3] if ($recs->[3]);
	$self->{KEY_POINT_LAT} = 
			($recs->[6] + $recs->[7] / 60) * 
			(($recs->[8] eq "S") ? -1 : 1) if ($recs->[6]);
	$self->{KEY_POINT_LON} = 
			$recs->[9] + $recs->[10] / 60 * 
			(($recs->[11] eq "W") ? -1 : 1) if ($recs->[9]);
	$self->{KEY_POINT_GRID_X} = $recs->[14] if ($recs->[14]);
	$self->{KEY_POINT_GRID_Y} = $recs->[15] if ($recs->[15]);
	$self->{KEY_POINT_UTM_ZONE} = $recs->[13] if ($recs->[13]);
	$self->{KEY_POINT_UTM_HEMISPHERE} = $recs->[16] if ($recs->[16]);
	
	return bless $self;
}

sub dump {
	my $self = shift;

	print "Point start\n";
	print "\tX = \t\t" . $self->getX . "\n";	
	print "\tY = \t\t" . $self->getY . "\n";	
	print "\tLat = \t\t" . $self->getLat . "\n";	
	print "\tLon = \t\t" . $self->getLon . "\n";	
	print "\tGridX = \t\t" . $self->getGridX . "\n";	
	print "\tGridY = \t\t" . $self->getGridY . "\n";	
	print "\tUTM Zone = \t\t" . $self->getUTMZone . "\n";	
	print "\tUTM Hemisph = \t\t" . $self->getUTMHemisphere . "\n";	
		
	print "Point end\n";	
}

sub getX {
	my $self = shift;
	return $self->{KEY_POINT_X};	
}

sub getY {
	my $self = shift;
	return $self->{KEY_POINT_Y};	
}

sub getLat {
	my $self = shift;
	return $self->{KEY_POINT_LAT};	
}

sub getLon {
	my $self = shift;
	return $self->{KEY_POINT_LON};	
}

sub setGCoords {
	my $self = shift;
	my $coords = shift;
			
	$self->{KEY_POINT_LAT} = $coords->[0];
	$self->{KEY_POINT_LON} = $coords->[1];
}

sub getGridX {
	my $self = shift;
	return $self->{KEY_POINT_GRID_X};	
}

sub getGridY {
	my $self = shift;
	return $self->{KEY_POINT_GRID_Y};	
}

sub getUTMZone {
	my $self = shift;
	return $self->{KEY_POINT_UTM_ZONE};	
}

sub getUTMHemisphere {
	my $self = shift;
	return $self->{KEY_POINT_UTM_HEMISPHERE};	
}

package MapData;

use constant KEY_MAP_NAME => "name";
use constant KEY_MAP_FILE => "file";
use constant KEY_MAP_FILE_NAME => "filename";
use constant KEY_MAP_FILE_EXT => "fileext";
use constant KEY_MAP_RASTER_WIDTH => "rasterwidth";
use constant KEY_MAP_RASTER_HEIGHT => "rasterheight";
use constant KEY_MAP_DATUM => "datum";
use constant KEY_PROJECTION_NAME => "projName";
use constant KEY_PROJECTION => "proj";
use constant KEY_POINTS => "points";

sub parse {
	my $fname = shift;

	my $self = bless {};
	
	my $strCount = 0;
	my @mapPoints;

	open(DAT, $fname) || die("Could not open .map file " . $fname);
	while(<DAT>)
	{
		$strCount += 1;
		chomp;
		s/^\s*(.*?)\s*$/$1/g;
		my @recs = Util::trim(split(/,/, $_, -1));
		
		$strCount == 2 && (
			$self->{KEY_MAP_NAME} = $_, 
			next
		); 
		$strCount == 3 && (
			$self->setFile($_),
			next
		); 
		$strCount == 5 && $recs[0] && (
			$self->{KEY_MAP_DATUM} = $recs[0],
			next
		); 
		$strCount == 9 && $recs[1] && (
			$self->{KEY_PROJECTION_NAME} = $recs[1],
			next
		); 
		$strCount >= 10 && $strCount <= 39 && $recs[2] && (
			push(@mapPoints, Point::parse(\@recs)),
			next
		); 
		$strCount == 40 && (
			$self->{KEY_PROJECTION} = join(',', Util::trim(split(/,/, $_, -1))),
			next
		); 
	}
	close(DAT);
	
	$self->{KEY_POINTS} = \@mapPoints;

	return $self;		
}

sub dump {
	my $self = shift;

	print "Map\n";
	# TODO: implement.		
}

sub name {
	my $self = shift;
	return $self->{KEY_MAP_NAME};
}

sub file {
	my $self = shift;
	return $self->{KEY_MAP_FILE_NAME} . "." . $self->{KEY_MAP_FILE_EXT};
}

sub setFile {
	my $self = shift;
	my $filename = shift;
	my $ext = $filename;
	
	$filename =~ s/^(.*[\\\/])?(.*)\..*$/$2/g;
	$ext =~ s/^.*\.//g;
	$ext =~ tr/A-Z/a-z/;
	
	$self->{KEY_MAP_FILE_NAME} = $filename;
	$self->{KEY_MAP_FILE_EXT} = $ext;
}

sub filename {
	my $self = shift;
	return $self->{KEY_MAP_FILE_NAME};
}

sub fileext {
	my $self = shift;
	return $self->{KEY_MAP_FILE_EXT};
}

sub setFilename {
	my $self = shift;
	my $p = shift;
	$self->{KEY_MAP_FILE_NAME} = $p;
}

sub setFileext {
	my $self = shift;
	my $p = shift;
	$self->{KEY_MAP_FILE_EXT} = $p;
}

sub width {
	my $self = shift;
	return $self->{KEY_MAP_RASTER_WIDTH};
}

sub height {
	my $self = shift;
	return $self->{KEY_MAP_RASTER_WIDTH};
}

sub setWidth {
	my $self = shift;
	my $p = shift;
	$self->{KEY_MAP_RASTER_WIDTH} = $p;
}

sub setHeight {
	my $self = shift;
	my $p = shift;
	$self->{KEY_MAP_RASTER_WIDTH} = $p;
}

sub datum {
	my $self = shift;
	return $self->{KEY_MAP_DATUM};
}

sub projection {
	my $self = shift;
	return $self->{KEY_PROJECTION_NAME};
}

sub projParams {
	my $self = shift;
	return $self->{KEY_PROJECTION};
}

sub points {
	my $self = shift;
	return $self->{KEY_POINTS};
}

package ProjData;

%datums = ();
%projs = ();

sub new {
	my $class = shift;

	my %opts = (@_);

	my $self = {};
	$self->{"opts"} = \%opts;
	return bless $self, $class;
}

sub init {
	my $self = shift;
	
	my $dext = $self->{"opts"}->{"correct"} ? "corrected" : "ozi";
	
	open(DAT, "@pkgdatadir@/datum_$dext.dat") || die("Failed to load datum mappings @pkgdatadir@/datum_$dext.dat\n");
	while(<DAT>) {
		my($k, $v) = split(/:/, $_);
		$datums{$k} = $v;
	}
	close(DAT);

	open(DAT, "@pkgdatadir@/proj.dat") || die("Failed to load projection mappings @pkgdatadir@/proj.dat\n");
	while(<DAT>) {
		my($k,$n,$v) = split(/:/, $_);
		$projs{$k} = $v;
	}
	close(DAT);
}

sub latlong {
	my $self = shift;
	my $map = shift;
	
	my $datum = $self->{"opts"}->{"wgs84"} ? "+ellps=WGS84 +datum=WGS84" : $datums{$map->datum};
	die "Unknown or unsupported datum " .  $map->datum unless $datum;

	return "+proj=latlong " . $datum . " +no_defs";		
}

sub projected {
	my $self = shift;
	my $map = shift;
	
	my $proj = $projs{$map->projection};
	die "Unknown or unsupported projection " .  $map->projection unless $proj;

	if ($proj =~ /\+no_defs\s*$/) {
		# It is grid coordinate system with fixed params and datum included.
		return $proj;
	}
	$proj = '"' . $proj . '"';	
	
	my $datum = $self->{"opts"}->{"wgs84"} ? "+ellps=WGS84 +datum=WGS84" : $datums{$map->datum};
	die "Unknown or unsupported datum " .  $map->datum unless $datum;
	
	my $t = join(',', Util::parenthize(Util::trim(split(/,/, $map->projParams, -1))));
	my $p = join(',', Util::trim(split(/,/, $map->projParams, -1)));
	$p =~ s/^${t}$/$proj/ee;
	
	return $p . " " . $datum . " +no_defs";	
}

package Util;

sub trim {
	my @ret;
	while (@_) {
		my $s = shift;
		$s =~ s/^\s*(.*?)\s*$/$1/g if ($s);
		push @ret, $s;
	}
	return @ret;
}

sub parenthize {
	my @ret;
	while (@_) {
		my $s = shift;
		$s =~ s/^(.*)$/\($1\)/g if ($s);
		push @ret, $s;
	}
	return @ret;
}
