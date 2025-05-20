#pragma once

#include "MRViewer/exports.h"
#include "MRViewerEventsListener.h"
#include "MRViewerFwd.h"
#include <AIS_ViewController.hxx>
#include <memory>

// Forward declarations
class Prs3d_Drawer;
class AIS_Shape;
class AIS_InteractiveObject;

namespace MR {

// Using protected inheritance because:
// Public members from AIS_ViewController become protected in ViewController
// Prevents external code from directly accessing AIS_ViewController's interface
// ViewController can still use these features internally
class MRVIEWER_CLASS ViewController
    : protected AIS_ViewController,
      public MultiListener<PostResizeListener, PreDrawListener, DrawListener,
                           PostDrawListener, MouseMoveListener,
                           MouseDownListener, MouseUpListener,
                           MouseScrollListener> {
public:
  MR_DELETE_MOVE(ViewController);

  MRVIEWER_API ViewController();
  MRVIEWER_API ~ViewController();

  MRVIEWER_API void initialize();
  MRVIEWER_API void shutdown();

  //! Add an ais object to the ais context.
  MRVIEWER_API void addAisObject(const Handle(AIS_InteractiveObject) &
                                 theAisObject);

protected:
  //! Initialize OCCT Rendering System.
  void initOCCTRenderingSystem();

  //! Fill 3D Viewer with a DEMO items.
  //! @note This is a temporary function to fill the 3D Viewer with a DEMO
  //! items.
  //! FIXME: It should be removed after the DEMO items are implemented.
  void initDemoScene();

  //! Initialize OCCT 3D Viewer.
  void initV3dViewer();

  //! Initialize OCCT AIS Context.
  void initAisContext();

  //! Initialize OCCT Visual Settings.
  void initVisualSettings();

  //! Initialize off-screen rendering.
  void initOffscreenRendering();

  //! @name Listeners
protected:
  void postResize_(int w, int h) override;

  void preDraw_() override;

  void draw_() override;

  void postDraw_() override;

  bool onMouseDown_(MouseButton btn, int modifiers) override;

  bool onMouseUp_(MouseButton btn, int modifiers) override;

  bool onMouseMove_(int x, int y) override;

  bool onMouseScroll_(float delta) override;

  //! @name Helper functions
private:
  gp_Pnt screenToViewCoordinates(int theX, int theY) const;

  //! Configure the highlight style for the given drawer
  void configureHighlightStyle(const Handle(Prs3d_Drawer) & theDrawer);

  //! Get the default AIS drawer for nice shape display (shaded with edges)
  Handle(Prs3d_Drawer) getDefaultAISDrawer();

  bool isMouseInViewport(int thePosX, int thePosY,
                         const Vector2i &framebufferSize,
                         const ViewportRectangle &viewportRect) const;

  //! Adjust mouse position based on current viewport
  Graphic3d_Vec2i
  adjustMousePosition(int thePosX, int thePosY, const Vector2i &framebufferSize,
                      const ViewportRectangle &viewportRect) const;

private:
  struct ViewInternal;
  std::unique_ptr<ViewInternal> internal_;
};

} // namespace MR
