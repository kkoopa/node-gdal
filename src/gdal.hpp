#ifndef __NODE_GDAL_GLOBAL_H__
#define __NODE_GDAL_GLOBAL_H__

// v8
#include <v8.h>

// node
#include <node.h>

// ogr
#include <ogr_api.h>
#include <ogrsf_frmts.h>

// gdal
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "gdal_dataset.hpp"

using namespace v8;
using namespace node;

namespace node_gdal {

	static Handle<Value> open(const Arguments &args)
	{
		HandleScope scope;

		std::string path;
		std::string mode = "r";
		GDALAccess access = GA_ReadOnly;

		NODE_ARG_STR(0, "path", path);
		NODE_ARG_OPT_STR(1, "update", mode);

		if (mode == "r+") {
			access = GA_Update;
		} else if (mode != "r") {
			return NODE_THROW("Invalid open mode. Must be \"r\" or \"r+\"");
		}

		OGRDataSource *ogr_ds = OGRSFDriverRegistrar::Open(path.c_str(), static_cast<int>(access));
		if(ogr_ds) {
			return scope.Close(Dataset::New(ogr_ds));
		}

		GDALDataset *gdal_ds = (GDALDataset*) GDALOpen(path.c_str(), access);
		if(gdal_ds) {
			return scope.Close(Dataset::New(gdal_ds));
		}

		return NODE_THROW("Error opening dataset");
	}

	static Handle<Value> setConfigOption(const Arguments &args)
	{
		HandleScope scope;

		std::string name;
		std::string val;

		NODE_ARG_STR(0, "name", name);

		if (args.Length() < 2) {
			return NODE_THROW("string or null value must be provided");
		}
		if(args[1]->IsString()){
			val = TOSTR(args[1]);
			CPLSetConfigOption(name.c_str(), val.c_str());
		} else if(args[1]->IsNull() || args[1]->IsUndefined()) {
			CPLSetConfigOption(name.c_str(), NULL);
		} else {
			return NODE_THROW("value must be a string or null");
		}

		return Undefined();
	}

	static Handle<Value> getConfigOption(const Arguments &args)
	{
		HandleScope scope;

		std::string name;
		NODE_ARG_STR(0, "name", name);

		return scope.Close(SafeString::New(CPLGetConfigOption(name.c_str(), NULL)));
	}

	static Handle<Value> decToDMS(const Arguments &args){
		HandleScope scope;

		double angle;
		std::string axis;
		int precision = 2;
		NODE_ARG_DOUBLE(0, "angle", angle);
		NODE_ARG_STR(1, "axis", axis);
		NODE_ARG_INT_OPT(2, "precision", precision);

		if (axis.length() > 0) {
			axis[0] = toupper(axis[0]);
		}
		if (axis != "Lat" && axis != "Long") {
			return NODE_THROW("Axis must be 'lat' or 'long'");
		}

		return scope.Close(SafeString::New(GDALDecToDMS(angle, axis.c_str(), precision)));
	}
}

#endif
