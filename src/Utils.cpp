#include "Utils.hpp"
#include "Model.hpp"

#include <fstream>

#include <GL/glew.h>
#include <GL/gl.h>

#ifdef ENABLE_CUDA
  #include <cuda_runtime.h>
#endif


void CheckOpenGLError(const char* call, const char* fname, int line)
{
  GLenum error = glGetError();

  if (error != GL_NO_ERROR)
  {
    std::string errorStr;
    switch (error)
    {
      case GL_INVALID_ENUM:                   errorStr = "GL_INVALID_ENUM"; break;
      case GL_INVALID_VALUE:                  errorStr = "GL_INVALID_VALUE"; break;
      case GL_INVALID_OPERATION:              errorStr = "GL_INVALID_OPERATION"; break;
      case GL_STACK_OVERFLOW:                 errorStr = "GL_STACK_OVERFLOW"; break;
      case GL_STACK_UNDERFLOW:                errorStr = "GL_STACK_UNDERFLOW"; break;
      case GL_OUT_OF_MEMORY:                  errorStr = "GL_OUT_OF_MEMORY"; break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
      default:                                errorStr = "Unknown error"; break;
    }

    std::cerr << "At: " << fname << ":" << line << std::endl \
     << " OpenGL call: " << call << std::endl \
      << " Error: " << errorStr << std::endl;
  }
}

#ifdef ENABLE_CUDA
void CheckCudaError(const char* call, const char* fname, int line)
{
    cudaError_t result_ = cudaGetLastError();
    if (result_ != cudaSuccess) {
        std::cerr << "At: " << fname << ":" << line << std::endl \
           << " Cuda call: " << call << " Error: " << cudaGetErrorString(result_) << std::endl;
        exit(1);
    }
}
#endif

std::string readFile(const std::string& filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if(!fileStream.is_open()) {
        std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
        return "";
    }

    std::string line = "";
    while(!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}

bool fileExists(const std::string& filename)
{
    std::ifstream infile(filename);
    return infile.good();
}
