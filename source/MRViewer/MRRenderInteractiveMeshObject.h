#pragma once

#include "MRMesh/MRViewportId.h"
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
  
  const Handle(AIS_InteractiveObject)& getAisObject() const override;

  const Handle(MeshVS_Mesh)& getMeshPrs() const;

protected:
  void syncPropertiesFromVisualObject_(ViewportId viewportId) override;

  void createInteractiveObject_(const ModelRenderParams& params) override;

  const ObjectMeshHolder* objMesh_;
  struct InternalData;
  std::unique_ptr<InternalData> internalData_;
};
} // namespace MR