// JsonStructParser.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "cjson/cJSON.h"
#include "iflib/Archive.h"
#include "iflib/utils/JsonBlob.h"

#include <vector>
#include <array>
#include <typeindex>

#include "JsonStructParser.h"

struct Vector
{
	float x;
	float y;
	float z;
	// add parser
	IFLIB_ADD_JSON_PARSE(IFDJI(x) IFDJI(y) IFDJI(z))
};	


struct UserDataA
{
	double age;
	std::string name;
	bool alive;
	std::vector<Vector> pos;
	// add parser
	IFLIB_ADD_JSON_PARSE(IFDJI(age) IFDJI(name) IFDJI(alive) IFDJI(pos))
};

int main()
{
	iflib::Archive * arch = iflib::GetDefArchive();
	iflib::IBlob * blob = arch->Open("data.json");
	auto json = iflib::JsonBlob::FromFile(blob);
	auto structJson = json->FindItem("player");
	UserDataA uda;
	UserDataA::Parser::Parse(uda, structJson->item);
    return 0; 
}