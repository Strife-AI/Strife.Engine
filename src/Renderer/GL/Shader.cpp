#include <System/Logger.hpp>
#include "Shader.hpp"
#include "gl3w.h"

void Shader::Compile(const char* vertexSource, const char* fragmentSource, const char* geometrySource)
{
    GLuint sVertex, sFragment, gShader;

    // Vertex Shader
    sVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sVertex, 1, &vertexSource, nullptr);
    glCompileShader(sVertex);
    CheckCompileErrors(sVertex, "VERTEX");

    // Fragment Shader
    sFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sFragment, 1, &fragmentSource, nullptr);
    glCompileShader(sFragment);
    CheckCompileErrors(sFragment, "FRAGMENT");

    // Geometry shader
    if (geometrySource != nullptr)
    {
        gShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gShader, 1, &geometrySource, nullptr);
        glCompileShader(gShader);
        CheckCompileErrors(gShader, "GEOMETRY");
    }
    // Shader Program
    _id = glCreateProgram();
    glAttachShader(_id, sVertex);
    glAttachShader(_id, sFragment);

    if (geometrySource != nullptr)
    {
        glAttachShader(_id, gShader);
    }

    glLinkProgram(_id);
    CheckCompileErrors(_id, "PROGRAM");

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(sVertex);
    glDeleteShader(sFragment);
    if (geometrySource != nullptr)
    {
        glDeleteShader(gShader);
    }
}

void Shader::CheckCompileErrors(GLuint object, std::string type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, NULL, infoLog);
            FatalError("Shader compile time error (%s): %s", type.c_str(), infoLog);
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, NULL, infoLog);
            FatalError("Shader link time error (%s): %s\n", type.c_str(), infoLog);
        }
    }
}

void Shader::Use()
{
    glUseProgram(_id);
}
