//#include <string>
//
//#include "OpenGlContext.hpp"
//#include "System/Logger.hpp"
//#include "GLCheck.hpp"
//#include "OpenGL.hpp"
//
//using namespace std;
//
//void OpenGlContext::Initialize(Logger* logger_)
//{
//    logger = logger_;
//
//    if (!InitOpenGl())
//    {
//        FatalError("Failed to init OpenGL");
//    }
//}
//
//bool OpenGlContext::InitOpenGl()
//{
//    return ogl_LoadFunctions() != ogl_LOAD_FAILED;
//}
//
//unsigned int OpenGlContext::CompileShader(const char* shaderName, const char* source, unsigned int shaderType) const
//{
//    const auto shaderId = glCreateShader(shaderType);
//
//    glCheck(glShaderSource(shaderId, 1, &source, nullptr));
//    glCheck(glCompileShader(shaderId));
//
//    GLint result = GL_FALSE;
//    glCheck(glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result));
//
//    int resultLength = 0;
//    glCheck(glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &resultLength));
//
//    if (result == GL_FALSE)
//    {
//        string errorMessage;
//        errorMessage.resize(resultLength + 1);
//        glCheck(glGetShaderInfoLog(shaderId, resultLength, nullptr, &errorMessage[0]));
//
//        FatalError(
//            "Failed to compile shader %s: %s (error code %d)",
//            shaderName,
//            errorMessage.c_str(),
//            result);
//
//        glCheck(glDeleteShader(shaderId));
//
//        return -1;
//    }
//    logger->Info("Compiled shader %s");
//
//    return shaderId;
//}
