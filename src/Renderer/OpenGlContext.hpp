#pragma once

class Logger;

class OpenGlContext
{
public:
    void Initialize(Logger* logger);

    unsigned int CompileShader(const char* shaderName, const char* source, unsigned int shaderType) const;

private:
    static bool InitOpenGl();

    Logger* logger;
};
