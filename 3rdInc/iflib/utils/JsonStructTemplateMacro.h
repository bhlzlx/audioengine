#if (JSON_TEMPLATE_STAGE == 0)
    #undef DEFINE_JSON_STRUCT_BEGIN
    #undef DEF_JSON_ITEM_BOOL
    #undef DEF_JSON_ITEM_NUMBER
    #undef DEF_JSON_ITEM_STRING
    #undef DEF_JSON_ITEM_VECTOR
    #undef DEF_JSON_ITEM_OBJECT
    #undef DEFINE_JSON_STRUCT_END
    #define DEFINE_JSON_STRUCT_BEGIN( CLASS ) struct CLASS \
    {\
        bool valid;\
        CLASS(){ valid = false; }\
        bool ParseJson( cJSON * _json );
    #define DEF_JSON_ITEM_BOOL(NAME) bool NAME;
    #define DEF_JSON_ITEM_NUMBER(NAME) double NAME;
    #define DEF_JSON_ITEM_STRING(NAME) std::string NAME;
    #define DEF_JSON_ITEM_VECTOR(TYPE, NAME ) std::vector<TYPE> NAME;
    #define DEF_JSON_ITEM_OBJECT(TYPE, NAME ) TYPE NAME;
    #define DEFINE_JSON_STRUCT_END };
#endif

#if( JSON_TEMPLATE_STAGE == 1)
#undef DEFINE_JSON_STRUCT_BEGIN
#undef DEF_JSON_ITEM_BOOL
#undef DEF_JSON_ITEM_NUMBER
#undef DEF_JSON_ITEM_STRING
#undef DEF_JSON_ITEM_VECTOR
#undef DEF_JSON_ITEM_OBJECT
#undef DEFINE_JSON_STRUCT_END

#define DEFINE_JSON_STRUCT_BEGIN( CLASS ) bool CLASS::ParseJson( cJSON * _json ){\
cJSON * item = nullptr;
#define DEF_JSON_ITEM_BOOL(NAME) item = cJSON_GetObjectItem( _json, #NAME ); if( !item ) return false; NAME = (item->type == cJSON_True);
#define DEF_JSON_ITEM_NUMBER(NAME) item = cJSON_GetObjectItem( _json, #NAME ); if( !item ) return false; NAME = item->valuedouble;
#define DEF_JSON_ITEM_STRING(NAME) item = cJSON_GetObjectItem( _json, #NAME ); if( !item ) return false; NAME = item->valuestring;

#define DEF_JSON_ITEM_VECTOR(TYPE, NAME )\
item = cJSON_GetObjectItem( _json, #NAME );\
if(!item )\
	return false;\
if( !cJSON_IsArray(item)) return false;\
int arrSz = cJSON_GetArraySize( item );\
for( int i = 0; i< arrSz; ++i ){\
	cJSON * arrItem = cJSON_GetArrayItem(item, i);\
	TYPE arrEle;\
	if( !arrEle.ParseJson(arrItem))\
		return false;\
	NAME.push_back(arrEle);\
}

#define DEF_JSON_ITEM_OBJECT(TYPE, NAME ) \
item = cJSON_GetObjectItem( _json, #NAME );\
if( !item ) return false;\
TYPE obj;\
if( !obj.ParseJson( item )) return false;\
NAME = obj;
#define DEFINE_JSON_STRUCT_END valid = true; return true; }
#endif