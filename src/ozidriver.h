/*
 * ozidriver.h
 *
 *  Created on: Jul 31, 2010
 *      Author: geom
 */

#ifndef OZIDRIVER_H_
#define OZIDRIVER_H_

#include <gdal.h>
#include <ogr_spatialref.h>
#include <cpl_port.h>

CPL_C_START

int CPL_DLL CPL_STDCALL ImportFromOzi(OGRSpatialReference *pSRS, char *pszDatum,
		char *pszProj, char *pszProjParms, int* targetEPSG);

CPL_C_END

#endif /* OZIDRIVER_H_ */
