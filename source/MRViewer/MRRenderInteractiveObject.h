#pragma once

#include "MRMesh/MRIRenderObject.h"
#include "MRViewerFwd.h"
#include "MRMesh/MRAffineXf3.h"

#include <AIS_InteractiveObject.hxx>
#include <Standard_Transient.hxx>

namespace MR
{

//! Base class for all custom AIS render objects.
//! Inherit from Standard_Transient to be able to be the owner of the InteractiveObject.
class RenderInteractiveObject : public virtual IRenderObject, public Standard_Transient
{
public:
  ~RenderInteractiveObject() override = default;

  // Returns the AIS object
  virtual const Handle(AIS_InteractiveObject)& getAisObject() const = 0;

  // We don't need to render anything for picker
  void renderPicker(const ModelBaseRenderParams& params, unsigned geomId) override
  {
    (void)params;
    (void)geomId;
  }

  // Update the location of the Interactive object from the world xf of the visual object
  MRVIEWER_API static void updateLocationFromWorldXf(const Handle(AIS_InteractiveObject)& obj,
                                                     const AffineXf3f&                    worldXf,
                                                     bool forceInvalidate = true);

protected:
  // This is called when the object is updated from the visual object
  virtual void syncPropertiesFromVisualObject_(ViewportId viewportId) = 0;

  //! Create the AIS object.
  virtual void createInteractiveObject_(const ModelRenderParams& params) = 0;
};
} // namespace MR