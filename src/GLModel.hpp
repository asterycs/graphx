#ifndef GLMODEL_HPP
#define GLMODEL_HPP

#include "GLDrawable.hpp"
#include "Model.hpp"
#include "Utils.hpp"

class GLModel : public GLDrawable
{
public:
  GLModel();
  GLModel(const GLModel& that) = delete;
  GLModel& operator=(const GLModel& that) = delete;
  ~GLModel();

#ifdef ENABLE_CUDA
  const Node* getDeviceBVH() const;
#endif

  const std::vector<Material>& getBVHBoxMaterials() const;
  const std::vector<MeshDescriptor>& getBVHBoxDescriptors() const;
  void load(const Model& model);
  const std::string& getFileName() const;

private:
  std::vector<MeshDescriptor> bvhBoxDescriptors;
  std::vector<Material> bvhBoxMaterials;
  std::string fileName;

#ifdef ENABLE_CUDA
  Node* deviceBVH;
#endif
};

#endif
