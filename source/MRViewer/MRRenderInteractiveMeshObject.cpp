#include "MRRenderInteractiveMeshObject.h"
#include "MRMesh/MRObjectMeshHolder.h"
#include "MRMeshDataSource.h"
#include "MRViewController.h"
#include "MRViewer/MRViewer.h"

// OCCT
#include <MeshVS_DisplayModeFlags.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_Mesh.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <MeshVS_NodalColorPrsBuilder.hxx>
#include <AIS_InteractiveContext.hxx>

namespace MR
{
RenderInteractiveMeshObject::RenderInteractiveMeshObject(const VisualObject& visObj)
{
  objMesh_ = dynamic_cast<const ObjectMeshHolder*>(&visObj);
  assert(objMesh_);
}

RenderInteractiveMeshObject::~RenderInteractiveMeshObject()
{
  spdlog::info("RenderInteractiveMeshObject::~RenderInteractiveMeshObject");
}

bool RenderInteractiveMeshObject::render([[maybe_unused]] const ModelRenderParams& params)
{
  // spdlog::info("RenderInteractiveMeshObject::render {}", objMesh_->name());

  createInteractiveObject_();

  auto& viewController = getViewerInstance().getViewController();
  auto& context = viewController.getAisContext();
  if(!context->IsDisplayed(meshPrs_))
  {
    viewController.addObject(meshPrs_, objMesh_);
  }

  return false;
}

size_t RenderInteractiveMeshObject::heapBytes() const
{
  return 0;
}

size_t RenderInteractiveMeshObject::glBytes() const
{
  // Currently I don't know how to calculate it from AIS, It just provides some 
  // overview information about the memory usage.
  return 0;
}

void RenderInteractiveMeshObject::syncPropertiesFromVisualObject_() {}

void RenderInteractiveMeshObject::createInteractiveObject_()
{
  if(isMeshPrsValid_)
  {
    return;
  }

  meshPrs_ = new MeshVS_Mesh();
  meshDataSource_ = new MR_MeshDataSource(*objMesh_->mesh());
  meshPrs_->SetDataSource(meshDataSource_);

  // Add Builders
  Handle(MeshVS_MeshPrsBuilder) mainBuilder = new MeshVS_MeshPrsBuilder(meshPrs_);
  meshPrs_->AddBuilder(mainBuilder, true);

  // Optional
  meshPrs_->SetOwner(this);

  isMeshPrsValid_ = true;
}
} // namespace MR
