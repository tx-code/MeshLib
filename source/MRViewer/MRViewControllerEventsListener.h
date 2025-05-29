#pragma once

#include "MRViewerEventsListener.h"

namespace MR
{

// @brief Used to listen to AIS_ViewController::OnSelectionChanged.
// Maybe you already find there is a MR::ISceneSelectionChange interface
// but It will not monitor Any Sub-Selections in AIS Context.
class MRVIEWER_CLASS AISSelectionChangeListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(AISSelectionChangeListener);
  virtual ~AISSelectionChangeListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void selectionChange_() = 0;
};
} // namespace MR