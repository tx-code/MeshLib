#include "MRRenderInteractiveObject.h"
#include "MRViewController.h"

#include <gp_Trsf.hxx>
#include <AIS_InteractiveContext.hxx>

namespace MR
{
void RenderInteractiveObject::updateLocationFromWorldXf(const Handle(AIS_InteractiveObject)& obj,
                                                        const AffineXf3f& worldXf,
                                                        bool              forceInvalidate)
{
  auto&   viewController = ViewController::getViewControllerInstance();
  auto&   context        = viewController.getAisContext();
  gp_Trsf localTrans;
  localTrans.SetValues(worldXf.A.x.x,
                       worldXf.A.x.y,
                       worldXf.A.x.z,
                       worldXf.b.x,
                       worldXf.A.y.x,
                       worldXf.A.y.y,
                       worldXf.A.y.z,
                       worldXf.b.y,
                       worldXf.A.z.x,
                       worldXf.A.z.y,
                       worldXf.A.z.z,
                       worldXf.b.z);
  context->SetLocation(obj, localTrans);
  if (forceInvalidate)
    viewController.forceInvalidate();
}
} // namespace MR
