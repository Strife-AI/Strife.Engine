#include "FileSystem.hpp"

#include <fstream>
#include <sstream>

#include "Logger.hpp"

FILE* OpenFile(const char* path, const char* mode)
{
	return fopen(path, mode);
}

bool TryReadFileContents(const char* path, std::string& result)
{
	std::ifstream file(path);

	if(!file.is_open())
	{
		Log("Failed to open %s for reading\n", path);
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	result = buffer.str();
	file.close();

	return true;
}

bool TryReadFileContents(const char* path, std::vector<unsigned char>& result)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
    {
        Log("Failed to open %s for reading\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    result.resize(size);
    fseek(file, 0, SEEK_SET);
    fread(result.data(), 1, size, file);
    fclose(file);

    return true;
}
