
#ifndef __GDAL_COMMON_H__
#define __GDAL_COMMON_H__

#include <v8.h>
#include <gdal_version.h>
#include <cpl_error.h>
#include <stdio.h>

namespace node_gdal {
	extern FILE *log_file;
}

#ifdef ENABLE_LOGGING
#define LOG(fmt, ...) if (node_gdal::log_file) { fprintf(node_gdal::log_file, fmt"\n", __VA_ARGS__); fflush(node_gdal::log_file); }
#else
#define LOG(fmt, ...)
#endif

//String::New(null) -> seg fault
class SafeString {
public:
	static v8::Handle<v8::Value> New(const char * data) {
		if (!data) {
			return v8::Null();
		} else {
			return v8::String::New(data);
		}
	}
};

inline const char* getOGRErrMsg(int err)
{
  if(err == 6) {
    //get more descriptive error
    //TODO: test if all OGRErr failures report an error msg
    return CPLGetLastErrorMsg();
  }
  switch(err) {
  case 0:
    return "No error";
  case 1:
    return "Not enough data";
  case 2:
    return "Not enough memory";
  case 3:
    return "Unsupported geometry type";
  case 4:
    return "Unsupported operation";
  case 5:
    return "Corrupt Data";
  case 6:
    return "Failure";
  case 7:
    return "Unsupported SRS";
  default:
    return "Invalid Error";
  }
};

#define TOSTR(obj) (*String::Utf8Value((obj)->ToString()))

#define NODE_THROW(msg) ThrowException(Exception::Error(String::New(msg)));

#define NODE_THROW_CPLERR(err) ThrowException(Exception::Error(String::New(CPLGetLastErrorMsg())));

#define NODE_THROW_LAST_CPLERR NODE_THROW_CPLERR

#define NODE_THROW_OGRERR(err) ThrowException(Exception::Error(String::New(getOGRErrMsg(err))));

#define ATTR(t, name, get, set)                                         \
    t->InstanceTemplate()->SetAccessor(String::NewSymbol(name), get, set);

#define ATTR_DONT_ENUM(t, name, get, set)                                         \
    t->InstanceTemplate()->SetAccessor(String::NewSymbol(name), get, set, Handle<Value>(), DEFAULT, DontEnum);

void READ_ONLY_SETTER(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info);

template <typename T, typename K>
class ClosedPtr {
public:
	static v8::Handle<v8::Value> Closed(K *raw) {
		if (!raw) {
			return v8::Null();
		}
		v8::HandleScope scope;
		T *wrapped = new T(raw);
		v8::Handle<v8::Value> ext = v8::External::New(wrapped);
		v8::Handle<v8::Object> obj = T::constructor->GetFunction()->NewInstance(1, &ext);
		return scope.Close(obj);
	}
};

#define IS_WRAPPED(obj, type) type::constructor->HasInstance(obj)

// ----- object property conversion -------

#define NODE_DOUBLE_FROM_OBJ(obj, key, var)                                                               \
{                                                                                                         \
  Local<String> sym = String::NewSymbol(key);                                                             \
  if (!obj->HasOwnProperty(sym)){                                                                                    \
     return ThrowException(Exception::Error(String::New("Object must contain property \"" key "\"")));    \
  }                                                                                                       \
  Local<Value> val = obj->Get(sym);                                                                       \
  if (!val->IsNumber()){                                                                                  \
    return ThrowException(Exception::Error(String::New("Property \"" key "\" must be a number")));        \
  }                                                                                                       \
  var = val->NumberValue();                                                                               \
}

#define NODE_STR_FROM_OBJ(obj, key, var)                                                                  \
{                                                                                                         \
  Local<String> sym = String::NewSymbol(key);                                                             \
  if (!obj->HasOwnProperty(sym)){                                                                                    \
     return ThrowException(Exception::Error(String::New("Object must contain property \"" key "\"")));    \
  }                                                                                                       \
  Local<Value> val = obj->Get(sym);                                                                       \
  if (!val->IsString()){                                                                                  \
    return ThrowException(Exception::Error(String::New("Property \"" key "\" must be a string")));        \
  }                                                                                                       \
  var = (*String::Utf8Value(val->ToString()));                                                            \
}

#define NODE_DOUBLE_FROM_OBJ_OPT(obj, key, var)                                                           \
{                                                                                                         \
  Local<String> sym = String::NewSymbol(key);                                                             \
  if (obj->HasOwnProperty(sym)){                                                                                     \
    Local<Value> val = obj->Get(sym);                                                                     \
    if (!val->IsNumber()){                                                                                \
      return ThrowException(Exception::Error(String::New("Property \"" key "\" must be a number")));      \
    }                                                                                                     \
    var = val->NumberValue();                                                                             \
  }                                                                                                       \
}

#define NODE_STR_FROM_OBJ_OPT(obj, key, var)                                                              \
{                                                                                                         \
  Local<String> sym = String::NewSymbol(key);                                                             \
  if (obj->HasOwnProperty(sym)){                                                                                     \
    Local<Value> val = obj->Get(sym);                                                                     \
    if (!val->IsString()){                                                                                \
      return ThrowException(Exception::Error(String::New("Property \"" key "\" must be a string")));      \
    }                                                                                                     \
    var = (*String::Utf8Value(val->ToString()));                                                          \
  }                                                                                                       \
}

// ----- argument conversion -------

//determine field index based on string/numeric js argument
//f -> OGRFeature* or OGRFeatureDefn*

#define ARG_FIELD_ID(num, f, var) {                                    \
  if (args[num]->IsString()) {                                         \
    std::string field_name = TOSTR(args[num]);                         \
    var = f->GetFieldIndex(field_name.c_str());                        \
    if (field_index == -1) {                                           \
      return NODE_THROW("Specified field name does not exist");        \
    }                                                                  \
  } else if (args[num]->IsInt32()) {                                   \
    var = args[num]->Int32Value();                                     \
    if (var < 0 || var >= f->GetFieldCount()) {                        \
      return NODE_THROW("Invalid field index");                        \
    }                                                                  \
  } else {                                                             \
    return NODE_THROW("Field index must be integer or string");        \
  }                                                                    \
}

#define NODE_ARG_INT(num, name, var)                                                                           \
  if (args.Length() < num + 1) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));      \
  }                                                                                                            \
  if (!args[num]->IsNumber()) {                                                                                \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an integer").c_str()))); \
  }                                                                                                            \
  var = static_cast<int>(args[num]->IntegerValue());


#define NODE_ARG_ENUM(num, name, enum_type, var)                                                                                       \
  if (args.Length() < num + 1) {                                                                                                       \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));                              \
  }                                                                                                                                    \
  if (!args[num]->IsInt32()) {                                                                                                         \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be of type " + std::string(#enum_type)).c_str()))); \
  }                                                                                                                                    \
  var = enum_type(args[num]->IntegerValue());


#define NODE_ARG_BOOL(num, name, var)                                                                          \
  if (args.Length() < num + 1) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));      \
  }                                                                                                            \
  if (!args[num]->IsBoolean()) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an boolean").c_str()))); \
  }                                                                                                            \
  var = static_cast<bool>(args[num]->BooleanValue());


#define NODE_ARG_DOUBLE(num, name, var)                                                                      \
  if (args.Length() < num + 1) {                                                                             \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));    \
  }                                                                                                          \
  if (!args[num]->IsNumber()) {                                                                              \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be a number").c_str()))); \
  }                                                                                                          \
  var = static_cast<double>(args[num]->NumberValue());


#define NODE_ARG_ARRAY(num, name, var)                                                                       \
  if (args.Length() < num + 1) {                                                                             \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));    \
  }                                                                                                          \
  if (!args[num]->IsArray()) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an array").c_str()))); \
  }                                                                                                          \
  var = Handle<Array>::Cast(args[num]);


#define NODE_ARG_OBJECT(num, name, var)                                                                       \
  if (args.Length() < num + 1) {                                                                              \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));     \
  }                                                                                                           \
  if (!args[num]->IsObject()) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an object").c_str()))); \
  }                                                                                                           \
  var = Handle<Object>::Cast(args[num]);


#define NODE_ARG_WRAPPED(num, name, type, var)                                                                                           \
  if (args.Length() < num + 1) {                                                                                                         \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));                                \
  }                                                                                                                                      \
  if (args[num]->IsNull() || args[num]->IsUndefined() || !type::constructor->HasInstance(args[num])) {                                   \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an instance of " + std::string(#type)).c_str()))); \
  }                                                                                                                                      \
  var = ObjectWrap::Unwrap<type>(args[num]->ToObject());                                                                                 \
  if (!var->get()) return ThrowException(Exception::Error(String::New(#type" parameter already destroyed")));


#define NODE_ARG_STR(num, name, var)                                                                          \
  if (args.Length() < num + 1) {                                                                              \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be given").c_str())));     \
  }                                                                                                           \
  if (!args[num]->IsString()) {                                                                               \
    return ThrowException(Exception::Error(String::New((std::string(name) + " must be an string").c_str()))); \
  }                                                                                                           \
  var = (*String::Utf8Value((args[num])->ToString()))

// ----- optional argument conversion -------

#define NODE_ARG_INT_OPT(num, name, var)                                                                         \
  if (args.Length() > num) {                                                                                     \
    if (args[num]->IsInt32()) {                                                                                  \
      var = static_cast<int>(args[num]->IntegerValue());                                                         \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                               \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an integer").c_str()))); \
    }                                                                                                            \
  }


#define NODE_ARG_ENUM_OPT(num, name, enum_type, var)                                                             \
  if (args.Length() > num) {                                                                                     \
    if (args[num]->IsInt32()) {                                                                                  \
      var = static_cast<enum_type>(args[num]->IntegerValue());                                                   \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                               \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an integer").c_str()))); \
    }                                                                                                            \
  }


#define NODE_ARG_BOOL_OPT(num, name, var)                                                                        \
  if (args.Length() > num) {                                                                                     \
    if (args[num]->IsBoolean()) {                                                                                \
      var = static_cast<bool>(args[num]->BooleanValue());                                                        \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                               \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an boolean").c_str()))); \
    }                                                                                                            \
  }


#define NODE_ARG_OPT_STR(num, name, var)                                                                        \
  if (args.Length() > num) {                                                                                    \
    if (args[num]->IsString()) {                                                                                \
      var = TOSTR(args[num]);                                                                                   \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                              \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an string").c_str()))); \
    }                                                                                                           \
  }


#define NODE_ARG_DOUBLE_OPT(num, name, var)                                                                    \
  if (args.Length() > num) {                                                                                   \
    if (args[num]->IsNumber()) {                                                                               \
      var = static_cast<double>(args[num]->NumberValue());                                                     \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                             \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be a number").c_str()))); \
    }                                                                                                          \
  }


#define NODE_ARG_WRAPPED_OPT(num, name, type, var)                                                                                         \
  if (args.Length() > num && !args[num]->IsNull() && !args[num]->IsUndefined()) {                                                          \
    if (!type::constructor->HasInstance(args[num])) {                                                                                      \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an instance of " + std::string(#type)).c_str()))); \
    }                                                                                                                                      \
    var = ObjectWrap::Unwrap<type>(args[num]->ToObject());                                                                                 \
    if (!var->get()) return ThrowException(Exception::Error(String::New(#type" parameter already destroyed")));                            \
  }


#define NODE_ARG_ARRAY_OPT(num, name, var)                                                                     \
  if (args.Length() > num) {                                                                                   \
    if (args[num]->IsArray()) {                                                                                \
      var = Handle<Array>::Cast(args[num]);                                                                    \
    } else if(!args[num]->IsNull() && !args[num]->IsUndefined()) {                                             \
      return ThrowException(Exception::Error(String::New((std::string(name) + " must be an array").c_str()))); \
    }                                                                                                          \
  }

// ----- wrapped methods w/ results-------

#define NODE_WRAPPED_METHOD_WITH_RESULT(klass, method, result_type, wrapped_method)                               \
Handle<Value> klass::method(const Arguments& args)                                                                \
{                                                                                                                 \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                            \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed"); \
  return HandleScope().Close(result_type::New(obj->this_->wrapped_method()));     \
}


#define NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(klass, method, result_type, wrapped_method, param_type, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                                          \
{                                                                                                                           \
  HandleScope scope;                                                                                                        \
  param_type *param;                                                                                                        \
  NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                             \
  if (!obj->this_)   return NODE_THROW(#klass" object has already been destroyed");                \
  return scope.Close(result_type::New(obj->this_->wrapped_method(param->get())));                  \
}

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_ENUM_PARAM(klass, method, result_type, wrapped_method, enum_type, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                                      \
{                                                                                                                       \
  HandleScope scope;                                                                                                    \
  enum_type param;                                                                                                      \
  NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                             \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                  \
  return scope.Close(result_type::New(obj->this_->wrapped_method(param)));                         \
}


#define NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM(klass, method, result_type, wrapped_method, param_name)        \
Handle<Value> klass::method(const Arguments& args)                                                                    \
{                                                                                                                     \
  HandleScope scope;                                                                                                  \
  std::string param;                                                                                                  \
  NODE_ARG_STR(0, #param_name, param);                                                                                \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                             \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                  \
  return scope.Close(result_type::New(obj->this_->wrapped_method(param.c_str())));                 \
}


#define NODE_WRAPPED_METHOD_WITH_RESULT_1_INTEGER_PARAM(klass, method, result_type, wrapped_method, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                              \
{                                                                                                               \
  HandleScope scope;                                                                                            \
  int param;                                                                                                    \
  NODE_ARG_INT(0, #param_name, param);                                                                          \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                          \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                               \
  return scope.Close(result_type::New(obj->this_->wrapped_method(param)));                                      \
}


#define NODE_WRAPPED_METHOD_WITH_RESULT_1_DOUBLE_PARAM(klass, method, result_type, wrapped_method, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                             \
{                                                                                                              \
  HandleScope scope;                                                                                           \
  double param;                                                                                                \
  NODE_ARG_DOUBLE(0, #param_name, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                         \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                              \
  return scope.Close(result_type::New(obj->this_->wrapped_method(param)));                                     \
}

// ----- wrapped methods w/ CPLErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT(klass, method, wrapped_method)                                       \
Handle<Value> klass::method(const Arguments& args)                                                                  \
{                                                                                                                   \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                              \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                   \
  int err = obj->this_->wrapped_method();                                                                           \
  if (err) return NODE_THROW_CPLERR(err);                                                                           \
  return Undefined();                                                                                               \
}


#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                                          \
{                                                                                                                           \
  HandleScope scope;                                                                                                        \
  param_type *param;                                                                                                        \
  NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                           \
  int err = obj->this_->wrapped_method(param->get());                                                                       \
  if (err) return NODE_THROW_CPLERR(err);                                                                                   \
  return Undefined();                                                                                                       \
}


#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)              \
Handle<Value> klass::method(const Arguments& args)                                                                    \
{                                                                                                                     \
  HandleScope scope;                                                                                                  \
  std::string param;                                                                                                  \
  NODE_ARG_STR(0, #param_name, param);                                                                                \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                                \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                     \
  int err = obj->this_->wrapped_method(param.c_str());                                                                \
  if (err) return NODE_THROW_CPLERR(err);                                                                             \
  return Undefined();                                                                                                 \
}


#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                              \
{                                                                                                               \
  HandleScope scope;                                                                                            \
  int param;                                                                                                    \
  NODE_ARG_INT(0, #param_name, param);                                                                          \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                          \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                               \
  int err = obj->this_->wrapped_method(param);                                                                  \
  if (err) return NODE_THROW_CPLERR(err);                                                                       \
  return Undefined();                                                                                           \
}


#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                             \
{                                                                                                              \
  HandleScope scope;                                                                                           \
  double param;                                                                                                \
  NODE_ARG_DOUBLE(0, #param_name, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                         \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                              \
  int err =obj->this_->wrapped_method(param);                                                                  \
  if (err) return NODE_THROW_CPLERR(err);                                                                      \
  return Undefined();                                                                                          \
}

// ----- wrapped methods w/ OGRErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(klass, method, wrapped_method)                                       \
Handle<Value> klass::method(const Arguments& args)                                                                  \
{                                                                                                                   \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                              \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                   \
  int err = obj->this_->wrapped_method();                                                                           \
  if (err) return NODE_THROW_OGRERR(err);                                                                           \
  return Undefined();                                                                                               \
}


#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                                          \
{                                                                                                                           \
  HandleScope scope;                                                                                                        \
  param_type *param;                                                                                                        \
  NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                           \
  int err = obj->this_->wrapped_method(param->get());                                                                       \
  if (err) return NODE_THROW_OGRERR(err);                                                                                   \
  return Undefined();                                                                                                       \
}


#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)              \
Handle<Value> klass::method(const Arguments& args)                                                                    \
{                                                                                                                     \
  HandleScope scope;                                                                                                  \
  std::string param;                                                                                                  \
  NODE_ARG_STR(0, #param_name, param);                                                                                \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                                \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                                     \
  int err = obj->this_->wrapped_method(param.c_str());                                                                \
  if (err) return NODE_THROW_OGRERR(err);                                                                             \
  return Undefined();                                                                                                 \
}


#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                              \
{                                                                                                               \
  HandleScope scope;                                                                                            \
  int param;                                                                                                    \
  NODE_ARG_INT(0, #param_name, param);                                                                          \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                          \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                               \
  int err = obj->this_->wrapped_method(param);                                                                  \
  if (err) return NODE_THROW_OGRERR(err);                                                                       \
  return Undefined();                                                                                           \
}


#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)       \
Handle<Value> klass::method(const Arguments& args)                                                             \
{                                                                                                              \
  HandleScope scope;                                                                                           \
  double param;                                                                                                \
  NODE_ARG_DOUBLE(0, #param_name, param);                                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                         \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                              \
  int err =obj->this_->wrapped_method(param);                                                                  \
  if (err) return NODE_THROW_OGRERR(err);                                                                      \
  return Undefined();                                                                                          \
}

// ----- wrapped methods -------

#define NODE_WRAPPED_METHOD(klass, method, wrapped_method)           \
Handle<Value> klass::method(const Arguments& args)                   \
{                                                                    \
  HandleScope scope;                                                 \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());               \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed"); \
  obj->this_->wrapped_method();                                      \
  return Undefined();                                                \
}


#define NODE_WRAPPED_METHOD_WITH_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                      \
{                                                                                                       \
  HandleScope scope;                                                                                    \
  param_type *param;                                                                                    \
  NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                  \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                                  \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                       \
  obj->this_->wrapped_method(param->get());                                                             \
  return Undefined();                                                                                   \
}


#define NODE_WRAPPED_METHOD_WITH_1_INTEGER_PARAM(klass, method, wrapped_method, param_name) \
Handle<Value> klass::method(const Arguments& args)                                          \
{                                                                                           \
  HandleScope scope;                                                                        \
  int param;                                                                                \
  NODE_ARG_INT(0, #param_name, param);                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");            \
  obj->this_->wrapped_method(param);                                                        \
  return Undefined();                                                                       \
}


#define NODE_WRAPPED_METHOD_WITH_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)  \
Handle<Value> klass::method(const Arguments& args)                                          \
{                                                                                           \
  HandleScope scope;                                                                        \
  double param;                                                                             \
  NODE_ARG_DOUBLE(0, #param_name, param);                                                   \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");           \
  obj->this_->wrapped_method(param);                                                        \
  return Undefined();                                                                       \
}


#define NODE_WRAPPED_METHOD_WITH_1_BOOLEAN_PARAM(klass, method, wrapped_method, param_name) \
Handle<Value> klass::method(const Arguments& args)                                          \
{                                                                                           \
  HandleScope scope;                                                                        \
  bool param;                                                                               \
  NODE_ARG_BOOL(0, #param_name, param);                                                     \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");           \
  obj->this_->wrapped_method(param);                                                        \
  return Undefined();                                                                       \
}


#define NODE_WRAPPED_METHOD_WITH_1_ENUM_PARAM(klass, method, wrapped_method, enum_type, param_name) \
Handle<Value> klass::method(const Arguments& args)                                                  \
{                                                                                                   \
  HandleScope scope;                                                                                \
  enum_type param;                                                                                  \
  NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                  \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                              \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");                   \
  obj->this_->wrapped_method(param);                                                                \
  return Undefined();                                                                               \
}


#define NODE_WRAPPED_METHOD_WITH_1_STRING_PARAM(klass, method, wrapped_method, param_name)  \
Handle<Value> klass::method(const Arguments& args)                                          \
{                                                                                           \
  HandleScope scope;                                                                        \
  std::string param;                                                                        \
  NODE_ARG_STR(0, #param_name, param);                                                      \
  klass *obj = ObjectWrap::Unwrap<klass>(args.This());                                      \
  if (!obj->this_) return NODE_THROW(#klass" object has already been destroyed");           \
  obj->this_->wrapped_method(param.c_str());                                                \
  return Undefined();                                                                       \
}

#endif