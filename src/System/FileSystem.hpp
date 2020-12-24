#pragma once

#include <cstdio>
#include <string>
#include <vector>

FILE* OpenFile(const char* path, const char* mode);
bool TryReadFileContents(const char* path, std::string& result);
bool TryReadFileContents(const char* path, std::vector<unsigned char>& result);