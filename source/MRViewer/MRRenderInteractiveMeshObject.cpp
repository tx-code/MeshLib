#include "MRRenderInteractiveMeshObject.h"
#include "MRMesh/MRMeshFwd.h"
#include "MRMesh/MRMatrix4.h"
#include "MRMesh/MRObjectMeshHolder.h"
#include "MRMesh/MRVisualObject.h"
#include "MRMeshDataSource.h"
#include "MRViewController.h"
#include "MRViewer.h"
#include "MRMesh/MRTimer.h"
#include "MRQuantityColorUtils.h"
#include "MRViewerInstance.h"

#include <bitset>

// OCCT
#include <MeshVS_DisplayModeFlags.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_Mesh.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <MeshVS_MeshSelectionMethod.hxx>
#include <MeshVS_ElementalColorPrsBuilder.hxx>
#include <MeshVS_TextPrsBuilder.hxx>
#include <MeshVS_VectorPrsBuilder.hxx>
#include <MeshVS_NodalColorPrsBuilder.hxx>
#include <AIS_InteractiveContext.hxx>
#include <gp_Trsf.hxx>
#include <vector>

namespace MR
{
struct RenderInteractiveMeshObject::InternalData
{
  bool                                     isMeshPrsValid{false};
  Handle(MR_MeshDataSource)                meshDataSource;
  Handle(MeshVS_Mesh)                      meshPrs;
  uint32_t                                 dirtyFlags{0};
  Matrix4f                                 prevXf{Matrix4f::identity()};
  std::vector<boost::signals2::connection> connections;

  // When some properties are changed, we need to redisplay the mesh.
  bool needRedisplay{false};

  enum PrsBuilderType : Standard_Integer
  {
    PrsBuilderType_Main = 0,
    PrsBuilderType_NodalColor,
    PrsBuilderType_ElementalColor,
    PrsBuilderType_Text,
    PrsBuilderType_Vector,
    PrsBuilderType_Count
  };
};

RenderInteractiveMeshObject::RenderInteractiveMeshObject(const VisualObject& visObj)
{
  objMesh_ = dynamic_cast<const ObjectMeshHolder*>(&visObj);
  assert(objMesh_);
  internalData_ = std::make_unique<InternalData>();

  internalData_->connections.push_back(
    const_cast<ObjectMeshHolder*>(objMesh_)->worldXfChangedSignal.connect([this]() {
      if (internalData_->isMeshPrsValid)
        updateLocationFromWorldXf(internalData_->meshPrs, objMesh_->worldXf(), true);
    }));
}

RenderInteractiveMeshObject::~RenderInteractiveMeshObject() {}

bool RenderInteractiveMeshObject::render([[maybe_unused]] const ModelRenderParams& params)
{
  if (!Viewer::constInstance()->isGLInitialized())
  {
    objMesh_->resetDirty();
    return false;
  }
  // At the beginning of each frame, we need to update the dirty flags
  internalData_->dirtyFlags = objMesh_->getDirtyFlags();

  auto& viewController = ViewController::getViewControllerInstance();
  auto& context        = viewController.getAisContext();

  // Create the AIS object if it doesn't exist
  createInteractiveObject_(params);

  // Sync properties from the visual object
  syncPropertiesFromVisualObject_(params.viewportId);

  // Add the AIS object to the context if it's not already displayed
  auto fnRedisplay = [&context, this]() {
    context->Redisplay(internalData_->meshPrs, false);
    internalData_->needRedisplay = false;
  };

  if (!context->IsDisplayed(internalData_->meshPrs))
  {
    viewController.addObject(internalData_->meshPrs, objMesh_);
  }
  else if (internalData_->needRedisplay)
  {
    fnRedisplay();
  }

  return true;
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

const Handle(AIS_InteractiveObject)& RenderInteractiveMeshObject::getAisObject() const
{
  return internalData_->meshPrs;
}

const Handle(MeshVS_Mesh)& RenderInteractiveMeshObject::getMeshPrs() const
{
  return internalData_->meshPrs;
}

void RenderInteractiveMeshObject::syncPropertiesFromVisualObject_(
  [[maybe_unused]] ViewportId viewportId)
{
  assert(internalData_->isMeshPrsValid);
  MR_TIMER;

  // Note: we must reset dirty flags here, or the mesh will be redrawn every frame
  objMesh_->resetDirty();

  if (internalData_->dirtyFlags & DIRTY_MESH)
  {
    // TODO: Use dirty flags to update the mesh datasource and prsbuilders
    spdlog::debug("Mesh is dirty, dirtyFlags: {}",
                  std::bitset<32>(internalData_->dirtyFlags).to_string());
  }

  // Get visualize properties from visual object
  bool showEdges = objMesh_->getVisualizeProperty(MeshVisualizePropertyType::Edges, viewportId);
  bool showNodes = objMesh_->getVisualizeProperty(MeshVisualizePropertyType::Points, viewportId);
  bool smoothShading =
    !objMesh_->getVisualizeProperty(MeshVisualizePropertyType::FlatShading, viewportId);

  // current Drawer's attributes
  Handle(MeshVS_Drawer) drawer = internalData_->meshPrs->GetDrawer();

  if (objMesh_->getRedrawFlag(viewportId))
  {
    internalData_->needRedisplay = true;
    // FIXME: We only support the edges and nodes for now
    bool prevShowEdges;
    bool prevShowNodes;
    drawer->GetBoolean(MeshVS_DA_ShowEdges, prevShowEdges);
    drawer->GetBoolean(MeshVS_DA_DisplayNodes, prevShowNodes);
    if (prevShowEdges != showEdges)
    {
      drawer->SetBoolean(MeshVS_DA_ShowEdges, showEdges);
    }
    if (prevShowNodes != showNodes)
    {
      drawer->SetBoolean(MeshVS_DA_DisplayNodes, showNodes);
    }

    // Colors
    const auto& frontColor = objMesh_->getFrontColor(false, viewportId);
    const auto& backColor  = objMesh_->getBackColor(viewportId);
    const auto& edgesColor = objMesh_->getEdgesColor(viewportId);

    Quantity_Color prevFrontColor;
    Quantity_Color prevBackColor;
    Quantity_Color prevEdgesColor;
    drawer->GetColor(MeshVS_DA_InteriorColor, prevFrontColor);
    drawer->GetColor(MeshVS_DA_InteriorColor, prevBackColor);
    drawer->GetColor(MeshVS_DA_EdgeColor, prevEdgesColor);
    internalData_->meshPrs->GetBuilderById(InternalData::PrsBuilderType_Main)
      ->GetDrawer()
      ->GetColor(MeshVS_DA_InteriorColor, prevFrontColor);

    if (!isColorEqual(prevFrontColor, frontColor))
    {
      drawer->SetColor(MeshVS_DA_InteriorColor, toQuantityColor(frontColor));
    }
    if (!isColorEqual(prevBackColor, backColor))
    {
      drawer->SetColor(MeshVS_DA_BackInteriorColor, toQuantityColor(backColor));
    }
    if (!isColorEqual(prevEdgesColor, edgesColor))
    {
      drawer->SetColor(MeshVS_DA_EdgeColor, toQuantityColor(edgesColor));
    }

    // TODO: FacesColorMap and VertsColorMap

    // Advanced shading
    bool prevSmoothShading;
    drawer->GetBoolean(MeshVS_DA_SmoothShading, prevSmoothShading);
    if (prevSmoothShading != smoothShading)
    {
      drawer->SetBoolean(MeshVS_DA_SmoothShading, smoothShading);
    }
  }
}

void RenderInteractiveMeshObject::createInteractiveObject_(
  [[maybe_unused]] const ModelRenderParams& params)
{
  // We only create once
  if (internalData_->isMeshPrsValid)
  {
    return;
  }

  internalData_->meshPrs = new MeshVS_Mesh();
  internalData_->meshPrs->SetDisplayMode(MeshVS_DMF_Shading);
  internalData_->meshPrs->SetMeshSelMethod(MeshVS_MSM_PRECISE); // select whole mesh
  internalData_->meshPrs->GetDrawer()->SetBoolean(MeshVS_DA_ColorReflection, true);

  internalData_->dirtyFlags &= ~DIRTY_MESH;
  internalData_->meshDataSource = new MR_MeshDataSource(*objMesh_->mesh());
  internalData_->meshPrs->SetDataSource(internalData_->meshDataSource);

  updateLocationFromWorldXf(internalData_->meshPrs, objMesh_->worldXf(), false);

  // Add Builders
  // We can get the builder by id.
  Handle(MeshVS_MeshPrsBuilder) mainBuilder =
    new MeshVS_MeshPrsBuilder(internalData_->meshPrs,
                              MeshVS_DMF_OCCMask,
                              internalData_->meshDataSource,
                              InternalData::PrsBuilderType_Main);
  internalData_->meshPrs->AddBuilder(mainBuilder, true);

  // Optional
  internalData_->meshPrs->SetOwner(this);

  internalData_->isMeshPrsValid = true;
}
} // namespace MR
