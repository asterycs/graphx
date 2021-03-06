#include "App.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <exception>
#include <sstream>

#include <glm/gtx/string_cast.hpp>

#include "nfd.h"

#define ILUT_USE_OPENGL
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

App::App() :
    debugPoints(),
    mousePressed(false),
    mousePrevPos(glcontext.getCursorPos()),
    activeRenderer(ActiveRenderer::GL),
    glcontext(),
#ifdef ENABLE_CUDA
    cudaRenderer(),
#endif
    glmodel(),
    gllight(),
    glcanvas(glm::ivec2(WWIDTH, WHEIGHT)),
    camera(),
    loader(),
    debugMode(DebugMode::NONE),
    debugBboxPtr(0u)
{
  ilInit();
  iluInit();
}

App::~App()
{

}

void App::resizeCallbackEvent(int width, int height)
{
  int newWidth = width;
  int newHeight = height;

  const glm::ivec2 newSize = glm::ivec2(newWidth, newHeight);

  glcontext.resize(newSize);
  glcanvas.resize(newSize);
#ifdef ENABLE_CUDA
  cudaRenderer.resize(newSize);
#endif
}

void App::MainLoop()
{
  while (glcontext.isAlive())
  {
    glcontext.clear();
    float dTime = glcontext.getDTime();
    handleControl(dTime);

    switch (activeRenderer)
    {
    case ActiveRenderer::GL:
      glcontext.draw(glmodel, gllight, camera);
      break;
#ifdef ENABLE_CUDA
      case ActiveRenderer::RAYTRACER: // Draw image to OpenGL texture and draw with opengl
      cudaRenderer.rayTraceToCanvas(glcanvas, camera, glmodel, gllight);
      glcontext.draw(glcanvas);
      break;
      case ActiveRenderer::PATHTRACER:
      cudaRenderer.pathTraceToCanvas(glcanvas, camera, glmodel, gllight);
      glcontext.draw(glcanvas);
      break;
#endif
    }

    if (debugMode != DebugMode::NONE)
    {
      drawDebugInfo();
    }

    glcontext.drawUI(activeRenderer, debugMode);
    glcontext.swapBuffers();
  }

  if (glmodel.getFileName() != "") // Check if model is loaded
    createSceneFile(LAST_SCENEFILE_NAME);
}

void App::drawDebugInfo()
{
  if (debugBboxPtr != model.getBVH().size())
    glcontext.draw(glmodel, gllight, camera, model.getBVH()[debugBboxPtr]);

  unsigned int ctr = 0;

  for (auto& n : model.getBVH())
  {
    if (debugBboxPtr == model.getBVH().size() || ctr == debugBboxPtr)
      glcontext.draw(n.bbox, camera);

    ++ctr;
  }

  glcontext.draw(debugPoints, camera);
}

void App::showWindow()
{
  glcontext.showWindow();
}

void App::handleControl(float dTime)
{
  // For mouse
  glm::dvec2 mousePos = glcontext.getCursorPos();

  if (!glcontext.UiWantsMouseInput())
  {
    if (mousePressed)
    {
      glm::dvec2 dir = mousePos - mousePrevPos;

      if (glm::length(dir) > 0.0)
        camera.rotate(dir, dTime);
    }
  }

  mousePrevPos = mousePos;

  if (glcontext.isKeyPressed(GLFW_KEY_W, 0))
    camera.translate(glm::vec3(0.f, 1.f, 0.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_S, 0))
    camera.translate(glm::vec3(0.f, -1.f, 0.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_A, 0))
    camera.translate(glm::vec3(1.f, 0.f, 0.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_D, 0))
    camera.translate(glm::vec3(-1.f, 0.f, 0.f), dTime);
  
  if (glcontext.isKeyPressed(GLFW_KEY_R, 0))
    camera.translate(glm::vec3(0.f, 0.f, 1.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_F, 0))
    camera.translate(glm::vec3(0.f, 0.f, -1.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_RIGHT, 0))
    camera.rotate(glm::vec2(2.f, 0.f), dTime);

  if (glcontext.isKeyPressed(GLFW_KEY_LEFT, 0))
    camera.rotate(glm::vec2(-2.f, 0.f), dTime);
    
  if (glcontext.isKeyPressed(GLFW_KEY_UP, 0))
    camera.rotate(glm::vec2(0.f, -2.f), dTime);
    
  if (glcontext.isKeyPressed(GLFW_KEY_DOWN, 0))
    camera.rotate(glm::vec2(0.f, 2.f), dTime);
}

void App::mouseCallback(int button, int action, int /*modifiers*/)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
      mousePressed = true;
    else if (action == GLFW_RELEASE)
      mousePressed = false;
  }
}

void App::scrollCallback(double /*xOffset*/, double yOffset)
{
  if (debugMode != DebugMode::NONE)
  {
    /*const std::size_t nLeaves = std::count_if(std::begin(model.getBVH()), std::end(model.getBVH()), [](const Node& n)
        {
          if (n.rightIndex == -1)
            return true;
          else
            return false;
        });
    */

    if (yOffset < 0)
      debugBboxPtr = debugBboxPtr > 0 ? debugBboxPtr - 1 : model.getBVH().size();
    else if (yOffset > 0)
      debugBboxPtr = debugBboxPtr >= model.getBVH().size() ? 0 : debugBboxPtr + 1;

  }else
  {
    if (yOffset < 0)
      camera.increaseFOV();
    else if (yOffset > 0)
      camera.decreaseFOV();
  }
}

void App::keyboardCallback(int key, int /*scancode*/, int action, int modifiers)
{

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
  {
    addLight();
  }
  else if (key == GLFW_KEY_O && action == GLFW_PRESS && (modifiers & GLFW_MOD_CONTROL))
  {
    std::cout << "Choose scene file" << std::endl;
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
      loadSceneFile(outPath);
      free(outPath);
    }
  }else if (key == GLFW_KEY_S && action == GLFW_PRESS && (modifiers & GLFW_MOD_CONTROL))
  {
    std::cout << "Choose model file" << std::endl;
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_SaveDialog( NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
      createSceneFile(outPath);
      free(outPath);
    }
  }
  else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
  {
    activeRenderer = static_cast<ActiveRenderer>((activeRenderer + 1) % 3);
    debugPoints.clear();
#ifdef ENABLE_CUDA
    cudaRenderer.reset();
#endif
  }
  else if (key == GLFW_KEY_O && action == GLFW_PRESS)
  {
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
      std::cout << "Opening model: " << outPath << std::endl;
      loadModel(outPath);
      free(outPath);
    }
  }
  else if (key == GLFW_KEY_L && action == GLFW_PRESS)
  {
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
      std::cout << "Loading scene file: " << outPath << std::endl;
      loadSceneFile(outPath);
      free(outPath);
    }
  }else if (key == GLFW_KEY_D && action == GLFW_PRESS && (modifiers & GLFW_MOD_CONTROL))
  {
    debugMode = static_cast<DebugMode>((debugMode + 1) % 3);
#ifdef ENABLE_CUDA
    const glm::ivec2 pos = glcontext.getCursorPos();

    if (debugMode == DebugMode::DEBUG_RAYTRACE)
      debugPoints = cudaRenderer.debugRayTrace(pos, glcanvas.getSize(), camera, glmodel, gllight);
    else if (debugMode == DebugMode::DEBUG_PATHTRACE)
      debugPoints = cudaRenderer.debugPathTrace(pos, glcanvas.getSize(), camera, glmodel, gllight);
    else
      debugPoints.clear();
#endif    
  }

}

void App::addLight()
{
  const glm::mat4 v = camera.getView();
  const glm::mat4 l = glm::inverse(v);

  Light light(l);

  gllight.load(light);
}

void App::createSceneFile(const std::string& filename)
{
  std::ofstream sceneFile;
  sceneFile.open(filename, std::ofstream::out | std::ofstream::trunc);

  /* Order:
   *  Model filename
   *  light
   *  camera
   */

  if (!sceneFile.is_open())
  {
    std::cerr << "Couldn't write scenefile" << std::endl;
    return;
  }

  std::string modelName = glmodel.getFileName();
  sceneFile << modelName << std::endl;

  sceneFile << gllight.getLight() << std::endl;
  sceneFile << camera << std::endl;

  sceneFile.close();

  std::cout << "Wrote scene file " << filename << std::endl;
}

void App::loadModel(const std::string& modelFile)
{
  Model scene = loader.loadOBJ(modelFile);
  model = scene;
  glmodel.load(scene);
}

void App::loadSceneFile(const std::string& filename)
{
  std::ifstream sceneFile;
  sceneFile.open(filename);

  /* Order:
   *  Model filename
   *  light
   *  camera
   */

  if (!sceneFile.is_open())
  {
    std::cerr << "Couldn't open scenefile" << std::endl;
    return;
  }

  std::string modelName;
  std::getline(sceneFile, modelName);
  loadModel(modelName);

  Light newLight;
  sceneFile >> newLight;
  gllight.load(newLight);

  sceneFile >> camera;
  sceneFile.close();

  std::cout << "Loaded scene file " << filename << std::endl;
}

#ifdef ENABLE_CUDA
void App::rayTraceToFile(const std::string& sceneFile, const std::string& outfile)
{
  loadSceneFile(sceneFile);

#ifdef ENABLE_CUDA
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  cudaEventRecord(start);
  cudaRenderer.rayTraceToCanvas(glcanvas, camera, glmodel, gllight);
  cudaEventRecord(stop);

  cudaEventSynchronize(stop);
  float millis = 0;
  cudaEventElapsedTime(&millis, start, stop);

  writeTextureToFile(glcanvas, outfile);
  std::cout << "Rendering time [ms]: " << millis << std::endl;
#endif

  return;
}

void App::pathTraceToFile(const std::string& sceneFile, const std::string& outfile, const int paths)
{
  loadSceneFile(sceneFile);

  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  cudaEventRecord(start);

  for (int i = 0; i < paths; ++i)
  {
    cudaRenderer.pathTraceToCanvas(glcanvas, camera, glmodel, gllight);
  }

  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  float millis = 0;
  cudaEventElapsedTime(&millis, start, stop);

  writeTextureToFile(glcanvas, outfile);
  std::cout << "Rendering time [ms]: " << millis << std::endl;

  return;
}
#endif

void App::writeTextureToFile(const GLTexture& texture, const std::string& fileName)
{
  ILuint imgID;

  IL_CHECK(ilGenImages(1, &imgID));
  IL_CHECK(ilBindImage(imgID));

  const glm::ivec2 size = texture.getSize();

  // Direct copy of the texture does not seem to work. Must pass it via a PBO.
  // Some others seem to experience the same: https://devtalk.nvidia.com/default/topic/913544/glgetteximage-gives-blank-result-after-cuda-surface-write/
  GLuint  pbo;
  GL_CHECK(glGenBuffers(1, &pbo));
  GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
  GL_CHECK(glBufferData(GL_PIXEL_PACK_BUFFER, size.x*size.y*3*sizeof(float), nullptr, GL_STREAM_COPY));

  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture.getTextureID()));

  GL_CHECK(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, NULL));
  void *mem = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

  // When asking OpenGL kindly for GL_UNSIGNED_BYTE data with glGetTexImage the result is just zeros.
  // Workaround by asking GL_FLOAT and manually converting.
  std::vector<unsigned char> tmp(size.x*size.y*3, 255);

  for (int i = 0; i < size.x*size.y*3; ++i)
  {
    tmp[i] *= *(static_cast<float*>(mem) + i);
  }

  IL_CHECK(ilTexImage(size.x, size.y, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, tmp.data()));

  IL_CHECK(ilEnable(IL_FILE_OVERWRITE));
  IL_CHECK(ilSaveImage(fileName.c_str()));

  GL_CHECK(glDeleteBuffers(1, &pbo));
  IL_CHECK(ilDeleteImages(1, &imgID));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
  GL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
}

