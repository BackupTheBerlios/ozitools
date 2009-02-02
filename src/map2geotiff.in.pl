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

use constant KEY_MAP_NAME => "name";
use constant KEY_MAP_FILE => "file";
use constant KEY_MAP_FILE_NAME => "filename";
use constant KEY_MAP_FILE_EXT => "fileext";
use constant KEY_MAP_RASTER_WIDTH => "rasterwidth";
use constant KEY_MAP_RASTER_HEIGHT => "rasterheight";
use constant KEY_MAP_DATUM => "datum";
use constant KEY_PROJECTION => "proj";
use constant KEY_PROJECTION_NAME => "projName";
use constant KEY_PROJECTION_LAT_ORIGIN => "projLatOrigin";
use constant KEY_PROJECTION_LON_ORIGIN => "projLonOrigin";
use constant KEY_PROJECTION_SCALE_FACTOR => "projScaleFactor";
use constant KEY_PROJECTION_FALSE_EAST => "projFalseEast";
use constant KEY_PROJECTION_FALSE_NORTH => "projFalseNorth";
use constant KEY_PROJECTION_LAT1 => "projLat1";
use constant KEY_PROJECTION_LAT2 => "projLat2";
use constant KEY_POINTS => "points";
use constant KEY_POINT_X => "pointX";
use constant KEY_POINT_Y => "pointY";
use constant KEY_POINT_LAT => "pointLat";
use constant KEY_POINT_LON => "pointLon";
use constant KEY_POINT_GRID_X => "pointGridX";
use constant KEY_POINT_GRID_Y => "pointGridY";
use constant KEY_POINT_UTM_ZONE => "pointUTMZone";
use constant KEY_POINT_UTM_HEMISPHERE => "pointUTMHem";

%datumParams = (
	'Pulkovo 1942 (1)' => 
		'+ellps=krass +towgs84=23.92,-141.27,-80.9,0,-0.35,-0.82,-0.12',
	'Pulkovo 1942 (2)' => 
		'+ellps=krass +towgs84=23.92,-141.27,-80.9,0,-0.37,-0.85,-0.12',
	'Pulkovo 1942(58)' => 
		'+ellps=krass +towgs84=33.4,-146.6,-76.3,-0.359,-0.053,0.844,-0.84',
	'Pulkovo 1942(83)' => 
		'+ellps=krass +towgs84=24,-123,-94,0.02,-0.25,-0.13,1.1',
	'WGS 84' => 
		'+ellps=WGS84 +datum=WGS84'
);

%projParams = (
	'Transverse Mercator' => 
		'+proj=tmerc +lat_0=%f +lon_0=%f +k=%f +x_0=%f +y_0=%f%.0s%.0s',
	'(UTM) Universal Transverse Mercator' => 
		'+proj=utm',
	'Latitude/Longitude' => 
		'+proj=latlong'
	
);

exit(main(@ARGV));

sub trim {
	my @ret;
	while (@_) {
		my $s = shift;
		$s =~ s/^\s*(.*?)\s*$/$1/g if ($s);
		push @ret, $s;
	}
	return @ret;
}

sub parse_map {
	my $fname = shift;
	
	my $strCount = 0;
	my %mapData;
	my %projectionData;
	my @mapPoints;

	open(DAT, $fname) || die("Could not open .map file " . $fname);
	while(<DAT>)
	{
		$strCount += 1;
		chomp;
		s/^\s*(.*?)\s*$/$1/g;
		my @recs = trim(split(/,/, $_, -1));
		
		$strCount == 2 && (
			$mapData{KEY_MAP_NAME} = $_, 
			next
		); 
		$strCount == 3 && (
			$mapData{KEY_MAP_FILE} = $_,
			next
		); 
		$strCount == 5 && $recs[0] && (
			$mapData{KEY_MAP_DATUM} = $recs[0],
			next
		); 
		$strCount == 9 && $recs[1] && (
			$projectionData{KEY_PROJECTION_NAME} = $recs[1],
			next
		); 
		$strCount >= 10 && $strCount <= 39 && $recs[2] && (
			push(@mapPoints, parse_point(\@recs)),
			next
		); 
		$strCount == 40 && (
			$projectionData{KEY_PROJECTION_LAT_ORIGIN} = $recs[1],
			$projectionData{KEY_PROJECTION_LON_ORIGIN} = $recs[2],
			$projectionData{KEY_PROJECTION_SCALE_FACTOR} = $recs[3],
			$projectionData{KEY_PROJECTION_FALSE_EAST} = $recs[4],
			$projectionData{KEY_PROJECTION_FALSE_NORTH} = $recs[5],
			$projectionData{KEY_PROJECTION_LAT1} = $recs[6],
			$projectionData{KEY_PROJECTION_LAT2} = $recs[7],
			next
		); 
	}
	close(DAT);
	
	$mapData{KEY_PROJECTION} = \%projectionData;
	$mapData{KEY_POINTS} = \@mapPoints;
	return \%mapData;		
}

sub parse_point {
	my $recs = shift;
	
	my %point = ();
	$point{KEY_POINT_X} = $recs->[2] if ($recs->[2]);
	$point{KEY_POINT_Y} = $recs->[3] if ($recs->[3]);
	$point{KEY_POINT_LAT} = 
			($recs->[6] + $recs->[7] / 60) * 
			(($recs->[8] eq "S") ? -1 : 1) if ($recs->[6]);
	$point{KEY_POINT_LON} = 
			$recs->[9] + $recs->[10] / 60 * 
			(($recs->[11] eq "W") ? -1 : 1) if ($recs->[9]);
	$point{KEY_POINT_GRID_X} = $recs->[14] if ($recs->[14]);
	$point{KEY_POINT_GRID_Y} = $recs->[15] if ($recs->[15]);
	$point{KEY_POINT_UTM_ZONE} = $recs->[13] if ($recs->[13]);
	$point{KEY_POINT_UTM_HEMISPHERE} = $recs->[16] if ($recs->[16]);
	
	return \%point;
}

sub main {
	my $file = '';
	
	GetOptions('map|m=s' => \$file);
	print "file is " . $file . "\n";
	
	my $mapData = parse_map($file);

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

	print %$mapData;
	print "\n";
	my $proj = $mapData->{KEY_PROJECTION};
	print %$proj;
	print "\n";
	
	foreach $p (@{$mapData->{KEY_POINTS}}) {
		print %$p;
		print "\n";
	}
}

sub map_proj_data {
	my $map = shift;

	my $datum = $datumParams{$map->{KEY_MAP_DATUM}};
	die "Unknown datum " .  $map->{KEY_MAP_DATUM} unless $datum;
	
	my $projData = $map->{KEY_PROJECTION};
	my $proj = $projParams{$projData->{KEY_PROJECTION_NAME}};
	die "Unknown projection " .  $projData->{KEY_PROJECTION_NAME} unless $proj;

	$proj = sprintf($proj,
		$projData->{KEY_PROJECTION_LAT_ORIGIN},
		$projData->{KEY_PROJECTION_LON_ORIGIN},
		$projData->{KEY_PROJECTION_SCALE_FACTOR},
		$projData->{KEY_PROJECTION_FALSE_EAST},
		$projData->{KEY_PROJECTION_FALSE_NORTH},
		$projData->{KEY_PROJECTION_LAT1},
		$projData->{KEY_PROJECTION_LAT2});
		
	return ($datum, $proj);
}

sub to_latlong {
	my $map = shift;
	
	my ($datum, $proj) = map_proj_data($map);
	
	my $from = new Geo::OSR::SpatialReference();
	$from->ImportFromProj4($proj . " " . $datum . " +no_defs");

	my $to = new Geo::OSR::SpatialReference();
	$to->ImportFromProj4("+proj=latlong " . $datum . " +no_defs");

	foreach $p (@{$map->{KEY_POINTS}}) {
		# Again we assume we have both lat/long values
		# if lat is defined.
		next if defined($p->{KEY_POINT_LAT});
		
		$from->setUTM($p->{KEY_POINT_UTM_ZONE},
			($p-{KEY_POINT_UTM_HEMISPHERE} eq "S") ? 0 : 1)
				if defined($p->{KEY_POINT_UTM_ZONE});

		my $trans = new Geo::OSR::CoordinateTransformation($from, $to);
		
		my $coords = $trans->TransformPoint(
			$p->{KEY_POINT_GRID_X},
			$p->{KEY_POINT_GRID_Y},
			0);
	
		$p->{KEY_POINT_LAT} = $coords->[0];
		$p->{KEY_POINT_LON} = $coords->[1];
	}

	return $map;	
}

sub prepare_raster {
	my $map = shift;
	
	die "Map raster file name is not defined!" unless $map->{KEY_MAP_FILE};
	
	my $filename = $map->{KEY_MAP_FILE};
	$filename =~ s/^(.*[\\\/])?(.*)\..*$/$2/g;
	my $ext = $map->{KEY_MAP_FILE};
	$ext =~ s/^.*\.//g;
	$ext =~ tr/A-Z/a-z/;
	
	die "ozf3 is not supported!" if $ext =~ /^ozf3/;
	
	if ($ext eq "ozf2") {
		my @args = ("@BINDIR@/ozf2tiff", $filename . "." . $ext, $filename . ".tif");
		system(@args);
		$ext = "tif";	
	}
	
	#print $filename . "   " . $ext . "\n";
	
	$map->{KEY_MAP_FILE} = $filename . "." . $ext;
	$map->{KEY_MAP_FILE_NAME} = $filename;
	$map->{KEY_MAP_FILE_EXT} = $ext;
	
	my $data = Geo::GDAL::Dataset::Open($map->{KEY_MAP_FILE});
	die "Unable to open map raster data" unless $data;

	my ($width, $height) = $data->Size();
	$map->{KEY_MAP_RASTER_WIDTH} = $width;
	$map->{KEY_MAP_RASTER_HEIGHT} = $height;
	
	return $map;
}

sub to_tagged_gtiff {
	my $map = shift;
	
	my @args = ("@GDAL_TRANSLATE@", "-of", "GTiff", "-co", "TILED=YES");

	my ($datum, $proj) = map_proj_data($map);
	
	my $a_srs = "+proj=latlong " . $datum . " +no_defs";
	
	push @args, "-a_srs";
	push @args, $a_srs;

	foreach $p (@{$map->{KEY_POINTS}}) {
		push @args, "-gcp";

		push @args, $p->{KEY_POINT_X};
		push @args, $p->{KEY_POINT_Y};
		push @args, $p->{KEY_POINT_LON};
		push @args, $p->{KEY_POINT_LAT};
	}

	push @args, $map->{KEY_MAP_FILE};
	push @args, $map->{KEY_MAP_FILE_NAME} . ".gtf";
	
	system(@args);
	
	$map->{KEY_MAP_FILE} = $map->{KEY_MAP_FILE_NAME} . ".gtf";
	$map->{KEY_MAP_FILE_EXT} = "gtf";

	return $map;	
}

sub warp {
	my $map = shift;
	
	my @args = ("@GDALWARP@", "-of", "GTiff", "-co", "TILED=YES");

	my ($datum, $proj) = map_proj_data($map);
		
	push @args, "-tps";
	
	push @args, "-ts";
	push @args, $map->{KEY_MAP_RASTER_WIDTH};
	push @args, $map->{KEY_MAP_RASTER_HEIGHT};

	my $s_srs = "+proj=latlong " . $datum . " +no_defs";
	#my $t_srs = $proj . " " . $datum . " +no_defs";
	my $t_srs = "+proj=latlong " . $datum . " +no_defs";
	
	push @args, "-s_srs";
	push @args, $s_srs;
	push @args, "-t_srs";
	push @args, $t_srs;


	push @args, $map->{KEY_MAP_FILE};
	push @args, $map->{KEY_MAP_FILE_NAME} . ".gtiff";
	
	system(@args);
	
	$map->{KEY_MAP_FILE} = $map->{KEY_MAP_FILE_NAME} . ".gtiff";
	$map->{KEY_MAP_FILE_EXT} = "gtiff";

	return $map;	
}

