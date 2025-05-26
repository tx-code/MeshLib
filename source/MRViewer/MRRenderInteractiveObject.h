#pragma once

#include "MRMesh/MRIRenderObject.h"

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

protected:
  // This is called when the object is updated from the visual object
  virtual void syncPropertiesFromVisualObject_(ViewportId viewportId) = 0;

  //! Create the AIS object.
  virtual void createInteractiveObject_(const ModelRenderParams& params) = 0;
};
} // namespace MR