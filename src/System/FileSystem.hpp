#pragma once
#include <cstdio>
#include <string>

FILE* OpenFile(const char* path, const char* mode);
bool TryReadFileContents(const char* path, std::string& result);