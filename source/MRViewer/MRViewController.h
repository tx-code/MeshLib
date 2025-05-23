#pragma once

#include "MRViewer/exports.h"
#include "MRViewerEventsListener.h"
#include "MRViewerFwd.h"
#include <AIS_ViewController.hxx>
#include <Standard_Handle.hxx>
#include <memory>

// Forward declarations
class Prs3d_Drawer;
class AIS_Shape;
class AIS_InteractiveObject;
class AIS_InteractiveContext;
class V3d_View;
class V3d_Viewer;

namespace MR
{

// Using protected inheritance because:
// Public members from AIS_ViewController become protected in ViewController
// Prevents external code from directly accessing AIS_ViewController's interface
// ViewController can still use these features internally
class MRVIEWER_CLASS ViewController : protected AIS_ViewController,
                                      public MultiListener<PostResizeListener,
                                                           PreDrawListener,
                                                           DrawListener,
                                                           PostDrawListener,
                                                           MouseMoveListener,
                                                           MouseDownListener,
                                                           MouseUpListener,
                                                           MouseScrollListener>
{
public:
  MR_DELETE_MOVE(ViewController);

  MRVIEWER_API ViewController();
  MRVIEWER_API ~ViewController();

  MRVIEWER_API void initialize();
  MRVIEWER_API void shutdown();

  //! @brief Adds an interactive object to the scene.
  //!
  //! This method displays the given graphics object in the scene.
  //! The behavior regarding object selection upon addition can be controlled
  //! by the `disableSelectionOnAdd` parameter.
  //!
  //! @param object The graphics object to be added to the scene.
  //! @param mrObject The MRObject to be added to the scene.
  //! @param disableSelectionOnAdd If true, the object will be added without
  //!                              activating it for selection, and it will not
  //!                              affect the current selection state. This is useful
  //!                              for auxiliary objects like measurement helpers that
  //!                              should not be selectable by default.
  //!                              If false (default), the object is added and will
  //!                              participate in selection mechanisms according to
  //!                              the AIS_InteractiveContext's current settings (e.g.,
  //!                              auto-activation of selection).
  //! @return true if the object was added (displayed) successfully, false otherwise.
  MRVIEWER_API bool addObject(const Handle(AIS_InteractiveObject)& object,
                              const Object*                        mrObject,
                              bool                                 disableSelectionOnAdd = false);

  //! Add an ais object to the ais context.
  MRVIEWER_API void addAisObject(const Handle(AIS_InteractiveObject)& theAisObject);

  //! Getters for OCCT objects
  MRVIEWER_API const Handle(AIS_InteractiveContext)& getAisContext() const;
  MRVIEWER_API const Handle(V3d_View)&               getView() const;
  MRVIEWER_API const Handle(V3d_Viewer)&             getViewer() const;

  //! @name override AIS_ViewController methods
protected:
  void OnSelectionChanged(const Handle(AIS_InteractiveContext)& theCtx,
                          const Handle(V3d_View)&               theView) override;

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

  //! Override the default render object for MeshLib's RenderXXXObject with our own
  //! RenderInteractiveXXXObject.
  void registerRenderInteractiveObjects();

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
  void configureHighlightStyle(const Handle(Prs3d_Drawer)& theDrawer);

  //! Get the default AIS drawer for nice shape display (shaded with edges)
  Handle(Prs3d_Drawer) getDefaultAISDrawer();

  //! Check if the mouse is in the viewport.
  //! But we use the Viewer::renderWindowHasFocus to check if the mouse is in the viewport now.
  bool isMouseInViewport(int                      thePosX,
                         int                      thePosY,
                         const Vector2i&          framebufferSize,
                         const ViewportRectangle& viewportRect) const;

  //! Adjust mouse position based on current viewport
  Graphic3d_Vec2i adjustMousePosition(int                      thePosX,
                                      int                      thePosY,
                                      const Vector2i&          framebufferSize,
                                      const ViewportRectangle& viewportRect) const;

  //! Synchronize the render objects with the scene objects.
  //!
  //! This function ensures that the render objects (i.e. the objects that are
  //! actually rendered) match the scene objects (i.e. the objects that are
  //! present in the scene).
  //!
  //! If the render objects and scene objects are out of sync, this function
  //! will update the render objects to match the scene objects.
  //!
  //! @param needRedraw Set to true if the scene needs to be redrawn after
  //!                   calling this function.
  void syncRenderObjectsWithScene(bool& needRedraw);

private:
  struct ViewInternal;
  std::unique_ptr<ViewInternal> internal_;
};

} // namespace MR
