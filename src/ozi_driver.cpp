/*
 * ozidriver.cpp
 *
 *  Created on: Jul 31, 2010
 *      Author: geom
 */

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_proxy.h>
#include <ogr_spatialref.h>
#include <cpl_string.h>

/*
 * OGR stuff.
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  OGRSpatialReference translation from OziExplorer
 *           georeferencing information.
 * Author:   Andrey Kiselev, dron@ak4719.spb.edu
 */

/************************************************************************/
/*  Correspondence between Ozi and EPSG datum codes.                    */
/************************************************************************/

typedef struct {
	const char *pszOziDatum;
	int nEPSGCode;
	const char *pszProjRef;
} OZIDatums;

static const OZIDatums
		aoDatums[] =
				{
						{ "Adindan", 4201,
								"+proj=latlong +ellps=clrk80 +towgs84=-162,-12,206,0,0,0,0 +no_defs" },
						{ "Afgooye", 4205,
								"+proj=latlong +ellps=krass +towgs84=-43.0,-163.0,45.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Ain el Abd 1970", 4204,
								"+proj=latlong +ellps=intl +towgs84=-150,-251,-2,0,0,0,0 +no_defs" },
						{ "Anna 1 Astro 1965", -1,
								"+proj=latlong +ellps=aust_SA +towgs84=-491,-22,435,0,0,0,0 +no_defs" },
						{ "Arc 1950", 4209,
								"+proj=latlong +ellps=clrk80 +towgs84=-143,-90,-294,0,0,0,0 +no_defs" },
						{ "Arc 1960", 4210,
								"+proj=latlong +ellps=clrk80 +towgs84=-160,-8,-300,0,0,0,0 +no_defs" },
						{ "Ascension Island 1958", 4712,
								"+proj=latlong +ellps=intl +towgs84=-207,107,52,0,0,0,0 +no_defs" },
						{ "Astro B4 Sorol Atoll", -1,
								"+proj=latlong +ellps=intl +towgs84=114,-116,-333,0,0,0,0 +no_defs" },
						{ "Astro Beacon 1945", -1,
								"+proj=latlong +ellps=intl +towgs84=145,75,-272,0,0,0,0 +no_defs" },
						{ "Astro DOS 71/4", -1,
								"+proj=latlong +ellps=intl +towgs84=-320,550,-494,0,0,0,0 +no_defs" },
						{ "Astronomic Stn 1952", -1,
								"+proj=latlong +ellps=intl +towgs84=124,-234,-25,0,0,0,0 +no_defs" },
						{ "Australian Geodetic 1966", 4202,
								"+proj=latlong +ellps=aust_SA +towgs84=-133,-48,148,0,0,0,0 +no_defs" },
						{ "Australian Geodetic 1984", 4203,
								"+proj=latlong +ellps=aust_SA +towgs84=-134,-48,149,0,0,0,0 +no_defs" },
						{ "Australian Geocentric 1994 (GDA94)", 4283,
								"+proj=latlong +ellps=GRS80 +towgs84=0.0,0.0,0.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Austrian", -1,
								"+proj=latlong +ellps=bessel +towgs84=594,84,471,0,0,0,0 +no_defs" },
						{ "Bellevue (IGN)", 4714,
								"+proj=latlong +ellps=intl +towgs84=-127,-769,472,0,0,0,0 +no_defs" },
						{ "Bermuda 1957", 4216,
								"+proj=latlong +ellps=clrk66 +towgs84=-73.0,213.0,296.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Bogota Observatory", 4218,
								"+proj=latlong +ellps=intl +towgs84=307.0,304.0,-318.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Campo Inchauspe", 4221,
								"+proj=latlong +ellps=intl +towgs84=-148.0,136.0,90.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Canton Astro 1966", -1,
								"+proj=latlong +ellps=intl +towgs84=298,-304,-375,0,0,0,0 +no_defs" },
						{ "Cape", 4222,
								"+proj=latlong +ellps=clrk80 +towgs84=-136.0,-108.0,-292.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Cape Cannaveral", 4717,
								"+proj=latlong +ellps=clrk66 +towgs84=-2,150,181,0,0,0,0 +no_defs" },
						{ "Carthage", 4223,
								"+proj=latlong +ellps=clrk80 +towgs84=-263,6,431,0,0,0,0 +no_defs" },
						{ "CH-1903", 4150,
								"+proj=latlong +ellps=bessel +towgs84=674,15,405,0,0,0,0 +no_defs" },
						{ "Chatham 1971", 4672,
								"+proj=latlong +ellps=intl +towgs84=175,-38,113,0,0,0,0 +no_defs" },
						{ "Chua Astro", 4224,
								"+proj=latlong +ellps=intl +towgs84=-134.0,229.0,-29.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Corrego Alegre", 4225,
								"+proj=latlong +ellps=intl +towgs84=-206.0,172.0,-6.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Djakarta (Batavia)", 4211,
								"+proj=latlong +ellps=bessel +towgs84=-377,681,-50,0,0,0,0 +no_defs" },
						{ "DOS 1968", -1,
								"+proj=latlong +ellps=intl +towgs84=230,-199,-752,0,0,0,0 +no_defs" },
						{ "Easter Island 1967", 4719,
								"+proj=latlong +ellps=intl +towgs84=211,147,111,0,0,0,0 +no_defs" },
						{ "Egypt", 4199,
								"+proj=latlong +ellps=intl +towgs84=-130,-117,-151,0,0,0,0 +no_defs" },
						{ "European 1950", 4230,
								"+proj=latlong +ellps=intl +towgs84=-87,-98,-121,0,0,0,0 +no_defs" },
						{ "European 1950 (Mean France)", 4230,
								"+proj=latlong +ellps=intl +towgs84=-87,-96,-120,0,0,0,0 +no_defs" },
						{ "European 1950 (Spain adn Portugal)", 4230,
								"+proj=latlong +ellps=intl +towgs84=-84,-107,-120,0,0,0,0 +no_defs" },
						{ "European 1979", 4668,
								"+proj=latlong +ellps=intl +towgs84=-86,-98,-119,0,0,0,0 +no_defs" },
						{ "Finland Hayford", -1,
								"+proj=latlong +ellps=intl +towgs84=-78,-231,-97,0,0,0,0 +no_defs" },
						{ "Gandajika Base", 4233,
								"+proj=latlong +ellps=intl +towgs84=-133.0,-321.0,50.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Geodetic Datum 1949", 4272,
								"+proj=latlong +ellps=intl +towgs84=84,-22,-209,0,0,0,0 +no_defs" },
						{ "Guam 1963", 4675,
								"+proj=latlong +ellps=clrk66 +towgs84=-100,-248,259,0,0,0,0 +no_defs" },
						{ "GUX 1 Astro", -1,
								"+proj=latlong +ellps=intl +towgs84=252,-209,-751,0,0,0,0 +no_defs" },
						{ "Hartebeeshoek94", 4148,
								"+proj=latlong +ellps=WGS84 +towgs84=0.0,0.0,0.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Hermannskogel", 4312,
								"+proj=latlong +ellps=bess_nam +towgs84=653,-212,449,0,0,0,0 +no_defs" },
						{ "Hjorsey 1955", 4658,
								"+proj=latlong +ellps=intl +towgs84=-73.0,46.0,-86.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Hong Kong 1963", 4739,
								"+proj=latlong +ellps=intl +towgs84=-156,-271,-189,0,0,0,0 +no_defs" },
						{ "Hu-Tzu-Shan", 4236,
								"+proj=latlong +ellps=intl +towgs84=-634,-549,-201,0,0,0,0 +no_defs" },
						{ "Indian Bangladesh", 4240,
								"+proj=latlong +ellps=evrst30 +towgs84=289.0,734.0,257.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Indian Thailand", 4239,
								"+proj=latlong +ellps=evrst30 +towgs84=214.0,836.0,303.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Israeli", -1,
								"+proj=latlong +a=6378300.789 +b=6356566.435 +towgs84=-235,-85,264,0,0,0,0 +no_defs" },
						{ "Ireland 1965", -1,
								"+proj=latlong +ellps=mod_airy +towgs84=506,-122,611,0,0,0,0 +no_defs" },
						{ "ISTS 073 Astro 1969", -1,
								"+proj=latlong +ellps=intl +towgs84=208,-435,-229,0,0,0,0 +no_defs" },
						{ "Johnston Island", 4725,
								"+proj=latlong +ellps=intl +towgs84=191,-77,-204,0,0,0,0 +no_defs" },
						{ "Kandawala", 4244,
								"+proj=latlong +ellps=evrst30 +towgs84=-97.0,787.0,86.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Kerguelen Island", 4698,
								"+proj=latlong +ellps=intl +towgs84=145.0,-187.0,103.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Kertau 1948", 4245,
								"+proj=latlong +a=6377304.063 +b=6356103.038993155 +towgs84=-11.0,851.0,5.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "L.C. 5 Astro", -1,
								"+proj=latlong +ellps=clrk66 +towgs84=42.0,124.0,147.0,0,0,0,0 +no_defs" },
						{ "Liberia 1964", 4251,
								"+proj=latlong +ellps=clrk80 +towgs84=-90.0,40.0,88.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Luson Mindanao", -1,
								"+proj=latlong +ellps=clrk66 +towgs84=-133.0,-79.0,-72.0,0,0,0,0 +no_defs" },
						{ "Luson Philippines", 4253,
								"+proj=latlong +ellps=clrk66 +towgs84=-133.0,-77.0,-51.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Mahe 1971", 4256,
								"+proj=latlong +ellps=clrk80 +towgs84=41.0,-220.0,-134.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Marco Astro", -1,
								"+proj=latlong +ellps=intl +towgs84=-289.0,-124.0,60.0,0,0,0,0 +no_defs" },
						{ "Massawa", 4262,
								"+proj=latlong +ellps=bessel +towgs84=639.0,405.0,60.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Merchich", 4261,
								"+proj=latlong +a=6378249.2 +b=6356515 +towgs84=31.0,146.0,47.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Midway Astro 1961", 4727,
								"+proj=latlong +ellps=intl +towgs84=912.0,-58.0,1227.0,0,0,0,0 +no_defs" },
						{ "Minna", 4263,
								"+proj=latlong +ellps=clrk80 +towgs84=-92.0,-93.0,122.0,0,0,0,0 +no_defs" },
						{ "NAD27 Alaska", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-5.0,135.0,172.0,0,0,0,0 +no_defs" },
						{ "NAD27 Bahamas", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-4.0,154.0,178.0,0,0,0,0 +no_defs" },
						{ "NAD27 Canada", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-10.0,158.0,187.0,0,0,0,0 +no_defs" },
						{ "NAD27 CanalZone", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=0.0,125.0,201.0,0,0,0,0 +no_defs" },
						{ "NAD27 Carribean", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-7.0,152.0,178.0,0,0,0,0 +no_defs" },
						{ "NAD27 Central", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=0.0,125.0,194.0,0,0,0,0 +no_defs" },
						{ "NAD27 CONUS", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-8.0,160.0,176.0,0,0,0,0 +no_defs" },
						{ "NAD27 Cuba", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-9.0,152.0,178.0,0,0,0,0 +no_defs" },
						{ "NAD27 Greenland", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=11.0,114.0,195.0,0,0,0,0 +no_defs" },
						{ "NAD27 Mexico", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=-12.0,130.0,190.0,0,0,0,0 +no_defs" },
						{ "NAD27 San Salvador", 4267,
								"+proj=latlong +ellps=clrk66 +datum=NAD27 +towgs84=1.0,140.0,165.0,0,0,0,0 +no_defs" },
						{ "NAD83", 4269, "+proj=latlong +ellps=GRS80 +datum=NAD83 +no_defs" },
						{ "Nahrwn Masirah IInd", -1,
								"+proj=latlong +ellps=clrk80 +towgs84=-247.0,-148.0,369.0,0,0,0,0 +no_defs" },
						{ "Nahrwn Saudi Arbia", -1,
								"+proj=latlong +ellps=clrk80 +towgs84=-231.0,-196.0,482.0,0,0,0,0 +no_defs" },
						{ "Nahrwn United Arab", -1,
								"+proj=latlong +ellps=clrk80 +towgs84=-249.0,-156.0,381.0,0,0,0,0 +no_defs" },
						{ "Naparima BWI", 4271,
								"+proj=latlong +ellps=intl +towgs84=-2.0,374.0,172.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "NGO1948", 4273,
								"+proj=latlong +a=6377492.018 +b=6356173.508712696 +towgs84=315.0,-217.0,528.0,0,0,0,0 +no_defs" },
						{ "NTF France", 4807,
								"+proj=latlong +a=6378249.2 +b=6356515 +towgs84=-168,-60,320,0,0,0,0 +no_defs" },
						{ "Norsk", -1,
								"+proj=latlong +a=6377492.018 +b=6356173.508712696 +towgs84=278.0,93.0,474.0,0,0,0,0 +no_defs" },
						{ "Observatorio 1966", 4129,
								"+proj=latlong +ellps=intl +towgs84=-425.0,-169.0,81.0,0,0,0,0 +no_defs" },
						{ "Old Egyptian", 4229,
								"+proj=latlong +ellps=helmert +towgs84=-130.0,110.0,-13.0,0,0,0,0 +no_defs" },
						{ "Old Hawaiian", 4135,
								"+proj=latlong +ellps=clrk66 +towgs84=61.0,-285.0,-181.0,0,0,0,0 +no_defs" },
						{ "Oman", -1,
								"+proj=latlong +ellps=clrk80 +towgs84=-346.0,-1.0,224.0,0,0,0,0 +no_defs" },
						{ "Old Srvy Grt Britn", 4277,
								"+proj=latlong +ellps=airy +towgs84=375.0,-111.0,431.0,0,0,0,0 +no_defs" },
						{ "Pico De Las Nieves", 4728,
								"+proj=latlong +ellps=intl +towgs84=-307.0,-92.0,127.0,0,0,0,0 +no_defs" },
						{ "Pitcairn Astro 1967", 4729,
								"+proj=latlong +ellps=intl +towgs84=185,165,42,0,0,0,0 +no_defs" },
						{ "Potsdam Rauenberg DHDN", 4314,
								"+proj=latlong +ellps=bessel +towgs84=606.0,23.0,413.0,0,0,0,0 +no_defs" },
						{ "Prov So Amrican 1956", 4248,
								"+proj=latlong +ellps=intl +towgs84=-288.0,175.0,-376.0,0,0,0,0 +no_defs" },
						{ "Prov So Chilean 1963", -1,
								"+proj=latlong +ellps=intl +towgs84=16.0,196.0,93.0,0,0,0,0 +no_defs" },
						{ "Puerto Rico", 4139,
								"+proj=latlong +ellps=clrk66 +towgs84=11.0,72.0,-101.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Pulkovo 1942 (1)", 4284,
								"+proj=latlong +ellps=krass +towgs84=28,-130,-95,0,0,0,0 +no_defs" },
						{ "Pulkovo 1942 (2)", 4284,
								"+proj=latlong +ellps=krass +towgs84=28,-130,-95,0,0,0,0 +no_defs" },
						{ "Qatar National", 4285,
								"+proj=latlong +ellps=intl +towgs84=-128.0,-283.0,22.0,0,0,0,0 +no_defs" },
						{ "Qornoq", 4287,
								"+proj=latlong +ellps=intl +towgs84=164,138,-189,0,0,0,0 +no_defs" },
						{ "Reunion", 4626,
								"+proj=latlong +ellps=intl +towgs84=94.0,-948.0,-1252.0,0,0,0,0 +no_defs" },
						{ "Rijksdriehoeksmeting", -1,
								"+proj=latlong +ellps=bessel +towgs84=593.0,26.0,478.0,0,0,0,0 +no_defs" },
						{ "Rome 1940", -1,
								"+proj=latlong +ellps=intl +towgs84=-255.0,-65.0,9.0,0,0,0,0 +no_defs" },
						{ "RT 90", 4124,
								"+proj=latlong +ellps=bessel +towgs84=498.0,-36.0,568.0,0,0,0,0 +no_defs" },
						{ "S42", -1,
								"+proj=latlong +ellps=krass +towgs84=28.0,-121.0,-77.0,0,0,0,0 +no_defs" },
						{ "Santo (DOS)", 4730,
								"+proj=latlong +ellps=intl +towgs84=170,42,84,0,0,0,0 +no_defs" },
						{ "Sao Braz", -1,
								"+proj=latlong +ellps=intl +towgs84=-203.0,141.0,53.0,0,0,0,0 +no_defs" },
						{ "Sapper Hill 1943", 4292,
								"+proj=latlong +ellps=intl +towgs84=-355.0,16.0,74.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Schwarzeck", 4293,
								"+proj=latlong +ellps=bess_nam +towgs84=616.0,97.0,-251.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "South American 1969", 4618,
								"+proj=latlong +ellps=aust_SA +towgs84=-57.0,1.0,-41.0,0,0,0,0 +no_defs" },
						{ "South Asia", -1,
								"+proj=latlong +ellps=fschr60m +towgs84=7.0,-10.0,-26.0,0,0,0,0 +no_defs" },
						{ "Southeast Base", -1,
								"+proj=latlong +ellps=intl +towgs84=-499.0,249.0,314.0,0,0,0,0 +no_defs" },
						{ "Southwest Base", -1,
								"+proj=latlong +ellps=intl +towgs84=-104.0,167.0,-38.0,0,0,0,0 +no_defs" },
						{ "Timbalai 1948", 4298,
								"+proj=latlong +ellps=evrst30 +towgs84=-689.0,691.0,-46.0,0,0,0,0 +no_defs" },
						{ "Tokyo", 4301,
								"+proj=latlong +ellps=bessel +towgs84=-128.0,481.0,664.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Tristan Astro 1968", 4734,
								"+proj=latlong +ellps=intl +towgs84=-632,438,-609,0,0,0,0 +no_defs" },
						{ "Viti Levu 1916", 4731,
								"+proj=latlong +ellps=clrk80 +towgs84=51,391,-36,0,0,0,0 +no_defs" },
						{ "Wake-Eniwetok 1960", -1,
								"+proj=latlong +ellps=hough =towgs84=101.0,52.0,-39.0,0,0,0,0 +no_defs" },
						{ "WGS 72", 4322,
								"+proj=latlong +ellps=WGS72 +towgs84=0,0,5.0,0,0,0,0 +no_defs" },
						{ "WGS 84", 4326, "+proj=latlong +ellps=WGS84 +datum=WGS84 +no_defs" },
						{ "Yacare", 4309,
								"+proj=latlong +ellps=intl +towgs84=-155.0,171.0,37.0,0.0,0.0,0.0,0.0 +no_defs" },
						{ "Zanderij", 4311,
								"+proj=latlong +ellps=intl +towgs84=-265.0,120.0,-358.0,0.0,0.0,0.0,0.0 +no_defs" },

						{ NULL, 0, NULL } };

int CPL_STDCALL ImportFromOzi(OGRSpatialReference *pSRS, char *pszDatum,
		char *pszProj, char *pszProjParms, int* targetEPSG) {

	pSRS->Clear();

	/* -------------------------------------------------------------------- */
	/*      Operate on the basis of the projection name.                    */
	/* -------------------------------------------------------------------- */
	char **papszProj = CSLTokenizeStringComplex(pszProj, ",", TRUE, TRUE);
	char **papszProjParms = CSLTokenizeStringComplex(pszProjParms, ",", TRUE,
			TRUE);
	char **papszDatum = NULL;

	const OZIDatums *paoDatum = aoDatums;

	/* -------------------------------------------------------------------- */
	/*      Try to translate the datum/spheroid.                            */
	/* -------------------------------------------------------------------- */
	papszDatum = CSLTokenizeString2(pszDatum, ",", CSLT_ALLOWEMPTYTOKENS
			| CSLT_STRIPLEADSPACES | CSLT_STRIPENDSPACES);
	if (papszDatum == NULL)
		goto not_enough_data;

	*targetEPSG = 4326; // WGS84

	// Search for matching datum
	while (paoDatum->pszOziDatum) {
		if (EQUAL( papszDatum[0], paoDatum->pszOziDatum )) {
			OGRSpatialReference oGCS;
			if (oGCS.importFromProj4(paoDatum->pszProjRef) != OGRERR_NONE) {
				CPLError(CE_Warning, CPLE_AppDefined, "proj4 failure");
				goto not_enough_data;
			}
			pSRS->CopyGeogCSFrom(&oGCS);
			char* twkt;
			oGCS.exportToWkt(&twkt);
			if (paoDatum->nEPSGCode > 0) {
				*targetEPSG = paoDatum->nEPSGCode;
			}
			break;
		}
		paoDatum++;
	}

	if (!paoDatum->pszOziDatum) {
		CPLError(CE_Warning, CPLE_AppDefined, "Wrong datum name \"%s\".",
				papszDatum[0]);

		CSLDestroy(papszProj);
		CSLDestroy(papszProjParms);
		CSLDestroy(papszDatum);

		return OGRERR_UNSUPPORTED_SRS;
	}

	/* -------------------------------------------------------------------- */
	/*      Try to setup projection.                            */
	/* -------------------------------------------------------------------- */
	if (CSLCount(papszProj) < 2) {
		goto not_enough_data;
	}

	if (EQUALN(papszProj[1], "Latitude/Longitude", 18)) {
	} else if (EQUALN(papszProj[1], "Mercator", 8)) {
		pSRS->SetMercator(0.0, 0.0, 1.0, 0.0, 0.0);
	} else if (EQUALN(papszProj[1], "Transverse Mercator", 19)) {
		if (CSLCount(papszProjParms) < 6)
			goto not_enough_data;
		pSRS->SetTM(CPLAtof(papszProjParms[1]), CPLAtof(papszProjParms[2]),
				CPLAtof(papszProjParms[3]), CPLAtof(papszProjParms[4]),
				CPLAtof(papszProjParms[5]));
	} else if (EQUALN(papszProj[1], "(UTM) Universal Transverse Mercator", 35)) {
		// TODO: calculate zone/hemisphere
		//pSRS->SetUTM();
	} else if (EQUALN(papszProj[1], "Lambert Conformal Conic", 23)) {
		if (CSLCount(papszProjParms) < 8)
			goto not_enough_data;
		pSRS->SetLCC(CPLAtof(papszProjParms[6]), CPLAtof(papszProjParms[7]),
				CPLAtof(papszProjParms[1]), CPLAtof(papszProjParms[2]), 0.0,
				0.0);
	} else if (EQUALN(papszProj[1], "Sinusoidal", 10)) {
		if (CSLCount(papszProjParms) < 3)
			goto not_enough_data;
		pSRS->SetSinusoidal(CPLAtof(papszProjParms[2]), 0.0, 0.0);
	} else if (EQUALN(papszProj[1], "Albers Equal Area", 17)) {
		if (CSLCount(papszProjParms) < 8)
			goto not_enough_data;
		pSRS->SetACEA(CPLAtof(papszProjParms[6]), CPLAtof(papszProjParms[7]),
				CPLAtof(papszProjParms[1]), CPLAtof(papszProjParms[2]), 0.0,
				0.0);
	} else if (EQUALN(papszProj[1], "(EQC) Equidistant Conic", 23)) {
		if (CSLCount(papszProjParms) < 8)
			goto not_enough_data;
		pSRS->SetEC(CPLAtof(papszProjParms[6]), CPLAtof(papszProjParms[7]),
				CPLAtof(papszProjParms[1]), CPLAtof(papszProjParms[2]), 0.0,
				0.0);
	} else if (EQUALN(papszProj[1], "Polyconic (American)", 20)) {
		if (CSLCount(papszProjParms) < 3)
			goto not_enough_data;
		pSRS->SetPolyconic(0.0, CPLAtof(papszProjParms[2]), 0.0, 0.0);
	} else if (EQUALN(papszProj[1], "Van Der Grinten", 15)) {
		if (CSLCount(papszProjParms) < 3)
			goto not_enough_data;
		pSRS->SetVDG(CPLAtof(papszProjParms[2]), 0.0, 0.0);
	} else if (EQUALN(papszProj[1], "(WIV) Wagner IV", 15)) {
		if (CSLCount(papszProjParms) < 3)
			goto not_enough_data;
		pSRS->SetWagner(4, CPLAtof(papszProjParms[2]), 0.0, 0.0);
	} else if (EQUALN(papszProj[1], "Bonne", 5)) {
		if (CSLCount(papszProjParms) < 6)
			goto not_enough_data;
		pSRS->SetBonne(CPLAtof(papszProjParms[1]), CPLAtof(papszProjParms[2]),
				0.0, 0.0);
	}
	else {
		CPLError(CE_Failure, CPLE_AppDefined,
				"OSR_Ozi: Unsupported projection: \"%s\"", papszProj[1]);

		CSLDestroy(papszProj);
		CSLDestroy(papszProjParms);
		CSLDestroy(papszDatum);

		return OGRERR_UNSUPPORTED_SRS;
	}

	/* -------------------------------------------------------------------- */
	/*      Grid units translation                                          */
	/* -------------------------------------------------------------------- */
	if (pSRS->IsLocal() || pSRS->IsProjected())
		pSRS->SetLinearUnits(SRS_UL_METER, 1.0);

	pSRS->FixupOrdering();

	CSLDestroy(papszProj);
	CSLDestroy(papszProjParms);
	CSLDestroy(papszDatum);

	return OGRERR_NONE;

	not_enough_data:

	CSLDestroy(papszProj);
	CSLDestroy(papszProjParms);
	CSLDestroy(papszDatum);

	return OGRERR_NOT_ENOUGH_DATA;

}

#define MAX_GCP 30

static CPLErr GDALOpenOziMapFile(const char *pszFilename, char ***papszLines) {

	int nLines = 0;

	VALIDATE_POINTER1( pszFilename, "GDALOpenOziMapFile", CE_Failure );

	*papszLines = CSLLoad2(pszFilename, 1000, 200, NULL);

	if (!*papszLines)
		return CE_Failure;

	nLines = CSLCount(*papszLines);
	int iLine = 0;
	for (iLine = 0; iLine < nLines; iLine++) {
		CPLDebug("OZI", (*papszLines)[iLine]);
	}
	// Check the OziExplorer Map file signature
	if (nLines < 5
			|| !EQUALN((*papszLines)[0], "OziExplorer Map Data File Version ", 34)) {
		CSLDestroy(*papszLines);
		CPLError(
				CE_Failure,
				CPLE_AppDefined,
				"GDALOpenOziMapFile(): file \"%s\" is not in OziExplorer Map format.",
				pszFilename);
		return CE_Failure;
	}

	return CE_None;
}

static CPLErr GDALParseOziMapParams(const char *pszFilename, char **papszLines,
		double *padfGeoTransform, char **ppszWKT, int *pnGCPCount,
		GDAL_GCP **ppasGCPs)

{
	char **papszTok = NULL;
	int iLine, nLines = 0;
	int nCoordinateCount = 0;
	GDAL_GCP asGCPs[MAX_GCP];

	VALIDATE_POINTER1( padfGeoTransform, "GDALLoadOziMapFile", CE_Failure );
	VALIDATE_POINTER1( pnGCPCount, "GDALLoadOziMapFile", CE_Failure );
	VALIDATE_POINTER1( ppasGCPs, "GDALLoadOziMapFile", CE_Failure );

	nLines = CSLCount(papszLines);

	OGRSpatialReference oSRS, *poTargetSRS = NULL, *poLatLong = NULL;
	OGRCoordinateTransformation *poTransform = NULL, *poPTransform = NULL;
	char *pszProj = NULL, *pszProjParms = NULL;

	for (iLine = 8; iLine < nLines; iLine++) {
		if (EQUALN(papszLines[iLine], "Map Projection", 14)) {
			pszProj = papszLines[iLine];
			continue;
		}

		if (EQUALN(papszLines[iLine], "Projection Setup", 16)) {
			pszProjParms = papszLines[iLine];
			continue;
		}
	}

	int targetEPSG;

	if (papszLines[4][0] != '\0' && pszProj && pszProjParms && ImportFromOzi(
			&oSRS, papszLines[4], pszProj, pszProjParms, &targetEPSG)
			== OGRERR_NONE) {

		OGRSpatialReference tSRS;
		tSRS.importFromEPSG(targetEPSG);

		poTargetSRS = oSRS.Clone();
		poTargetSRS->CopyGeogCSFrom(&tSRS);

		if (ppszWKT != NULL)
			poTargetSRS->exportToWkt(ppszWKT);

		poLatLong = oSRS.CloneGeogCS();
		poTransform = OGRCreateCoordinateTransformation(poLatLong, poTargetSRS);
		poPTransform = OGRCreateCoordinateTransformation(&oSRS, poTargetSRS);
	} else {
		CPLError(
				CE_Failure,
				CPLE_AppDefined,
				"GDALLoadOziMapFile(): file \"%s\" is not georeferenced correctly.",
				pszFilename);
		return CE_Failure;
	}

	// Iterate all lines in the TAB-file
	for (iLine = 5; iLine < nLines; iLine++) {
		CSLDestroy(papszTok);
		papszTok = CSLTokenizeString2(papszLines[iLine], ",",
				CSLT_ALLOWEMPTYTOKENS | CSLT_STRIPLEADSPACES
						| CSLT_STRIPENDSPACES);

		if (CSLCount(papszTok) < 12)
			continue;

		if (CSLCount(papszTok) >= 12 && EQUALN(papszTok[0], "Point", 5)
				&& nCoordinateCount < MAX_GCP) {
			if (!EQUAL(papszTok[3], "")) {
				double dfLon, dfLat;

				GDALInitGCPs(1, asGCPs + nCoordinateCount);

				// Set pixel/line part
				asGCPs[nCoordinateCount].dfGCPPixel = CPLAtofM(papszTok[2]);
				asGCPs[nCoordinateCount].dfGCPLine = CPLAtofM(papszTok[3]);

				if (!EQUAL(papszTok[6], "") && !EQUAL(papszTok[7], "")
						&& !EQUAL(papszTok[9], "") && !EQUAL(papszTok[10], "")) {

					// Set geographical coordinates of the pixels
					dfLon = CPLAtofM(papszTok[9]) + CPLAtofM(papszTok[10])
							/ 60.0;
					dfLat = CPLAtofM(papszTok[6]) + CPLAtofM(papszTok[7])
							/ 60.0;
					if (EQUAL(papszTok[11], "W"))
						dfLon = -dfLon;
					if (EQUAL(papszTok[8], "S"))
						dfLat = -dfLat;

					if (poTransform)
						poTransform->Transform(1, &dfLon, &dfLat);

					asGCPs[nCoordinateCount].dfGCPX = dfLon;
					asGCPs[nCoordinateCount].dfGCPY = dfLat;

					nCoordinateCount++;
				}
			}
		}
	}

	if (poTransform)
		delete poTransform;
	if (poPTransform)
		delete poPTransform;
	if (poLatLong)
		delete poLatLong;
	if (poTargetSRS)
		delete poTargetSRS;

	CSLDestroy(papszTok);

	if (nCoordinateCount == 0) {
		CPLDebug("GDAL", "GDALLoadOziMapFile(\"%s\") did not get any GCPs.",
				pszFilename);
		return CE_Failure;
	}

	/* -------------------------------------------------------------------- */
	/*      Try to convert the GCPs into a geotransform definition, if      */
	/*      possible.  Otherwise we will need to use them as GCPs.          */
	/* -------------------------------------------------------------------- */
	if (!GDALGCPsToGeoTransform(nCoordinateCount, asGCPs, padfGeoTransform,
			FALSE)) {
		if (pnGCPCount && ppasGCPs) {
			CPLDebug("GDAL",
					"GDALLoadOziMapFile(%s) found file, wasn't able to derive a\n"
						"first order geotransform.  Using points as GCPs.",
					pszFilename);

			*ppasGCPs = (GDAL_GCP *) CPLCalloc(sizeof(GDAL_GCP),
					nCoordinateCount);
			memcpy(*ppasGCPs, asGCPs, sizeof(GDAL_GCP) * nCoordinateCount);
			*pnGCPCount = nCoordinateCount;
		}
	} else {
		GDALDeinitGCPs(nCoordinateCount, asGCPs);
	}

	return CE_None;
}

#undef MAX_GCP

/*
 * OZI Driver stuff
 *
 *
 *
 */
class CPL_DLL OziDataset: public GDALProxyDataset {
private:
	GDALDataset *poUnderlyingDS;
	char *pszProjectionRef;
	double adfGeoTransform[6];
	int nGCPCount;
	GDAL_GCP *pasGCPList;
protected:
	virtual GDALDataset *RefUnderlyingDataset();
	virtual void UnrefUnderlyingDataset(GDALDataset* poUnderlyingDataset);
public:
	OziDataset(GDALDataset *poDS);
	virtual ~OziDataset();

	virtual const char *GetProjectionRef(void);
	virtual CPLErr SetProjection(const char *);

	virtual CPLErr GetGeoTransform(double *);
	virtual CPLErr SetGeoTransform(double *);

	virtual int GetGCPCount();
	virtual const char *GetGCPProjection();
	virtual const GDAL_GCP *GetGCPs();
	virtual CPLErr SetGCPs(int nGCPCount, const GDAL_GCP *pasGCPList,
			const char *pszGCPProjection);

	static GDALDataset *Open(GDALOpenInfo *);
	static int Identify(GDALOpenInfo *);
};

OziDataset::OziDataset(GDALDataset *poDS) {
	poUnderlyingDS = poDS;
	pszProjectionRef = NULL;
	nGCPCount = 0;
	pasGCPList = NULL;

	adfGeoTransform[0] = 0.0;
	adfGeoTransform[1] = 1.0;
	adfGeoTransform[2] = 0.0;
	adfGeoTransform[3] = 0.0;
	adfGeoTransform[4] = 0.0;
	adfGeoTransform[5] = 1.0;
}

OziDataset::~OziDataset() {
	FlushCache();
	if (pszProjectionRef) {
		CPLFree(pszProjectionRef);
	}
	if (pasGCPList) {
		CPLFree(pasGCPList);
	}
	if (poUnderlyingDS) {
		delete poUnderlyingDS;
	}
}

GDALDataset* OziDataset::RefUnderlyingDataset() {
	return poUnderlyingDS;
}
void OziDataset::UnrefUnderlyingDataset(GDALDataset* poUnderlyingDataset) {
}

const char* OziDataset::GetProjectionRef(void) {
	return pszProjectionRef;
}
CPLErr OziDataset::SetProjection(const char *) {
	return CE_None;
}

CPLErr OziDataset::GetGeoTransform(double *padfTransform) {
	memcpy(padfTransform, adfGeoTransform, sizeof(double) * 6);
	return CE_None;
}
CPLErr OziDataset::SetGeoTransform(double *) {
	return CE_None;
}
int OziDataset::GetGCPCount() {
	return nGCPCount;
}
const char *OziDataset::GetGCPProjection() {
	return pszProjectionRef;
}
const GDAL_GCP *OziDataset::GetGCPs() {
	return pasGCPList;
}
CPLErr OziDataset::SetGCPs(int nGCPCount, const GDAL_GCP *pasGCPList,
		const char *pszGCPProjection) {
	return CE_None;
}

/************************************************************************/
/*                                Identify()                            */
/************************************************************************/

int OziDataset::Identify(GDALOpenInfo * poOpenInfo)

{
	if (poOpenInfo->nHeaderBytes < 34)
		return FALSE;

	if (strncmp((const char *) poOpenInfo->pabyHeader,
			"OziExplorer Map Data File Version ", 34) != 0)
		return FALSE;

	return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/
GDALDataset *OziDataset::Open(GDALOpenInfo * poOpenInfo)

{
	if (!Identify(poOpenInfo)) {
		return NULL;
	}
	if (poOpenInfo->eAccess == GA_Update) {
		CPLError(CE_Failure, CPLE_NotSupported,
				"The Ozi driver does not support update access to existing"
					" files.\n");
		return NULL;
	}

	/* -------------------------------------------------------------------- */
	/*      Open the file and ingest.                                       */
	/* -------------------------------------------------------------------- */
	FILE *fp;
	int nGifErr;

	fp = VSIFOpenL(poOpenInfo->pszFilename, "r");
	if (fp == NULL)
		return NULL;

	/* -------------------------------------------------------------------- */
	/*      Create a corresponding GDALDataset.                             */
	/* -------------------------------------------------------------------- */

	char **papszLines = NULL;
	if (GDALOpenOziMapFile(poOpenInfo->pszFilename, &papszLines) != CE_None) {
		return NULL;
	}

	char *pszImgName = NULL;
	char **papszTok = NULL;
	papszTok = CSLTokenizeString2(papszLines[2], "\\/", CSLT_ALLOWEMPTYTOKENS
			| CSLT_STRIPLEADSPACES | CSLT_STRIPENDSPACES);
	int nPNum = CSLCount(papszTok);
	if (nPNum > 0) {
		pszImgName = CPLStrdup(papszTok[nPNum - 1]);
		CSLDestroy(papszTok);
	} else {
		CPLError(CE_Failure, CPLE_AppDefined,
				"Open(): cannot parse image file  \"%s\".", papszLines[2]);
		CSLDestroy(papszTok);
		CSLDestroy(papszLines);
		return NULL;
	}

	GDALDataset *poSrcDS;

	poSrcDS = (GDALDataset *) GDALOpenShared(pszImgName, GA_ReadOnly);
	if (!poSrcDS) {
		CPLError(CE_Failure, CPLE_AppDefined,
				"Open(): cannot open image file  \"%s\".", pszImgName);
		CPLFree(pszImgName);
		CSLDestroy(papszLines);
		return NULL;
	}
	CPLFree(pszImgName);
	OziDataset *poDS = new OziDataset(poSrcDS);

	poDS->eAccess = GA_ReadOnly;
	if (GDALParseOziMapParams(poOpenInfo->pszFilename, papszLines,
			poDS->adfGeoTransform, &poDS->pszProjectionRef, &poDS->nGCPCount,
			&poDS->pasGCPList) != CE_None) {
		CPLError(CE_Failure, CPLE_AppDefined,
				"Open(): cannot parse map params.");
		delete poDS;
		CSLDestroy(papszLines);
		return NULL;
	}

	return poDS;
}

extern "C" CPL_DLL void GDALRegister_OZI() {

	GDALDriver *poDriver;

	if (GDALGetDriverByName("OZI") == NULL) {
		poDriver = new GDALDriver();

		poDriver->SetDescription("OZI");
		poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "OZIExplorer MAP (.map)");
		poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "frmt_various.html#OZI");
		poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "map");

		poDriver->pfnOpen = OziDataset::Open;
		poDriver->pfnIdentify = OziDataset::Identify;

		GetGDALDriverManager()->RegisterDriver(poDriver);
	}
}
