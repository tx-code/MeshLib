#include "MRViewControllerEventsListener.h"
#include "MRViewer.h"
#include "MRViewController.h"

namespace MR
{
void AISSelectionChangeListener::connect(Viewer*                           viewer,
                                         int                               group,
                                         boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().selectionChangeSignal.connect(
    group,
    MAKE_SLOT(&AISSelectionChangeListener::selectionChange_),
    pos);
}
} // namespace MR