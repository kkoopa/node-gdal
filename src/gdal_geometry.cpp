#include "gdal_common.hpp"

#include "gdal_spatial_reference.hpp"
#include "gdal_coordinate_transformation.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"
#include "gdal_point.hpp"
#include "gdal_linestring.hpp"
#include "gdal_linearring.hpp"
#include "gdal_polygon.hpp"
#include "gdal_multipoint.hpp"
#include "gdal_multilinestring.hpp"
#include "gdal_multipolygon.hpp"

#include "fast_buffer.hpp"

#include <node_buffer.h>
#include <sstream>
#include <stdlib.h>
#include <ogr_core.h>

namespace node_gdal {

Persistent<FunctionTemplate> Geometry::constructor;

void Geometry::Initialize(Handle<Object> target)
{
	HandleScope scope;

	constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Geometry::New));
	constructor->InstanceTemplate()->SetInternalFieldCount(1);
	constructor->SetClassName(String::NewSymbol("Geometry"));

	//NODE_SET_METHOD(constructor, "fromWKBType", Geometry::create);
	NODE_SET_METHOD(constructor, "fromWKT", Geometry::createFromWkt);
	NODE_SET_METHOD(constructor, "fromWKB", Geometry::createFromWkb);
	NODE_SET_METHOD(constructor, "getName", Geometry::getName);
	NODE_SET_METHOD(constructor, "getConstructor", Geometry::getConstructor);

	NODE_SET_PROTOTYPE_METHOD(constructor, "toString", toString);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toKML", exportToKML);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toGML", exportToGML);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toJSON", exportToJSON);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toWKT", exportToWKT);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toWKB", exportToWKB);
	NODE_SET_PROTOTYPE_METHOD(constructor, "isEmpty", isEmpty);
	NODE_SET_PROTOTYPE_METHOD(constructor, "isValid", isValid);
	NODE_SET_PROTOTYPE_METHOD(constructor, "isSimple", isSimple);
	NODE_SET_PROTOTYPE_METHOD(constructor, "isRing", isRing);
	NODE_SET_PROTOTYPE_METHOD(constructor, "clone", clone);
	NODE_SET_PROTOTYPE_METHOD(constructor, "empty", empty);
	NODE_SET_PROTOTYPE_METHOD(constructor, "closeRings", closeRings);
	NODE_SET_PROTOTYPE_METHOD(constructor, "intersects", intersects);
	NODE_SET_PROTOTYPE_METHOD(constructor, "equals", equals);
	NODE_SET_PROTOTYPE_METHOD(constructor, "disjoint", disjoint);
	NODE_SET_PROTOTYPE_METHOD(constructor, "touches", touches);
	NODE_SET_PROTOTYPE_METHOD(constructor, "crosses", crosses);
	NODE_SET_PROTOTYPE_METHOD(constructor, "within", within);
	NODE_SET_PROTOTYPE_METHOD(constructor, "contains", contains);
	NODE_SET_PROTOTYPE_METHOD(constructor, "overlaps", overlaps);
	NODE_SET_PROTOTYPE_METHOD(constructor, "boundary", boundary);
	NODE_SET_PROTOTYPE_METHOD(constructor, "distance", distance);
	NODE_SET_PROTOTYPE_METHOD(constructor, "convexHull", convexHull);
	NODE_SET_PROTOTYPE_METHOD(constructor, "buffer", buffer);
	NODE_SET_PROTOTYPE_METHOD(constructor, "intersection", intersection);
	NODE_SET_PROTOTYPE_METHOD(constructor, "union", unionGeometry);
	NODE_SET_PROTOTYPE_METHOD(constructor, "difference", difference);
	NODE_SET_PROTOTYPE_METHOD(constructor, "symDifference", symDifference);
	NODE_SET_PROTOTYPE_METHOD(constructor, "centroid", centroid);
	NODE_SET_PROTOTYPE_METHOD(constructor, "simplify", simplify);
	NODE_SET_PROTOTYPE_METHOD(constructor, "simplifyPreserveTopology", simplifyPreserveTopology);
	NODE_SET_PROTOTYPE_METHOD(constructor, "segmentize", segmentize);
	NODE_SET_PROTOTYPE_METHOD(constructor, "swapXY", swapXY);
	NODE_SET_PROTOTYPE_METHOD(constructor, "getEnvelope", getEnvelope);
	NODE_SET_PROTOTYPE_METHOD(constructor, "getEnvelope3D", getEnvelope3D);
	NODE_SET_PROTOTYPE_METHOD(constructor, "transform", transform);
	NODE_SET_PROTOTYPE_METHOD(constructor, "transformTo", transformTo);

	ATTR(constructor, "srs", srsGetter, srsSetter);
	ATTR(constructor, "wkbSize", wkbSizeGetter, READ_ONLY_SETTER);
	ATTR(constructor, "dimension", dimensionGetter, READ_ONLY_SETTER);
	ATTR(constructor, "coordinateDimension", coordinateDimensionGetter, READ_ONLY_SETTER);
	ATTR(constructor, "wkbType", typeGetter, READ_ONLY_SETTER);
	ATTR(constructor, "name", nameGetter, READ_ONLY_SETTER);

	target->Set(String::NewSymbol("Geometry"), constructor->GetFunction());
}

Geometry::Geometry(OGRGeometry *geom)
	: ObjectWrap(),
	  this_(geom),
	  owned_(true),
	  size_(0)
{
	LOG("Created Geometry [%p]", geom);
}

Geometry::Geometry()
	: ObjectWrap(),
	  this_(NULL),
	  owned_(true),
	  size_(0)
{
}

Geometry::~Geometry()
{
	if(this_) {
		LOG("Disposing Geometry [%p] (%s)", this_, owned_ ? "owned" : "unowned");
		if (owned_) {
			OGRGeometryFactory::destroyGeometry(this_);
			V8::AdjustAmountOfExternalAllocatedMemory(-size_);
		}
		LOG("Disposed Geometry [%p]", this_)
		this_ = NULL;
	}
}

Handle<Value> Geometry::New(const Arguments& args)
{
	HandleScope scope;
	Geometry *f;

	if (!args.IsConstructCall()) {
		return NODE_THROW("Cannot call constructor as function, you need to use 'new' keyword");
	}

	if (args[0]->IsExternal()) {
		Local<External> ext = Local<External>::Cast(args[0]);
		void* ptr = ext->Value();
		f = static_cast<Geometry *>(ptr);

	} else {
		return NODE_THROW("Geometry doesnt have a constructor, use Geometry.fromWKT(), Geometry.fromWKB() or type-specific constructor. ie. new ogr.Point()");
		//OGRwkbGeometryType geometry_type;
		//NODE_ARG_ENUM(0, "geometry type", OGRwkbGeometryType, geometry_type);
		//OGRGeometry *geom = OGRGeometryFactory::createGeometry(geometry_type);
		//f = new Geometry(geom);
	}

	f->Wrap(args.This());
	return args.This();
}

Handle<Value> Geometry::New(OGRGeometry *geom)
{
	HandleScope scope;
	return scope.Close(Geometry::New(geom, true));
}

Handle<Value> Geometry::New(OGRGeometry *geom, bool owned)
{
	HandleScope scope;

	if (!geom) {
		return Null();
	}

	OGRwkbGeometryType type = getGeometryType_fixed(geom);
	type = wkbFlatten(type);

	switch (type) {
		case wkbPoint:
			return scope.Close(Point::New(static_cast<OGRPoint*>(geom), owned));
		case wkbLineString:
			return scope.Close(LineString::New(static_cast<OGRLineString*>(geom), owned));
		case wkbLinearRing:
			return scope.Close(LinearRing::New(static_cast<OGRLinearRing*>(geom), owned));
		case wkbPolygon:
			return scope.Close(Polygon::New(static_cast<OGRPolygon*>(geom), owned));
		case wkbGeometryCollection:
			return scope.Close(GeometryCollection::New(static_cast<OGRGeometryCollection*>(geom), owned));
		case wkbMultiPoint:
			return scope.Close(MultiPoint::New(static_cast<OGRMultiPoint*>(geom), owned));
		case wkbMultiLineString:
			return scope.Close(MultiLineString::New(static_cast<OGRMultiLineString*>(geom), owned));
		case wkbMultiPolygon:
			return scope.Close(MultiPolygon::New(static_cast<OGRMultiPolygon*>(geom), owned));
		default:
			return NODE_THROW("Tried to create unsupported geometry type");
	}
}

OGRwkbGeometryType Geometry::getGeometryType_fixed(OGRGeometry* geom)
{
	//For some reason OGRLinearRing::getGeometryType uses OGRLineString's method...
	//meaning OGRLinearRing::getGeometryType returns wkbLineString

	//http://trac.osgeo.org/gdal/ticket/1755

	OGRwkbGeometryType type = geom->getGeometryType();

	if (std::string(geom->getGeometryName()) == "LINEARRING") {
		type = (OGRwkbGeometryType) (wkbLinearRing | (type & wkb25DBit));
	}

	return type;
}

Handle<Value> Geometry::toString(const Arguments& args)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());
	std::ostringstream ss;
	ss << "Geometry (" << geom->this_->getGeometryName() << ")";
	return scope.Close(String::New(ss.str().c_str()));
}


NODE_WRAPPED_METHOD(Geometry, closeRings, closeRings);
NODE_WRAPPED_METHOD(Geometry, empty, empty);
NODE_WRAPPED_METHOD(Geometry, swapXY, swapXY);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, isEmpty, Boolean, IsEmpty);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, isValid, Boolean, IsValid);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, isSimple, Boolean, IsSimple);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, isRing, Boolean, IsRing);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, clone, Geometry, clone);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, convexHull, Geometry, ConvexHull);
NODE_WRAPPED_METHOD_WITH_RESULT(Geometry, boundary, Geometry, Boundary);
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, intersects, Boolean, Intersects, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, equals, Boolean, Equals, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, disjoint, Boolean, Disjoint, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, touches, Boolean, Touches, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, crosses, Boolean, Crosses, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, within, Boolean, Within, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, contains, Boolean, Contains, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, overlaps, Boolean, Overlaps, Geometry, "geometry to compare");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, distance, Number, Distance, Geometry, "geometry to use for distance calculation");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, intersection, Geometry, Intersection, Geometry, "geometry to use for intersection");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, unionGeometry, Geometry, Union, Geometry, "geometry to use for union");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, difference, Geometry, Difference, Geometry, "geometry to use for difference");
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Geometry, symDifference, Geometry, SymDifference, Geometry, "geometry to use for sym difference");
NODE_WRAPPED_METHOD_WITH_RESULT_1_DOUBLE_PARAM(Geometry, simplify, Geometry, Simplify, "tolerance");
NODE_WRAPPED_METHOD_WITH_RESULT_1_DOUBLE_PARAM(Geometry, simplifyPreserveTopology, Geometry, SimplifyPreserveTopology, "tolerance");
NODE_WRAPPED_METHOD_WITH_1_DOUBLE_PARAM(Geometry, segmentize, segmentize, "segment length");
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(Geometry, transform, transform, CoordinateTransformation, "transform");
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(Geometry, transformTo, transformTo, SpatialReference, "spatial reference");

//manually wrap this method because we don't have macros for multiple params
Handle<Value> Geometry::buffer(const Arguments& args)
{
	HandleScope scope;

	double distance;
	int number_of_segments = 30;

	NODE_ARG_DOUBLE(0, "distance", distance);
	NODE_ARG_INT_OPT(1, "number of segments", number_of_segments);

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	return scope.Close(Geometry::New(geom->this_->Buffer(distance, number_of_segments)));
}


Handle<Value> Geometry::exportToWKT(const Arguments& args)
{
	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	char *text = NULL;
	OGRErr err = geom->this_->exportToWkt(&text);

	if (err) {
		return NODE_THROW_OGRERR(err);
	}
	if (text) {
		return scope.Close(SafeString::New(text));
	}

	return Undefined();
}

Handle<Value> Geometry::exportToWKB(const Arguments& args)
{
	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	int size = geom->this_->WkbSize();
	unsigned char *data = (unsigned char*) malloc(size);

	//byte order
	OGRwkbByteOrder byte_order;
	std::string order = "MSB";
	NODE_ARG_OPT_STR(0, "byte order", order);
	if (order == "MSB") {
		byte_order = wkbXDR;
	} else if (order == "LSB") {
		byte_order = wkbNDR;
	} else {
		return NODE_THROW("byte order must be 'MSB' or 'LSB'");
	}

	#if GDAL_VERSION_MAJOR > 1 || (GDAL_VERSION_MINOR > 10)
	//wkb variant
	OGRwkbVariant wkb_variant;
	std::string variant = "OGC";
	NODE_ARG_OPT_STR(1, "wkb variant", variant);
	if (variant == "OGC") {
		wkb_variant = wkbVariantOgc;
	} else if (order == "ISO") {
		wkb_variant = wkbVariantIso;
	} else {
		return NODE_THROW("byte order must be 'OGC' or 'ISO'");
	}
	OGRErr err = geom->this_->exportToWkb(byte_order, data, wkb_variant);
	#else
	OGRErr err = geom->this_->exportToWkb(byte_order, data);
	#endif

	//^^ export to wkb and fill buffer ^^
	//TODO: avoid extra memcpy in FastBuffer::New and have exportToWkb write directly into buffer

	if (err) {
		free(data);
		return NODE_THROW_OGRERR(err);
	}

	Handle<Value> result = FastBuffer::New(data, size);
	free(data);

	return scope.Close(result);

}

Handle<Value> Geometry::exportToKML(const Arguments& args)
{
	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	char *text = geom->this_->exportToKML();
	if (text) {
		Handle<Value> result = String::New(text);
		CPLFree(text);
		return scope.Close(result);
	}

	return Undefined();
}

Handle<Value> Geometry::exportToGML(const Arguments& args)
{
	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	char *text = geom->this_->exportToGML();
	if (text) {
		Handle<Value> result = String::New(text);
		CPLFree(text);
		return scope.Close(result);
	}

	return Undefined();
}

Handle<Value> Geometry::exportToJSON(const Arguments& args)
{
	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	char *text = geom->this_->exportToJson();
	if (text) {
		Handle<Value> result = String::New(text);
		CPLFree(text);
		return scope.Close(result);
	}

	return Undefined();
}

// The Centroid method wants the caller to create the point to fill in. Instead
// of requiring the caller to create the point geometry to fill in, we new up an
// OGRPoint and put the result into it and return that.
Handle<Value> Geometry::centroid(const Arguments& args)
{
	HandleScope scope;
	OGRPoint *point = new OGRPoint();

	ObjectWrap::Unwrap<Geometry>(args.This())->this_->Centroid(point);

	return scope.Close(Point::New(point));
}

Handle<Value> Geometry::getEnvelope(const Arguments& args)
{
	//returns object containing boundaries until complete OGREnvelope binding is built

	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	OGREnvelope *envelope = new OGREnvelope();
	geom->this_->getEnvelope(envelope);

	Local<Object> obj = Object::New();
	obj->Set(String::NewSymbol("minX"), Number::New(envelope->MinX));
	obj->Set(String::NewSymbol("maxX"), Number::New(envelope->MaxX));
	obj->Set(String::NewSymbol("minY"), Number::New(envelope->MinY));
	obj->Set(String::NewSymbol("maxY"), Number::New(envelope->MaxY));

	delete envelope;

	return scope.Close(obj);
}

Handle<Value> Geometry::getEnvelope3D(const Arguments& args)
{
	//returns object containing boundaries until complete OGREnvelope binding is built

	HandleScope scope;

	Geometry *geom = ObjectWrap::Unwrap<Geometry>(args.This());

	OGREnvelope3D *envelope = new OGREnvelope3D();
	geom->this_->getEnvelope(envelope);

	Local<Object> obj = Object::New();
	obj->Set(String::NewSymbol("minX"), Number::New(envelope->MinX));
	obj->Set(String::NewSymbol("maxX"), Number::New(envelope->MaxX));
	obj->Set(String::NewSymbol("minY"), Number::New(envelope->MinY));
	obj->Set(String::NewSymbol("maxY"), Number::New(envelope->MaxY));
	obj->Set(String::NewSymbol("minZ"), Number::New(envelope->MinZ));
	obj->Set(String::NewSymbol("maxZ"), Number::New(envelope->MaxZ));

	delete envelope;

	return scope.Close(obj);
}

// --- JS static methods (OGRGeometryFactory) ---

Handle<Value> Geometry::createFromWkt(const Arguments &args)
{
	HandleScope scope;

	std::string wkt_string;
	SpatialReference *srs = NULL;

	NODE_ARG_STR(0, "wkt", wkt_string);
	NODE_ARG_WRAPPED_OPT(1, "srs", SpatialReference, srs);

	char *wkt = (char*) wkt_string.c_str();
	OGRGeometry *geom = NULL;
	OGRSpatialReference *ogr_srs = NULL;
	if (srs) {
		ogr_srs = srs->get();
	}

	OGRErr err = OGRGeometryFactory::createFromWkt(&wkt, ogr_srs, &geom);
	if (err) {
		return NODE_THROW_OGRERR(err);
	}

	return scope.Close(Geometry::New(geom, true));
}

Handle<Value> Geometry::createFromWkb(const Arguments &args)
{
	HandleScope scope;

	std::string wkb_string;
	SpatialReference *srs = NULL;

	Handle<Object> wkb_obj;
	NODE_ARG_OBJECT(0, "wkb", wkb_obj);
	NODE_ARG_WRAPPED_OPT(1, "srs", SpatialReference, srs);

	std::string obj_type = TOSTR(wkb_obj->GetConstructorName());

	if(obj_type != "Buffer"){
		return NODE_THROW("Argument must be a buffer object");
	}

	unsigned char* data = (unsigned char *) Buffer::Data(wkb_obj);
	size_t length = Buffer::Length(wkb_obj);

	OGRGeometry *geom = NULL;
	OGRSpatialReference *ogr_srs = NULL;
	if (srs) {
		ogr_srs = srs->get();
	}

	OGRErr err = OGRGeometryFactory::createFromWkb(data, ogr_srs, &geom, length);
	if (err) {
		return NODE_THROW_OGRERR(err);
	}

	return scope.Close(Geometry::New(geom, true));
}

Handle<Value> Geometry::create(const Arguments &args)
{
	HandleScope scope;

	OGRwkbGeometryType type = wkbUnknown;
	NODE_ARG_ENUM(0, "type", OGRwkbGeometryType, type);

	return scope.Close(Geometry::New(OGRGeometryFactory::createGeometry(type), true));
}

Handle<Value> Geometry::srsGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(SpatialReference::New(geom->this_->getSpatialReference(), false));
}

void Geometry::srsSetter(Local<String> property, Local<Value> value, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());

	OGRSpatialReference *srs = NULL;
	if (IS_WRAPPED(value, SpatialReference)) {
		SpatialReference *srs_obj = ObjectWrap::Unwrap<SpatialReference>(value->ToObject());
		srs = srs_obj->get();
	} else if (!value->IsNull() && !value->IsUndefined()) {
		NODE_THROW("srs must be SpatialReference object");
		return;
	}

	geom->this_->assignSpatialReference(srs);
}

Handle<Value> Geometry::nameGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(SafeString::New(geom->this_->getGeometryName()));
}

Handle<Value> Geometry::typeGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(Integer::New(getGeometryType_fixed(geom->this_)));
}

Handle<Value> Geometry::wkbSizeGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(Integer::New(geom->this_->WkbSize()));
}

Handle<Value> Geometry::dimensionGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(Integer::New(geom->this_->getDimension()));
}

Handle<Value> Geometry::coordinateDimensionGetter(Local<String> property, const AccessorInfo &info)
{
	HandleScope scope;
	Geometry *geom = ObjectWrap::Unwrap<Geometry>(info.This());
	return scope.Close(Integer::New(geom->this_->getCoordinateDimension()));
}

Handle<Value> Geometry::getConstructor(OGRwkbGeometryType type){
	type = wkbFlatten(type);
	switch (type) {
		case wkbPoint:              return Point::constructor->GetFunction();
		case wkbLineString:         return LineString::constructor->GetFunction();
		case wkbLinearRing:         return LinearRing::constructor->GetFunction();
		case wkbPolygon:            return Polygon::constructor->GetFunction();
		case wkbGeometryCollection: return GeometryCollection::constructor->GetFunction();
		case wkbMultiPoint:         return MultiPoint::constructor->GetFunction();
		case wkbMultiLineString:    return MultiLineString::constructor->GetFunction();
		case wkbMultiPolygon:       return MultiPolygon::constructor->GetFunction();
		default:                    return Null();
	}
}

Handle<Value> Geometry::getConstructor(const Arguments &args){
	HandleScope scope;
	OGRwkbGeometryType type;
	NODE_ARG_ENUM(0, "wkbType", OGRwkbGeometryType, type);
	return scope.Close(getConstructor(type));
}

Handle<Value> Geometry::getName(const Arguments &args){
	HandleScope scope;
	OGRwkbGeometryType type;
	NODE_ARG_ENUM(0, "wkbType", OGRwkbGeometryType, type);
	return scope.Close(SafeString::New(OGRGeometryTypeToName(type)));
}

} // namespace node_gdal