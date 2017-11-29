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
	XJSON_PARSE(JITEM(x) JITEM(y) JITEM(z))
};	


struct Player
{
	double age;
	std::string name;
	bool alive;
	std::vector<Vector> pos;
	// add parser
	XJSON_PARSE(JITEM(age) JITEM(name) JITEM(alive) JITEM(pos))
};

int main()
{
	iflib::Archive * arch = iflib::GetDefArchive();
	iflib::IBlob * blob = arch->Open("data.json");
	auto json = iflib::JsonBlob::FromFile(blob);
	auto structJson = json->FindItem("player");
	Player uda;
	Player::Parser::Parse(uda, structJson->item);
    return 0; 
}