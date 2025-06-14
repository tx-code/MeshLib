#pragma once

#include "MRMeshFwd.h"
#include "MRVisualObject.h"
#include "MRXfBasedCache.h"
#include <vector>

namespace MR
{

enum class MRMESH_CLASS TopoShapeVisualizePropertyType
{
  Points, // Vertices
  Edges,  // FaceBoundaries
  Transparency,
  _count [[maybe_unused]]
};

// clang-format off
template <>
struct IsVisualizeMaskEnum<TopoShapeVisualizePropertyType> : std::true_type{};

// clang-format on

// Note: This class acts as a base holder for topological shapes,
// but currently only manages visualization-related properties.
// The actual storage and interfaces for TopoDS_Shape and related data
// will be provided in the ObjectTopoShape concrete implementation.

// ----------------------------------------------------------------------------
// Mapping between mesh triangles and B-Rep faces.
// We reuse MRMesh 中通用的 `std::vector<FaceBitSet>`（各 region 即一个 B-Rep 面），
// 这样无需再在 MRMesh 层重复定义 Topo*Id 等类型。
// ----------------------------------------------------------------------------

struct TriTopoFaceBiMap
{
    // index == TopoFaceId (implicit); FaceBitSet contains triangles belonging to the face
    std::vector<FaceBitSet> faceRegions;
};

class MRMESH_CLASS ObjectTopoShapeHolder : public VisualObject
{
public:
  MRMESH_API             ObjectTopoShapeHolder();
  ObjectTopoShapeHolder& operator=(ObjectTopoShapeHolder&&) noexcept = default;
  ObjectTopoShapeHolder(ObjectTopoShapeHolder&&) noexcept            = default;
  virtual ~ObjectTopoShapeHolder()                                   = default;

  constexpr static const char* TypeName() noexcept { return "TopoShapeHolder"; }

  virtual const char* typeName() const override { return TypeName(); }

  [[nodiscard]] MRMESH_API bool supportsVisualizeProperty(AnyVisualizeMaskEnum type) const override;

  /// get all visualize properties masks
  MRMESH_API AllVisualizeProperties getAllVisualizeProperties() const override;
  /// returns mask of viewports where given property is set
  MRMESH_API const ViewportMask& getVisualizePropertyMask(AnyVisualizeMaskEnum type) const override;

  /// sets width of lines on screen in pixels
  MRMESH_API virtual void setLineWidth(float size);
  /// sets size of points on screen in pixels
  MRMESH_API virtual void setPointSize(float size);

  /// returns width of lines on screen in pixels
  virtual float getLineWidth() const { return lineWidth_; }

  /// returns size of points on screen in pixels
  virtual float getPointSize() const { return pointSize_; }

  const Color& getEdgesColor(ViewportId id = {}) const { return edgesColor_.get(id); }

  virtual void setEdgesColor(const Color& color, ViewportId id = {})
  {
    edgesColor_.set(color, id);
    needRedraw_ = true;
  }

  const Color& getPointsColor(ViewportId id = {}) const { return pointsColor_.get(id); }

  virtual void setPointsColor(const Color& color, ViewportId id = {})
  {
    pointsColor_.set(color, id);
    needRedraw_ = true;
  }

  /// 获取网格；若传入 map 指针，则返回三角形-TopoFace 双向映射
  MRMESH_API const std::shared_ptr<Mesh>& getMesh( TriTopoFaceBiMap* map = nullptr ) const;

protected:
  ObjectTopoShapeHolder(const ObjectTopoShapeHolder& other) = default;

  // Extracts a mesh from the shape and stores it in the mesh_ member.
  // Implementation should be done in derived class.
  virtual void extractMeshFromShape_() const {}
  
  mutable std::optional<std::shared_ptr<Mesh>> mesh_;
  mutable std::optional<TriTopoFaceBiMap>      triFaceMap_;

  /// width on lines on screen in pixels
  float lineWidth_{1.0f};
  float pointSize_{5.f};

  ViewportMask showEdges_;
  ViewportMask showPoints_;
  ViewportMask transparency_;

  ViewportProperty<Color> edgesColor_;
  ViewportProperty<Color> pointsColor_;

private:
  /// this is private function to set default colors of this type (ObjectMeshHolder) in constructor
  /// only
  void setDefaultColors_();
};

} // namespace MR