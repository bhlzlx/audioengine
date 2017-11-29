#pragma once

#define BASICPARSECODE \
template<>\
static bool ParseBasicType(double& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	if (_json->type == cJSON_Number)\
	{\
		_v = _json->valuedouble;\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(std::vector<double>& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	int n = cJSON_GetArraySize(_json);\
	for (int i = 0; i < n; ++i)\
	{\
		auto item = cJSON_GetArrayItem(_json, i);\
		_v.push_back(item->valuedouble);\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(float& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	if (_json->type == cJSON_Number)\
	{\
		_v = _json->valuedouble;\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(std::vector<float>& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	int n = cJSON_GetArraySize(_json);\
	for (int i = 0; i < n; ++i)\
	{\
		auto item = cJSON_GetArrayItem(_json, i);\
		_v.push_back(item->valuedouble);\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(std::string& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	if (_json->type == cJSON_String)\
	{\
		_v = _json->valuestring;\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(std::vector<std::string>& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	int n = cJSON_GetArraySize(_json);\
	for (int i = 0; i < n; ++i)\
	{\
		auto item = cJSON_GetArrayItem(_json, i);\
		_v.push_back(item->valuestring);\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(bool& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	if (_json->type == cJSON_True)\
	{\
		_v = true;\
	}\
	else\
	{\
		_v = false;\
	}\
	return true;\
}\
\
template<>\
static bool ParseBasicType(std::vector<bool>& _v, cJSON * _json)\
{\
	if (!_json) return false;\
	int n = cJSON_GetArraySize(_json);\
	for (int i = 0; i < n; ++i)\
	{\
		auto item = cJSON_GetArrayItem(_json, i);\
		bool v;\
		if (item->type == cJSON_True)\
		{\
			v = true;\
		}\
		else\
		{\
			v = false;\
		}\
		_v.push_back(v);\
	}\
	return true;\
}\
\
template<>static bool ParseObject(double& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(std::vector<double>& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(float& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(std::vector<float>& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(std::string& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(std::vector<std::string>& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(bool& _value, cJSON* _json) { return false; }\
template<>static bool ParseObject(std::vector<bool>& _value, cJSON* _json) { return false; }\

#define DEFINE_PARSER_HEADER \
struct Parser\
{\
template< class T>\
static bool ParseObject(T& _value, cJSON* _json)\
{\
	if (!_json) return false;\
	T::Parser::Parse(_value, _json);\
}\
\
template< class T >\
static bool ParseObject(std::vector<T>& _value, cJSON* _json)\
{\
	if (!_json) return false;\
\
	int nItem = cJSON_GetArraySize(_json);\
	for (int i = 0; i < nItem; ++i)\
	{\
		auto item = cJSON_GetArrayItem(_json, i);\
		T value;\
		T::Parser::Parse(value, item);\
		_value.push_back(value);\
	}\
	return true;\
}\
\
template< class T>\
static bool ParseBasicType(T& _value, cJSON* _json)\
{\
	return false;\
}\
\
template< class T >\
static bool Parse(T& _value, cJSON* _json)\
{\
	if (!_json) return false;\
	cJSON* item = nullptr;\

#define IFDJI( ITEM )\
item = cJSON_GetObjectItem(_json, #ITEM );\
if (IsBasicType(_value.ITEM))\
{\
	ParseBasicType(_value.ITEM, item);\
}\
else\
{\
	ParseObject(_value.ITEM, item);\
}


#define DEFINE_PARSER_TAIL\
	return true;\
		}\
		BASICPARSECODE\
	};

#define IFLIB_ADD_JSON_PARSE( ITEMS ) DEFINE_PARSER_HEADER ITEMS DEFINE_PARSER_TAIL
#define DEF_PARSE_ITEM( ITEM )


std::array<std::type_index, 8> BasicTypes =
{
	typeid(double),
	typeid(float),
	typeid(std::string),
	typeid(bool),
	typeid(std::vector<double>),
	typeid(std::vector<float>),
	typeid(std::vector<std::string>),
	typeid(std::vector<bool>)
};

template< class T>
bool IsBasicType(T& _v)
{
	for (auto& type : BasicTypes)
	{
		if (type == typeid(_v))
		{
			return true;
		}
	}
	return false;
}

template<>
bool IsBasicType(double& _v) { return true; }