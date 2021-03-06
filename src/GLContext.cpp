#include "GLContext.hpp"
#include "App.hpp"
#include "Utils.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/component_wise.hpp>

#include <imgui.h>

GLContext::GLContext() :
  modelShader(),
  lightShader(),
  canvasShader(),
  window(nullptr),
  size(WWIDTH, WHEIGHT),
  ui()
{
  if (!glfwInit())
  {
    std::cerr << "GLFW init failed" << std::endl;
  }

  glfwSetTime(0);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_ALPHA_BITS, 8);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 8);

  window = glfwCreateWindow(size.x, size.y, "GRAPHX", nullptr, nullptr);
  if (window == nullptr) {
      throw std::runtime_error("Failed to create GLFW window");
  }
  GL_CHECK(glfwMakeContextCurrent(window));

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if(err!=GLEW_OK) {
    throw std::runtime_error("glewInit failed");
  }

  // Defuse bogus error
  glGetError();

  ui.init(window);

  GL_CHECK(glClearColor(0.2f, 0.25f, 0.3f, 1.0f));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  int width, height;
  GL_CHECK(glfwGetFramebufferSize(window, &width, &height));
  GL_CHECK(glViewport(0, 0, width, height));
  glfwSwapInterval(1);
  glfwSwapBuffers(window);

  glfwSetMouseButtonCallback(window,
      [](GLFWwindow *, int button, int action, int modifiers) {
          App::getInstance().mouseCallback(button, action, modifiers);
      }
  );
  
  glfwSetScrollCallback(window,
      [](GLFWwindow *, double xOffset, double yOffset) {
          App::getInstance().scrollCallback(xOffset, yOffset);
      }
  );

  glfwSetKeyCallback(window,
      [](GLFWwindow *, int key, int scancode, int action, int mods) {
          App::getInstance().keyboardCallback(key, scancode, action, mods);
      }
  );

  glfwSetWindowSizeCallback(window,
      [](GLFWwindow *, int width, int height) {
          App::getInstance().resizeCallbackEvent(width, height);
      }
  );

  glfwSetErrorCallback([](int error, const char* description) {
    std::cout << "Error: " << error << " " << description << std::endl;
  });

  modelShader.loadShader("shaders/model/vshader.glsl", "shaders/model/fshader.glsl");
  lightShader.loadShader("shaders/light/vshader.glsl", "shaders/light/fshader.glsl");
  canvasShader.loadShader("shaders/canvas/vshader.glsl", "shaders/canvas/fshader.glsl");
  depthShader.loadShader("shaders/depth/vshader.glsl", "shaders/depth/fshader.glsl");
  lineShader.loadShader("shaders/line/vshader.glsl", "shaders/line/fshader.glsl");
  
  std::cout << "OpenGL context initialized" << std::endl;
}

GLContext::~GLContext()
{
  glfwDestroyWindow(window);
  glfwTerminate();
}

bool GLContext::shadersLoaded() const
{
  if (modelShader.isLoaded() \
 && lightShader.isLoaded() \
 && canvasShader.isLoaded() \
 && depthShader.isLoaded() \
 && lineShader.isLoaded())
    return true;
  else
    return false;
}

void GLContext::clear()
{
  if (!shadersLoaded())
    return;
    
  GL_CHECK(glClearColor(0.2f, 0.25f, 0.3f, 1.0f));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  GL_CHECK(glEnable(GL_DEPTH_TEST));

  glfwPollEvents();
}

void GLContext::swapBuffers()
{
  glfwSwapBuffers(window);
}

bool GLContext::isAlive()
{
  return !glfwWindowShouldClose(window);
}

float GLContext::getDTime()
{
  return ui.getDTime();
}

float GLContext::getTime() const
{
  return (float) glfwGetTime();
}

void GLContext::drawUI(const enum ActiveRenderer activeRenderer, const enum DebugMode debugMode)
{
  ui.draw(activeRenderer, debugMode);
}

bool GLContext::UiWantsMouseInput()
{
  ImGuiIO& io = ImGui::GetIO();

  if (io.WantCaptureMouse || io.WantMoveMouse)
    return true;
  else
    return false;
}

void GLContext::draw(const GLModel& model, const GLLight& light, const Camera& camera)
{
  updateShadowMap(model, light);
  //drawShadowMap(light); // For debug
  drawModel(model, camera, light);
  drawLight(light, camera);
}

void GLContext::draw(const GLModel& model, const GLLight& light, const Camera& camera, const Node& debugNode)
{
  drawNodeTriangles(model, light, camera, debugNode);
}

void GLContext::draw(const AABB& box, const Camera& camera)
{
  const glm::fvec3 p0(box.min);
  const glm::fvec3 p1(box.max);
  const glm::fvec3 p2(p0.x, p0.y, p1.z);
  const glm::fvec3 p3(p0.x, p1.y, p0.z);
  const glm::fvec3 p4(p1.x, p0.y, p0.z);
  const glm::fvec3 p5(p0.x, p1.y, p1.z);
  const glm::fvec3 p6(p1.x, p0.y, p1.z);
  const glm::fvec3 p7(p1.x, p1.y, p0.z);

  const std::vector<glm::fvec3> vertices
  {
    p5, p1,
    p1, p7,
    p7, p3,
    p3, p5,

    p2, p6,
    p6, p4,
    p4, p0,
    p0, p2,

    p5, p2,
    p1, p6,
    p7, p4,
    p3, p0
  };

  draw(vertices, camera);
}

void GLContext::drawShadowMap(const GLLight& light)
{
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  const glm::ivec2 shadowMapSize = light.getShadowMap().getSize();
  glViewport(0,0,shadowMapSize.x, shadowMapSize.y);
  
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  canvasShader.bind();
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, light.getShadowMap().getDepthTextureID()));
  GL_CHECK(glBindVertexArray(light.getShadowMap().getVaoID()));

  canvasShader.updateUniform1i("texture", 0);

  GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

  GL_CHECK(glBindVertexArray(0));
  canvasShader.unbind();
}

void GLContext::updateShadowMap(const GLDrawable& model, const GLLight& light)
{
  if (!shadersLoaded() || model.getNTriangles() == 0)
    return;

  GLuint frameBufferID = light.getShadowMap().getFrameBufferID();

  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID));
  glClear(GL_DEPTH_BUFFER_BIT);
  GL_CHECK(glEnable(GL_DEPTH_TEST));
  GL_CHECK(glCullFace(GL_FRONT));

  const auto vaoID = model.getVaoID();
  const auto& meshDescriptors = model.getMeshDescriptors();
  const glm::ivec2 depthTextureSize = light.getShadowMap().getSize();
  GL_CHECK(glViewport(0, 0, depthTextureSize.x, depthTextureSize.y));

  depthShader.bind();
  GL_CHECK(glBindVertexArray(vaoID));
  GL_CHECK(depthShader.updateUniformMat4f("MVP", light.getDepthMVP()));

  for (auto& meshDescriptor : meshDescriptors)
  {
    GL_CHECK(glDrawElements(GL_TRIANGLES, meshDescriptor.vertexIds.size(), GL_UNSIGNED_INT, meshDescriptor.vertexIds.data()));
  }

  depthShader.unbind();
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
  GL_CHECK(glBindVertexArray(0));
  return;
}

void GLContext::showWindow()
{
  glfwShowWindow(window);
}

void GLContext::drawNodeTriangles(const GLModel& model, const GLLight& light, const Camera& camera, const Node& node)
{
  if (!shadersLoaded() || model.getNTriangles() == 0)
    return;

  GL_CHECK(glViewport(0,0,size.x, size.y));
  GL_CHECK(glCullFace(GL_BACK));
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_CHECK(glDepthFunc(GL_LEQUAL));

  const auto& vaoID = model.getVaoID();

  modelShader.bind();
  modelShader.updateUniformMat4f("posToCamera", camera.getMVP(size));
  modelShader.updateUniformMat4f("biasedDepthToLight", light.getDepthBiasMVP());
  modelShader.updateUniform3fv("lightPos", light.getLight().getPosition());
  modelShader.updateUniform3fv("lightNormal", light.getLight().getNormal());

  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  modelShader.updateUniform1i("shadowMap", 0);
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, light.getShadowMap().getDepthTextureID()));
  GL_CHECK(glBindVertexArray(vaoID));


  std::vector<GLuint> vIndices(node.nTri * 3);

  unsigned int cntr = node.startTri * 3;
  for (auto& v : vIndices)
  {
    v = cntr++;
  }

  modelShader.updateUniform3fv("material.colorAmbient", glm::fvec3(1.f, 1.f, 0.f));
  modelShader.updateUniform3fv("material.colorDiffuse", glm::fvec3(1.f, 1.f, 0.f));
  GL_CHECK(glDrawElements(GL_TRIANGLES, vIndices.size(), GL_UNSIGNED_INT, vIndices.data()));


  GL_CHECK(glBindVertexArray(0));
  modelShader.unbind();
}

void GLContext::drawModel(const GLModel& model, const Camera& camera, const GLLight& light)
{
  if (!shadersLoaded() || model.getNTriangles() == 0)
    return;

  GL_CHECK(glViewport(0,0,size.x, size.y));
  GL_CHECK(glCullFace(GL_BACK));
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_CHECK(glDepthFunc(GL_LEQUAL));

  const auto& vaoID = model.getVaoID();
  const auto& meshDescriptors = model.getMeshDescriptors();
  const auto& materials = model.getMaterials();

  modelShader.bind();
  modelShader.updateUniformMat4f("posToCamera", camera.getMVP(size));
  //modelShader.updateUniformMat3f("normalToCamera", glm::mat3(glm::transpose(glm::inverse(camera.getMVP(size)))));
  modelShader.updateUniformMat4f("biasedDepthToLight", light.getDepthBiasMVP());
  modelShader.updateUniform3fv("lightPos", light.getLight().getPosition());
  modelShader.updateUniform3fv("lightNormal", light.getLight().getNormal());

  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  modelShader.updateUniform1i("shadowMap", 0);
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, light.getShadowMap().getDepthTextureID()));
  GL_CHECK(glBindVertexArray(vaoID));

  for (auto& meshDescriptor : meshDescriptors)
  {
    auto& material = materials[meshDescriptor.materialIdx];

    modelShader.updateUniform3fv("material.colorAmbient", material.colorAmbient);
    modelShader.updateUniform3fv("material.colorDiffuse", material.colorDiffuse);
    //modelShader.updateUniform3fv("material.colorSpecular", material.colorSpecular);

    GL_CHECK(glDrawElements(GL_TRIANGLES, meshDescriptor.vertexIds.size(), GL_UNSIGNED_INT, meshDescriptor.vertexIds.data()));
  }

  GL_CHECK(glBindVertexArray(0));
  modelShader.unbind();
}

void GLContext::drawLight(const GLLight& light, const Camera& camera)
{
  if (!shadersLoaded() || light.getNTriangles() == 0)
    return;

  GL_CHECK(glViewport(0,0,size.x, size.y));
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  lightShader.bind();

  const auto& vaoID = light.getVaoID();
  GL_CHECK(glBindVertexArray(vaoID));
  lightShader.updateUniformMat4f("MVP", camera.getMVP(size));
  
  GL_CHECK(glEnable(GL_CULL_FACE));
  GL_CHECK(glCullFace(GL_FRONT));
  lightShader.updateUniform3fv("lightColor", glm::vec3(0.f, 0.f, 0.f));
	GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, light.getNTriangles() * 3));

  GL_CHECK(glCullFace(GL_BACK));
  glm::fvec3 frontLightCol;

  float maxCol = glm::compMax(light.getLight().getEmission());

  if (light.getLight().isEnabled())
    frontLightCol = light.getLight().getEmission() / glm::fvec3(maxCol);
  else
    frontLightCol = light.getLight().getEmission() / glm::fvec3(maxCol) * glm::fvec3(0.3f, 0.3f, 0.3f);


  lightShader.updateUniform3fv("lightColor", frontLightCol);
  GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, light.getNTriangles() * 3));

	GL_CHECK(glBindVertexArray(0));
	lightShader.unbind();
  GL_CHECK(glDisable(GL_CULL_FACE));
}

glm::ivec2 GLContext::getCursorPos()
{
  double x, y;
  glfwGetCursorPos(window, &x, &y);

  return glm::ivec2(x, y);
}

bool GLContext::isKeyPressed(const int glfwKey, const int modifiers) const
{
  const int ctrl  = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) << 1;
  const int shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) << 0;
  const int super = glfwGetKey(window, GLFW_KEY_LEFT_SUPER) || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) << 3;
  const int alt   = glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) << 2;

  const int pressedMods = shift | ctrl | alt | super;

  if (modifiers == pressedMods)
    return glfwGetKey(window, glfwKey);
  else
    return false;
}

void GLContext::resize(const glm::ivec2& newSize)
{
  GL_CHECK(glViewport(0,0,newSize.x, newSize.y));
  this->size = newSize;

  ui.resize(newSize);
}

void GLContext::draw(const std::vector<glm::fvec3>& points, const Camera& camera)
{
  if (!shadersLoaded() || points.size() == 0)
     return;

  lineShader.bind();

  GLuint vao, vbo;
  GL_CHECK(glGenVertexArrays(1, &vao));
  GL_CHECK(glBindVertexArray(vao));

  GL_CHECK(glGenBuffers(1, &vbo));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::fvec3) * points.size(), points.data(), GL_STATIC_DRAW));

  GLint posID = lineShader.getAttribLocation("position");

  GL_CHECK(glVertexAttribPointer(posID, 3, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CHECK(glEnableVertexAttribArray(posID));

  lineShader.updateUniformMat4f("MVP", camera.getMVP(size));

  GL_CHECK(glDrawArrays(GL_LINES, 0, points.size() * 2));
  GL_CHECK(glDeleteVertexArrays(1, &vao));
}

void GLContext::draw(const GLTexture& canvas)
{
  if (!shadersLoaded() || canvas.getTextureID() == 0)
     return;

   GL_CHECK(glActiveTexture(GL_TEXTURE0));
   canvasShader.bind();
   GL_CHECK(glBindTexture(GL_TEXTURE_2D, canvas.getTextureID()));
   
   // GLint posID = canvasShader.getAttribLocation("texture");
   canvasShader.updateUniform1i("texture", 0);
  
   GLuint dummyVao;
   GL_CHECK(glGenVertexArrays(1, &dummyVao));
   GL_CHECK(glBindVertexArray(dummyVao));

   GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

   GL_CHECK(glBindVertexArray(0));
   GL_CHECK(glDeleteVertexArrays(1, &dummyVao));
   canvasShader.unbind();
}

glm::ivec2 GLContext::getSize() const
{
  return size;
}
