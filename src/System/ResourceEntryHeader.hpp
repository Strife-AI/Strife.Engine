#pragma once

#include <string>

struct ResourceEntryHeader
{
	unsigned int key;
	unsigned int type;
	unsigned int offset;
	std::string name;
};