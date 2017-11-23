#pragma once
#include "cjson/cjson.h"
#include <string>

#undef JSON_TEMPLATE_STAGE
#define JSON_TEMPLATE_STAGE 0
#include "JsonStructTemplateMacro.h"
#include "JsonStructUserDef.h"