/*
 * ozf_driver.cpp
 *
 *  Created on: Dec 12, 2010
 *      Author: geom
 */
#include <gdal.h>
#include <gdal_priv.h>
#include "ozf_decoder.h"

class OZFRasterBand;

class CPL_DLL OZFDataset: public GDALDataset {
	friend class OZFRasterBand;
private:
	ozf_stream* source;

public:
	virtual ~OZFDataset();

	static GDALDataset *Open(GDALOpenInfo *);
};

class CPL_DLL OZFRasterBand: public GDALRasterBand {
public:
	OZFRasterBand(OZFDataset *, int);
	virtual CPLErr IReadBlock(int, int, void *);
	virtual GDALColorInterp GetColorInterpretation();
};

OZFDataset::~OZFDataset() {
	FlushCache();
	if (source) {
		ozf_close(source);
	}
}

GDALDataset* OZFDataset::Open(GDALOpenInfo * poOpenInfo) {
	// -------------------------------------------------------------------- //
	//      Confirm the requested access is supported.                      //
	// -------------------------------------------------------------------- //
	if (poOpenInfo->eAccess == GA_Update) {
		CPLError(CE_Failure, CPLE_NotSupported,
				"The OZF driver does not support update access to existing datasets.\n");
		return NULL;
	}

	// -------------------------------------------------------------------- //
	//      Create a corresponding GDALDataset.                             //
	// -------------------------------------------------------------------- //
	OZFDataset *poDS;

	poDS = new OZFDataset();

	poDS->source = ozf_open(poOpenInfo->pszFilename);
	if (poDS->source->ozf2) {
		poDS->nRasterXSize = poDS->source->ozf2->width;
		poDS->nRasterYSize = poDS->source->ozf2->height;
	} else if (poDS->source->ozf3) {
		poDS->nRasterXSize = poDS->source->ozf3->width;
		poDS->nRasterYSize = poDS->source->ozf3->height;
	} else {
		delete poDS;
		return NULL;
	}

	poDS->eAccess = GA_ReadOnly;
	// -------------------------------------------------------------------- //
	//      Create band information objects.                                //
	// -------------------------------------------------------------------- //
	poDS->SetBand(1, new OZFRasterBand(poDS, 1));
	poDS->SetBand(2, new OZFRasterBand(poDS, 2));
	poDS->SetBand(3, new OZFRasterBand(poDS, 3));

	// -------------------------------------------------------------------- //
	//      Initialize default overviews.                                   //
	// -------------------------------------------------------------------- //
	poDS->oOvManager.Initialize(poDS, poOpenInfo->pszFilename);
	return (poDS);
}

OZFRasterBand::OZFRasterBand(OZFDataset *poDS, int nBand) {
	this->poDS = poDS;
	this->nBand = nBand;

	eDataType = GDT_Byte;

	nBlockXSize = 64;
	nBlockYSize = 64;
}

CPLErr OZFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void * pImage) {

	OZFDataset *poDS = (OZFDataset *) this->poDS;

	unsigned char* buffer = (unsigned char*) VSIMalloc(64 * 64 * 4
			* sizeof(unsigned char));

	int scale = ozf_num_scales(poDS->source) - 1;

	ozf_get_tile(poDS->source, scale, nBlockXOff, nBlockYOff, buffer);

	for (int i = 0; i < 64 * 64; i++) {
		((GByte*) pImage)[i] = buffer[i * 4 + this->nBand];
	}

	return CE_None;
}

GDALColorInterp OZFRasterBand::GetColorInterpretation() {
	switch (this->nBand) {
	case 1:
		return GCI_RedBand;
	case 2:
		return GCI_GreenBand;
	case 3:
		return GCI_BlueBand;
	default:
		return GCI_AlphaBand;
	}
}

extern "C" CPL_DLL void GDALRegister_OZF() {

	GDALDriver *poDriver;

	if (GDALGetDriverByName("OZF") == NULL) {
		poDriver = new GDALDriver();

		poDriver->SetDescription("OZF");
		poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
				"OZIExplorer OZF/OZFX (.ozf2/.ozfx3)");
		poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "frmt_various.html#OZF");
		poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "ozf2");

		poDriver->pfnOpen = OZFDataset::Open;

		GetGDALDriverManager()->RegisterDriver(poDriver);
	}
}
