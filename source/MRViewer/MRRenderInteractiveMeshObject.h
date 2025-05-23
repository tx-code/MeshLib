#pragma once

#include "MRMeshDataSource.h"
#include "MRViewer/MRRenderInteractiveObject.h"

#include <MeshVS_Mesh.hxx>

namespace MR
{
class RenderInteractiveMeshObject : public RenderInteractiveObject
{
public:
  RenderInteractiveMeshObject(const VisualObject& visObj);

  ~RenderInteractiveMeshObject() override;

  bool render(const ModelRenderParams& params) override;

  size_t heapBytes() const override;
  size_t glBytes() const override;
  
  const Handle(AIS_InteractiveObject)& getAisObject() const override {
    return meshPrs_;
  }

  const Handle(MeshVS_Mesh)& getMeshPrs() const {
    return meshPrs_;
  }

protected:
  void syncPropertiesFromVisualObject_() override;

  void createInteractiveObject_() override;

  const ObjectMeshHolder* objMesh_;

  bool isMeshPrsValid_ = false;
  Handle(MR_MeshDataSource) meshDataSource_;
  Handle(MeshVS_Mesh) meshPrs_;
};
} // namespace MR