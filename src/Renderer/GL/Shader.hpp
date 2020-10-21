#pragma once

#include <string>

#include "glcorearb.h"

class Shader
{
public:
    void Compile(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr);
    void Use();

    int ProgramId() const { return _id; }

private:
    void CheckCompileErrors(GLuint object, std::string type);

    int _id;
};

