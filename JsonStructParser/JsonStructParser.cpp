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

struct Vector {
	double x;
	double y;
	double z;
	XJSON(x, y, z)
};

struct Player {
	double age;
	std::string name;
	bool alive;
	std::vector<Vector> pos;
	XJSON(age, name, alive, pos)
};

int main()
{
	iflib::Archive * arch = iflib::GetDefArchive();
	iflib::IBlob * blob = arch->Open("data.json");
	auto json = iflib::JsonBlob::FromFile(blob);
	auto structJson = json->FindItem("player");
	Player uda;
	Player::X::Parse(uda, structJson->item);

	cJSON * savedRoot = cJSON_CreateObject();
	cJSON * savedPlayer = cJSON_CreateObject();
	cJSON_AddItemToObject(savedRoot, "player", savedPlayer);

	Player::X::Save(uda, savedPlayer );

	const char * str = cJSON_Print(savedRoot);

	printf("%s", str);

    return 0; 
}