#include <fstream>

#include "ShaderResource.hpp"

enum class ShaderType
{
    None,
    Vertex,
    Fragment
};

bool ShaderResource::LoadFromFile(const ResourceSettings& settings)
{
    std::string vertexShader;
    std::string fragmentShader;

    std::ifstream file(settings.path);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    ShaderType type = ShaderType::None;

    while (!file.eof())
    {
        std::getline(file, line);

        if (line == "// vertex") type = ShaderType::Vertex;
        else if (line == "// fragment") type = ShaderType::Fragment;
        else
        {
            switch (type)
            {
            case ShaderType::Vertex:
                vertexShader += "\n" + line;
                type = ShaderType::Vertex;
                break;
            case ShaderType::Fragment:
                fragmentShader += "\n" + line;
                type = ShaderType::Fragment;
                break;
            default:
                Log("Forgot to set shader type to vertex or fragment\n");
                return false;
            }
        }
    }

    _resource.emplace();
    _resource.value().Compile(vertexShader.c_str(), fragmentShader.c_str());

    return true;
}
