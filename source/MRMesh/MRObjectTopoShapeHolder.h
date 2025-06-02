#pragma once

#include "MRMeshFwd.h"
#include "MRVisualObject.h"
#include "MRXfBasedCache.h"

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

protected:
  ObjectTopoShapeHolder(const ObjectTopoShapeHolder& other) = default;
  
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