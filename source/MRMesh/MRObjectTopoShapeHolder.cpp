#include "MRObjectTopoShapeHolder.h"
#include "MRObjectFactory.h"
#include "MRSceneColors.h"
#include "MRViewportId.h"

namespace MR
{

MR_ADD_CLASS_FACTORY(ObjectTopoShapeHolder);

ObjectTopoShapeHolder::ObjectTopoShapeHolder()
{
  setDefaultColors_();
}

bool ObjectTopoShapeHolder::supportsVisualizeProperty(AnyVisualizeMaskEnum type) const
{
  return VisualObject::supportsVisualizeProperty(type)
         || type.tryGet<TopoShapeVisualizePropertyType>().has_value();
}

AllVisualizeProperties ObjectTopoShapeHolder::getAllVisualizeProperties() const
{
  AllVisualizeProperties ret = VisualObject::getAllVisualizeProperties();
  getAllVisualizePropertiesForEnum<TopoShapeVisualizePropertyType>(ret);
  return ret;
}

const ViewportMask& ObjectTopoShapeHolder::getVisualizePropertyMask(AnyVisualizeMaskEnum type) const
{
  if (auto value = type.tryGet<TopoShapeVisualizePropertyType>())
  {
    switch (*value)
    {
      case TopoShapeVisualizePropertyType::Edges:
        return showEdges_;
      case TopoShapeVisualizePropertyType::Points:
        return showPoints_;
      case TopoShapeVisualizePropertyType::Transparency:
        return transparency_;
      case TopoShapeVisualizePropertyType::_count:
        break; // MSVC warns if this is missing, despite `[[maybe_unused]]` on the `_count`.
    }
    assert(false && "Invalid enum.");
    return visibilityMask_;
  }
  else
  {
    return VisualObject::getVisualizePropertyMask(type);
  }
}

void ObjectTopoShapeHolder::setLineWidth(float size)
{
  if (lineWidth_ == size)
    return;
  lineWidth_  = size;
  needRedraw_ = true;
}

void ObjectTopoShapeHolder::setPointSize(float size)
{
  if (pointSize_ == size)
    return;
  pointSize_  = size;
  needRedraw_ = true;
}

const std::shared_ptr<Mesh>& ObjectTopoShapeHolder::getMesh(TriTopoFaceBiMap* map) const
{
  static const std::shared_ptr<Mesh> emptyMesh;

  if (!mesh_)
    extractMeshFromShape_();

  // if mesh extraction failed
  if (!mesh_)
    return emptyMesh;

  // copy mapping if requested and available
  if (map && triFaceMap_)
    *map = *triFaceMap_;

  return mesh_.value();
}

void ObjectTopoShapeHolder::setDefaultColors_()
{
  setFrontColor(SceneColors::get(SceneColors::SelectedObjectMesh), true);
  setFrontColor(SceneColors::get(SceneColors::UnselectedObjectMesh), false);
  edgesColor_.set(SceneColors::get(SceneColors::Edges));
  pointsColor_.set(SceneColors::get(SceneColors::Points));
}

} // namespace MR