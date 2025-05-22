#include "MRViewController.h"
#include "MRMesh/MRBox.h"
#include "MRMesh/MRMesh.h"
#include "MRMouseController.h"
#include "MRViewer.h"
#include "MRViewer/MRRibbonMenu.h"
#include "MRViewport.h"
#include "imgui.h"

// OCCT
#include <AIS_AnimationCamera.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <AIS_Triangulation.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <ElSLib.hxx>
#include <GeomAbs_Shape.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Graphic3d_Vec2.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_Texture.hxx>
#include <Poly_Triangulation.hxx>
#include <ProjLib.hxx>
#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Prs3d_TypeOfHLR.hxx>
#include <Prs3d_TypeOfHighlight.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>
#include <SelectMgr_PickingStrategy.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_Light.hxx>
#include <V3d_TypeOfView.hxx>
#include <V3d_View.hxx>

// Platform specific includes for Aspect_Window
#if defined(_WIN32)
  #include <WNT_Window.hxx>
  #define GLFW_EXPOSE_NATIVE_WIN32
  #define GLFW_EXPOSE_NATIVE_WGL
#elif defined(__APPLE__)
  #include <Cocoa_Window.hxx>
  #define GLFW_EXPOSE_NATIVE_COCOA
  #define GLFW_EXPOSE_NATIVE_NSGL
#else // Linux/X11
  #include <Xw_Window.hxx>
  #define GLFW_EXPOSE_NATIVE_X11
  #define GLFW_EXPOSE_NATIVE_GLX
#endif
#include "MRViewer/MRGladGlfw.h"
#include <GLFW/glfw3native.h> // For native access

#define LOG_VIEW_CONTROLLER 1

//! Anonymous namespace for internal functions
namespace
{

//! Create an opengl driver
Handle(OpenGl_GraphicDriver) createOpenGlDriver(
  const Handle(Aspect_DisplayConnection)& displayConnection,
  bool                                    glDebug = false)
{
  Handle(OpenGl_GraphicDriver) aGraphicDriver =
    new OpenGl_GraphicDriver(displayConnection, Standard_False);
  aGraphicDriver->ChangeOptions().buffersNoSwap = Standard_True;
  // contextCompatible is needed to support line thickness
  aGraphicDriver->ChangeOptions().contextCompatible = !glDebug;
  aGraphicDriver->ChangeOptions().ffpEnable         = false;
  aGraphicDriver->ChangeOptions().contextDebug      = glDebug;
  aGraphicDriver->ChangeOptions().contextSyncDebug  = glDebug;
  return aGraphicDriver;
}

Aspect_VKeyMouse toAspectKeyMouse(MR::MouseButton btn)
{
  switch (btn)
  {
    case MR::MouseButton::Left:
      return Aspect_VKeyMouse_LeftButton;
    case MR::MouseButton::Right:
      return Aspect_VKeyMouse_RightButton;
    case MR::MouseButton::Middle:
      return Aspect_VKeyMouse_MiddleButton;
    default:
      return Aspect_VKeyMouse_NONE;
  }
}

Aspect_VKeyFlags toAspectKeyFlags(int modifiers)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if ((modifiers & GLFW_MOD_SHIFT) != 0)
  {
    aFlags |= Aspect_VKeyFlags_SHIFT;
  }
  if ((modifiers & GLFW_MOD_CONTROL) != 0)
  {
    aFlags |= Aspect_VKeyFlags_CTRL;
  }
  if ((modifiers & GLFW_MOD_ALT) != 0)
  {
    aFlags |= Aspect_VKeyFlags_ALT;
  }
  if ((modifiers & GLFW_MOD_SUPER) != 0)
  {
    aFlags |= Aspect_VKeyFlags_META;
  }
  return aFlags;
}
} // namespace

namespace MR
{
struct ViewController::ViewInternal
{
  //! GLFW window
  GLFWwindow* glfwWindow{nullptr};
  //! OCCT's wrapper for the window
  Handle(Aspect_Window) occtAspectWindow;
  //! OCCT 3D Viewer
  Handle(V3d_Viewer) viewer;
  //! OCCT 3D View
  Handle(V3d_View) view;
  //! 3D position in the view (converted from screen coordinates)
  gp_Pnt positionInView;
  //! OCCT Interactive Context
  Handle(AIS_InteractiveContext) context;
  //! Initial width of the offscreen render target
  int renderWidth{1280};
  //! Initial height of the offscreen render target
  int renderHeight{720};
  //! AIS ViewCube for scene orientation
  Handle(AIS_ViewCube) viewCube;
  //! If true, ViewCube animation completes in a single update
  bool fixedViewCubeAnimationLoop{false};

  //! OpenGL graphics context
  Handle(OpenGl_Context) glContext;
  //! Framebuffer for offscreen rendering
  Handle(OpenGl_FrameBuffer) offscreenFBO;
  //! Position of the ImGui viewport displaying the render
  Vector2f viewPos{0.0f, 0.0f};
  //! True if the ImGui window containing the render has focus
  bool renderWindowHasFocus{false};
  //! Scale factor for the view
  float scaleFactor{1.0f};

  /// Visual properties
  //! Face color
  Quantity_Color faceColor{Quantity_NOC_GRAY90};
  //! Edge color
  Quantity_Color edgeColor{Quantity_NOC_BLACK};
  //! Edge width
  double boundaryEdgeWidth{2.0};
  //! Vertex color
  Quantity_Color vertexColor{Quantity_NOC_BLACK};
  //! Vertex size
  double vertexSize{2.0};

  //! Selection color
  Quantity_Color selectionColor{Quantity_NOC_RED};
  //! Highlight color
  Quantity_Color highlightColor{Quantity_NOC_YELLOW};
};

ViewController::ViewController()
    : internal_(std::make_unique<ViewInternal>())
{
}

ViewController::~ViewController() = default;

void ViewController::initialize()
{
#if LOG_VIEW_CONTROLLER
  spdlog::info("ViewController::initializing");
#endif

  auto& viewerRef = getViewerInstance();

  assert(viewerRef.window != nullptr);
  internal_->glfwWindow = viewerRef.window;

  if (!internal_->glfwWindow)
  {
    Message::DefaultMessenger()->Send("ViewController: GLFW window is null on construction.",
                                      Message_Fail);
    return;
  }

  // Create the OCCT Aspect_Window wrapper
#if defined(_WIN32)
  internal_->occtAspectWindow =
    new WNT_Window((Aspect_Drawable)glfwGetWin32Window(internal_->glfwWindow));
#elif defined(__APPLE__)
  internal_->occtAspectWindow =
    new Cocoa_Window((Aspect_Drawable)glfwGetCocoaWindow(internal_->glfwWindow));
#else // Linux/X11
  Handle(Aspect_DisplayConnection) aDispConn =
    new Aspect_DisplayConnection((Aspect_XDisplay*)glfwGetX11Display());
  internal_->occtAspectWindow =
    new Xw_Window(aDispConn, (Aspect_Drawable)glfwGetX11Window(internal_->glfwWindow));
#endif
  if (!internal_->occtAspectWindow.IsNull())
  {
    internal_->occtAspectWindow->SetVirtual(Standard_True); // Set as virtual
  }
  else
  {
    spdlog::error("Failed to create OCCT Aspect_Window wrapper.");
  }

  initOCCTRenderingSystem();
  initDemoScene();
}

void ViewController::shutdown()
{
#if LOG_VIEW_CONTROLLER
  spdlog::info("ViewController::shutting down");
#endif

  // We need to release the FBO and the GL context before shutting down manually
  if (!internal_->offscreenFBO.IsNull() && !internal_->glContext.IsNull())
  {
    internal_->offscreenFBO->Release(internal_->glContext.get());
    internal_->offscreenFBO.Nullify();
    internal_->glContext.Nullify();
  }

  if (!internal_->view.IsNull())
  {
    internal_->view->Remove();
  }
  internal_->occtAspectWindow.Nullify();
}

bool ViewController::addObject(const Handle(AIS_InteractiveObject)& object,
                               bool                                 disableSelectionOnAdd)
{
  if (internal_->context.IsNull())
  {
    spdlog::error("ViewController::addObject(): Context is not initialized.");
    return false;
  }

  if (object)
  {
    if (disableSelectionOnAdd)
    {
      // Logic for when selection is disabled on add
      const bool onEntry_AutoActivateSelection = internal_->context->GetAutoActivateSelection();
      const int  defaultDisplayMode            = internal_->context->DisplayMode();
      internal_->context->SetAutoActivateSelection(false);
      internal_->context->Display(
        object,
        defaultDisplayMode,
        -1,
        false); // -1 for selection mode often means no specific selection activation
      internal_->context->SetAutoActivateSelection(onEntry_AutoActivateSelection);
    }
    else
    {
      // Default logic
      internal_->context->Display(object, false);
    }
  }
  return true;
}

void ViewController::addAisObject(const Handle(AIS_InteractiveObject)& theAisObject)
{
  if (theAisObject.IsNull())
  {
    spdlog::error("ViewController::addAisObject(): AIS object is null.");
    return;
  }

  if (internal_->context.IsNull())
  {
    spdlog::error("ViewController::addAisObject(): Context is not initialized.");
    return;
  }

  theAisObject->SetAttributes(getDefaultAISDrawer());
  internal_->context->Display(theAisObject, AIS_Shaded, 0, false);
}

const Handle(AIS_InteractiveContext)& ViewController::getAisContext() const
{
  return internal_->context;
}

const Handle(V3d_View)& ViewController::getView() const
{
  return internal_->view;
}

const Handle(V3d_Viewer)& ViewController::getViewer() const
{
  return internal_->viewer;
}

//---------------------------------------------------------
// AIS_ViewController overrides

void ViewController::OnSelectionChanged(
  [[maybe_unused]] const Handle(AIS_InteractiveContext)& theCtx,
  [[maybe_unused]] const Handle(V3d_View)&               theView)
{

  // TODO: Meaningful implementation
  spdlog::info("Selection changed in OCCT context.");
}

//---------------------------------------------------------
// Init

void ViewController::initOCCTRenderingSystem()
{
  assert(internal_->occtAspectWindow);

  initV3dViewer();
  initAisContext();
  initOffscreenRendering();
  initVisualSettings();
}

void ViewController::initDemoScene()
{
  assert(internal_->context && internal_->view);
  gp_Ax2 anAxis;
  anAxis.SetLocation(gp_Pnt(0.0, 0.0, 0.0));
  Handle(AIS_Shape) aBox = new AIS_Shape(BRepPrimAPI_MakeBox(anAxis, 50, 50, 50).Shape());
  addAisObject(aBox);

  anAxis.SetLocation(gp_Pnt(25.0, 125.0, 0.0));
  Handle(AIS_Shape) aCone = new AIS_Shape(BRepPrimAPI_MakeCone(anAxis, 25, 0, 50).Shape());
  addAisObject(aCone);

  TCollection_AsciiString aGlInfo;
  {
    TColStd_IndexedDataMapOfStringString aRendInfo;
    internal_->view->DiagnosticInformation(aRendInfo, Graphic3d_DiagnosticInfo_Basic);
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aRendInfo); aValueIter.More();
         aValueIter.Next())
    {
      if (!aGlInfo.IsEmpty())
      {
        aGlInfo += "\n";
      }
      aGlInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " + aValueIter.Value();
    }
  }
  spdlog::info("OpenGL info: \n{}", aGlInfo.ToCString());
}

void ViewController::initV3dViewer()
{
#if LOG_VIEW_CONTROLLER
  spdlog::info("Initializing OCCT 3D Viewer.");
#endif

  assert(internal_->occtAspectWindow);

  Handle(Aspect_DisplayConnection) aDispConn;
#if !defined(_WIN32) && !defined(__APPLE__) // For X11
  aDispConn = internal_->occtAspectWindow->GetDisplay();
#else
  aDispConn = new Aspect_DisplayConnection();
#endif
  Handle(OpenGl_GraphicDriver) aGraphicDriver = createOpenGlDriver(aDispConn);

  auto& viewer_ = internal_->viewer; // shortcut
  auto& view_   = internal_->view;   // shortcut

  viewer_ = new V3d_Viewer(aGraphicDriver);
  viewer_->SetLightOn(new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE, true));
  viewer_->SetLightOn(new V3d_AmbientLight(Quantity_NOC_WHITE));
  viewer_->SetDefaultTypeOfView(V3d_ORTHOGRAPHIC);
  viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
  view_ = viewer_->CreateView();
  view_->SetImmediateUpdate(Standard_False);

  Aspect_RenderingContext aNativeGlContext = NULL;
#if defined(_WIN32)
  aNativeGlContext = glfwGetWGLContext(internal_->glfwWindow);
#elif defined(__APPLE__)
  aNativeGlContext = (Aspect_RenderingContext)glfwGetNSGLContext(internal_->glfwWindow);
#else // Linux/X11
  aNativeGlContext = glfwGetGLXContext(internal_->glfwWindow);
#endif
  view_->SetWindow(internal_->occtAspectWindow, aNativeGlContext);
  // Dark theme
  // TODO: Make this configurable
  view_->SetBgGradientColors(
    Quantity_Color(100 / 255.0, 100 / 255.0, 100 / 255.0, Quantity_TOC_RGB),
    Quantity_Color(200 / 255.0, 200 / 255.0, 200 / 255.0, Quantity_TOC_RGB),
    Aspect_GFM_VER);

  internal_->view->ChangeRenderingParams().ToShowStats = Standard_True;
  internal_->view->ChangeRenderingParams().CollectedStats =
    Graphic3d_RenderingParams::PerfCounters_All;

  internal_->glContext = aGraphicDriver->GetSharedContext();
  if (internal_->glContext.IsNull())
  {
    spdlog::error("ViewController: Failed to get OpenGl_Context.");
    assert(false);
  }
}

void ViewController::initAisContext()
{
#if LOG_VIEW_CONTROLLER
  spdlog::info("Initializing OCCT AIS Context.");
#endif

  assert(internal_->viewer.IsNull() == false);
  auto& context_ = internal_->context; // shortcut

  context_ = new AIS_InteractiveContext(internal_->viewer);
  context_->SetAutoActivateSelection(true);
  context_->SetToHilightSelected(false);
  context_->SetPickingStrategy(SelectMgr_PickingStrategy_OnlyTopmost);
  context_->SetDisplayMode(AIS_Shaded, false);
  context_->EnableDrawHiddenLine();
  context_->SetPixelTolerance(2);

  auto& default_drawer = context_->DefaultDrawer();
  default_drawer->SetWireAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0));
  default_drawer->SetTypeOfHLR(Prs3d_TOH_PolyAlgo);

  constexpr bool s_display_viewcube = true;

  if constexpr (s_display_viewcube)
  {
    internal_->viewCube = new AIS_ViewCube();
    internal_->viewCube->SetBoxColor(Quantity_NOC_GRAY75);
    internal_->viewCube->SetSize(55);
    internal_->viewCube->SetFontHeight(10);
    internal_->viewCube->SetAxesLabels("X", "Y", "Z");
    internal_->viewCube->SetTransformPersistence(
      new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers,
                                  Aspect_TOTP_RIGHT_LOWER,
                                  Graphic3d_Vec2i(85, 85)));
    if (!internal_->viewCube->Attributes()->HasOwnDatumAspect())
    {
      Handle(Prs3d_DatumAspect) datumAspect = new Prs3d_DatumAspect();
      datumAspect->ShadingAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED2);
      datumAspect->ShadingAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN2);
      datumAspect->ShadingAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE2);
      internal_->viewCube->Attributes()->SetDatumAspect(datumAspect);
    }

    if (this->ViewAnimation())
    {
      internal_->viewCube->SetViewAnimation(this->ViewAnimation());
    }
    else
    {
      spdlog::warn("ViewController: ViewAnimation not available for ViewCube.");
    }

    if (internal_->fixedViewCubeAnimationLoop)
    {
      internal_->viewCube->SetDuration(0.1);
      internal_->viewCube->SetFixedAnimationLoop(true);
    }
    else
    {
      internal_->viewCube->SetDuration(0.5);
      internal_->viewCube->SetFixedAnimationLoop(false);
    }
    internal_->context->Display(internal_->viewCube, false);
  }
}

void ViewController::initVisualSettings()
{
#if LOG_VIEW_CONTROLLER
  spdlog::info("Initializing OCCT Visual Settings.");
#endif

  assert(internal_->context.IsNull() == false);
  auto& context_ = internal_->context; // shortcut

  // Higlight Selected
  Handle(Prs3d_Drawer) selectionDrawer = new Prs3d_Drawer();
  selectionDrawer->SetupOwnDefaults();
  selectionDrawer->SetColor(internal_->selectionColor);
  selectionDrawer->SetDisplayMode(0);
  selectionDrawer->SetZLayer(Graphic3d_ZLayerId_Default);
  selectionDrawer->SetTypeOfDeflection(Aspect_TOD_RELATIVE);
  selectionDrawer->SetDeviationAngle(context_->DeviationAngle());
  selectionDrawer->SetDeviationCoefficient(context_->DeviationCoefficient());
  context_->SetSelectionStyle(selectionDrawer); // equal to
                                                // SetHighlightStyle(Prs3d_TypeOfHighlight_Selected,
                                                // selectionDrawer);
  context_->SetHighlightStyle(Prs3d_TypeOfHighlight_LocalSelected, selectionDrawer);
  context_->SetHighlightStyle(Prs3d_TypeOfHighlight_SubIntensity, selectionDrawer);

  // Higlight Dynamic
  Handle(Prs3d_Drawer) hilightDrawer = new Prs3d_Drawer();
  hilightDrawer->SetupOwnDefaults();
  hilightDrawer->SetColor(internal_->highlightColor);
  hilightDrawer->SetDisplayMode(0);
  hilightDrawer->SetZLayer(Graphic3d_ZLayerId_Top);
  hilightDrawer->SetTypeOfDeflection(Aspect_TOD_RELATIVE);
  hilightDrawer->SetDeviationAngle(context_->DeviationAngle());
  hilightDrawer->SetDeviationCoefficient(context_->DeviationCoefficient());
  context_->SetHighlightStyle(Prs3d_TypeOfHighlight_Dynamic, hilightDrawer);

  // Higlight Local
  Handle(Prs3d_Drawer) hilightLocalDrawer = new Prs3d_Drawer();
  hilightLocalDrawer->SetupOwnDefaults();
  hilightLocalDrawer->SetColor(internal_->highlightColor);
  hilightLocalDrawer->SetDisplayMode(1);
  hilightLocalDrawer->SetZLayer(Graphic3d_ZLayerId_Top);
  hilightLocalDrawer->SetTypeOfDeflection(Aspect_TOD_RELATIVE);
  hilightLocalDrawer->SetDeviationAngle(context_->DeviationAngle());
  hilightLocalDrawer->SetDeviationCoefficient(context_->DeviationCoefficient());

  Handle(Prs3d_ShadingAspect) shadingAspect = new Prs3d_ShadingAspect();
  shadingAspect->SetColor(internal_->highlightColor);
  shadingAspect->SetTransparency(0);
  shadingAspect->Aspect()->SetPolygonOffsets((int)Aspect_POM_Fill, 0.99f, 0.0f);
  hilightLocalDrawer->SetShadingAspect(shadingAspect);

  Handle(Prs3d_LineAspect) lineAspect =
    new Prs3d_LineAspect(internal_->highlightColor, Aspect_TOL_SOLID, 3.0);
  hilightLocalDrawer->SetLineAspect(lineAspect);
  hilightLocalDrawer->SetSeenLineAspect(lineAspect);
  hilightLocalDrawer->SetWireAspect(lineAspect);
  hilightLocalDrawer->SetFaceBoundaryAspect(lineAspect);
  hilightLocalDrawer->SetFreeBoundaryAspect(lineAspect);
  hilightLocalDrawer->SetUnFreeBoundaryAspect(lineAspect);

  context_->SetHighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic, hilightLocalDrawer);
}

void ViewController::initOffscreenRendering()
{
  assert(internal_->glContext && internal_->view);

#if LOG_VIEW_CONTROLLER
  spdlog::info("Initializing offscreen rendering.");
#endif

  internal_->offscreenFBO = Handle(OpenGl_FrameBuffer)::DownCast(
    internal_->view->View()->FBOCreate(internal_->renderWidth, internal_->renderHeight));
  if (!internal_->offscreenFBO.IsNull())
  {
    internal_->offscreenFBO->ColorTexture()->Sampler()->Parameters()->SetFilter(
      Graphic3d_TOTF_BILINEAR);
    internal_->view->View()->SetFBO(internal_->offscreenFBO);
    getViewerInstance().colorFrameBufferId = internal_->offscreenFBO->ColorTexture()->TextureId();
  }
  else
  {
    spdlog::error("Failed to create offscreen FBO.");
  }
}

//---------------------------------------------------------
// Listeners

void ViewController::postResize_(int w, int h)
{
  if (w != 0 && h != 0 && !internal_->view.IsNull())
  {
    internal_->view->MustBeResized();
    internal_->view->Invalidate(); // invalidate the whole view
  }
}

void ViewController::preDraw_()
{
  const auto& viewport     = getViewerInstance().viewport();
  const auto& viewportRect = viewport.getViewportRect();

  int  newRenderWidth  = (int)width(viewportRect);
  int  newRenderHeight = (int)height(viewportRect);
  bool needRedraw      = false;

  if (newRenderWidth != internal_->renderWidth || newRenderHeight != internal_->renderHeight)
  {
    internal_->renderWidth  = newRenderWidth;
    internal_->renderHeight = newRenderHeight;

    // Update OCCT's window representation (Aspect_Window)
#if defined(_WIN32)
    Handle(WNT_Window)::DownCast(internal_->occtAspectWindow)
      ->SetPos(0, 0, newRenderWidth, newRenderHeight);
#elif defined(__APPLE__)
    // Note: Cocoa_Window might not have a direct SetSize or equivalent.
    // Behavior might differ on macOS.
    spdlog::debug("OCCT View: Cocoa_Window size update may require "
                  "platform-specific handling for SetSize.");
#else // Linux/X11
    Handle(Xw_Window)::DownCast(internal_->occtAspectWindow)
      ->SetSize(newRenderWidth, newRenderHeight);
#endif
    // If the underlying OCCT window resizes, the V3d_View also needs to be
    // notified.
    internal_->view->MustBeResized();

    // Update V3d_View's camera aspect ratio, Optional
    internal_->view->Camera()->SetAspect(static_cast<float>(newRenderWidth)
                                         / static_cast<float>(newRenderHeight));

    internal_->offscreenFBO->InitLazy(internal_->glContext,
                                      Graphic3d_Vec2i(newRenderWidth, newRenderHeight),
                                      GL_RGB8,
                                      GL_DEPTH24_STENCIL8);

    needRedraw = true;
  }

  auto menu_scaling = getViewerInstance().getMenuPlugin()->menu_scaling();
  if (std::abs(internal_->scaleFactor - menu_scaling) > 0.0001)
  {
    internal_->scaleFactor = menu_scaling;
    internal_->viewCube->SetSize(55 * internal_->scaleFactor, true);
    internal_->viewCube->SetFontHeight(10 * internal_->scaleFactor);
    internal_->viewCube->SetTransformPersistence(new Graphic3d_TransformPers(
      Graphic3d_TMF_TriedronPers,
      Aspect_TOTP_RIGHT_LOWER,
      Graphic3d_Vec2i((int)(85 * internal_->scaleFactor), (int)(85 * internal_->scaleFactor))));
    internal_->viewCube->Redisplay(true);

    float newResolution =
      internal_->view->ChangeRenderingParams().Resolution * internal_->scaleFactor;
    internal_->view->ChangeRenderingParams().Resolution = (int)newResolution;

    needRedraw = true;
  }

  if (needRedraw)
  {
    internal_->view->Redraw();
  }
}

void ViewController::draw_()
{
  if (internal_->view && internal_->context)
  {
    FlushViewEvents(internal_->context, internal_->view, true);
  }
}

void ViewController::postDraw_()
{
  // Let the view animation to update the view
  if (!ViewAnimation()->IsStopped())
  {
    getViewerInstance().incrementForceRedrawFrames();
  }
}

bool ViewController::onMouseDown_(MouseButton btn, int modifiers)
{
  if (!getViewerInstance().renderWindowHasFocus)
  {
    return false;
  }

  if (PressMouseButton(LastMousePosition(),
                       toAspectKeyMouse(btn),
                       toAspectKeyFlags(modifiers),
                       false))
  {
    internal_->view->InvalidateImmediate();
  }

  return true;
}

bool ViewController::onMouseUp_(MouseButton btn, int modifiers)
{
  if (!getViewerInstance().renderWindowHasFocus)
  {
    return false;
  }

  if (ReleaseMouseButton(LastMousePosition(),
                         toAspectKeyMouse(btn),
                         toAspectKeyFlags(modifiers),
                         false))
  {
    internal_->view->InvalidateImmediate();
  }

  return true;
}

bool ViewController::onMouseMove_(int x, int y)
{
  const auto& rect            = getViewerInstance().viewport().getViewportRect();
  const auto& framebufferSize = getViewerInstance().framebufferSize;

  if (!getViewerInstance().renderWindowHasFocus)
  {
    // When the render window loses focus, reset the view input,
    // so that the view is not moved by the mouse
    ResetViewInput();
    return false;
  }

  Graphic3d_Vec2i fixedPos = adjustMousePosition(x, y, framebufferSize, rect);
  if (UpdateMousePosition(fixedPos, PressedMouseButtons(), LastMouseFlags(), false))
  {
    internal_->view->InvalidateImmediate();
  }

  return true;
}

bool ViewController::onMouseScroll_(float delta)
{
  if (!getViewerInstance().renderWindowHasFocus)
  {
    return false;
  }

  const auto& curMousePos     = getViewerInstance().mouseController().getMousePos();
  const auto& rect            = getViewerInstance().viewport().getViewportRect();
  const auto& framebufferSize = getViewerInstance().framebufferSize;

  Graphic3d_Vec2i aAdjustedPos =
    adjustMousePosition(curMousePos.x, curMousePos.y, framebufferSize, rect);
  if (UpdateZoom(Aspect_ScrollDelta(aAdjustedPos, int(delta * myScrollZoomRatio))))
  {
    internal_->view->InvalidateImmediate();
  }
  return true;
}

//---------------------------------------------------------
// Helper functions

Handle(Prs3d_Drawer) ViewController::getDefaultAISDrawer()
{
  // Normal mode drawer
  Handle(Prs3d_ShadingAspect) shadingAspect = new Prs3d_ShadingAspect();
  shadingAspect->SetColor(internal_->faceColor);
  shadingAspect->SetMaterial(Graphic3d_NOM_PLASTIC);
  shadingAspect->SetTransparency(0);
  shadingAspect->Aspect()->SetPolygonOffsets((int)Aspect_POM_Fill, 0.99f, 0.0f);

  Handle(Prs3d_LineAspect) lineAspect =
    new Prs3d_LineAspect(internal_->edgeColor, Aspect_TOL_SOLID, internal_->boundaryEdgeWidth);

  Handle(Prs3d_Drawer) drawer = new Prs3d_Drawer();
  drawer->SetShadingAspect(shadingAspect);
  drawer->SetLineAspect(lineAspect);
  drawer->SetSeenLineAspect(lineAspect);
  drawer->SetWireAspect(lineAspect);
  drawer->SetFaceBoundaryAspect(lineAspect);
  drawer->SetFreeBoundaryAspect(lineAspect);
  drawer->SetUnFreeBoundaryAspect(lineAspect);
  drawer->SetFaceBoundaryUpperContinuity(GeomAbs_C2);
  drawer->SetPointAspect(
    new Prs3d_PointAspect(Aspect_TOM_O_POINT, internal_->vertexColor, internal_->vertexSize));

  drawer->SetFaceBoundaryDraw(true);
  drawer->SetDisplayMode(AIS_Shaded);
  drawer->SetTypeOfDeflection(Aspect_TOD_RELATIVE);
  return drawer;
}

void ViewController::configureHighlightStyle(const Handle(Prs3d_Drawer)& theDrawer)
{
  Handle(Prs3d_ShadingAspect) shadingAspect = new Prs3d_ShadingAspect();
  shadingAspect->SetColor(internal_->highlightColor);
  shadingAspect->SetMaterial(Graphic3d_NOM_PLASTIC);
  shadingAspect->SetTransparency(0);
  shadingAspect->Aspect()->SetPolygonOffsets((int)Aspect_POM_Fill, 0.99f, 0.0f);

  theDrawer->SetShadingAspect(shadingAspect);
  theDrawer->SetDisplayMode(AIS_Shaded);
}

bool ViewController::isMouseInViewport(int                      thePosX,
                                       int                      thePosY,
                                       const Vector2i&          framebufferSize,
                                       const ViewportRectangle& viewportRect) const
{
  // Need to flip y coordinate
  if ((thePosX < viewportRect.min.x) || (thePosX > viewportRect.max.x)
      || ((framebufferSize.y - thePosY) < viewportRect.min.y)
      || ((framebufferSize.y - thePosY) > viewportRect.max.y))
  {
    return false;
  }
  return true;
}

Graphic3d_Vec2i ViewController::adjustMousePosition(int                      thePosX,
                                                    int                      thePosY,
                                                    const Vector2i&          framebufferSize,
                                                    const ViewportRectangle& viewportRect) const
{
  return Graphic3d_Vec2i((int)(thePosX - viewportRect.min.x),
                         (int)(thePosY + viewportRect.max.y - framebufferSize.y));
}

} // namespace MR

#undef LOG_VIEW_CONTROLLER