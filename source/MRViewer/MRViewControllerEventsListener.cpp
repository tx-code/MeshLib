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

void ObjectStartDraggingListener::connect(Viewer*                           viewer,
                                          int                               group,
                                          boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().objectStartDraggingSignal.connect(
    group,
    MAKE_SLOT(&ObjectStartDraggingListener::objectStartDragging_),
    pos);
}

void ObjectConfirmedDraggingListener::connect(Viewer*                           viewer,
                                              int                               group,
                                              boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().objectConfirmedDraggingSignal.connect(
    group,
    MAKE_SLOT(&ObjectConfirmedDraggingListener::objectConfirmedDragging_),
    pos);
}

void ObjectUpdateDraggingListener::connect(Viewer*                           viewer,
                                           int                               group,
                                           boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().objectUpdateDraggingSignal.connect(
    group,
    MAKE_SLOT(&ObjectUpdateDraggingListener::objectUpdateDragging_),
    pos);
}

void ObjectStopDraggingListener::connect(Viewer*                           viewer,
                                         int                               group,
                                         boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().objectStopDraggingSignal.connect(
    group,
    MAKE_SLOT(&ObjectStopDraggingListener::objectStopDragging_),
    pos);
}

void ObjectAbortDraggingListener::connect(Viewer*                           viewer,
                                          int                               group,
                                          boost::signals2::connect_position pos)
{
  if (!viewer)
    return;
  connection_ = viewer->getViewController().objectAbortDraggingSignal.connect(
    group,
    MAKE_SLOT(&ObjectAbortDraggingListener::objectAbortDragging_),
    pos);
}
} // namespace MR