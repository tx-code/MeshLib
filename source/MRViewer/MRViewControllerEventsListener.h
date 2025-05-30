#pragma once

#include "MRViewerEventsListener.h"
#include "exports.h"

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

// (try) start dragging object
class MRVIEWER_CLASS ObjectStartDraggingListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(ObjectStartDraggingListener);
  virtual ~ObjectStartDraggingListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void objectStartDragging_() = 0;
};

// dragging interaction is confirmed.
class MRVIEWER_CLASS ObjectConfirmedDraggingListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(ObjectConfirmedDraggingListener);
  virtual ~ObjectConfirmedDraggingListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void objectConfirmedDragging_() = 0;
};

// perform dragging (update position)
class MRVIEWER_CLASS ObjectUpdateDraggingListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(ObjectUpdateDraggingListener);
  virtual ~ObjectUpdateDraggingListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void objectUpdateDragging_() = 0;
};

// stop dragging (save position)
class MRVIEWER_CLASS ObjectStopDraggingListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(ObjectStopDraggingListener);
  virtual ~ObjectStopDraggingListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void objectStopDragging_() = 0;
};

// abort dragging (restore initial position)
class MRVIEWER_CLASS ObjectAbortDraggingListener : public ConnectionHolder
{
public:
  MR_ADD_CTOR_DELETE_MOVE(ObjectAbortDraggingListener);
  virtual ~ObjectAbortDraggingListener() = default;

  MRVIEWER_API virtual void connect(Viewer*                           viewer,
                                    int                               group,
                                    boost::signals2::connect_position pos) override;

protected:
  virtual void objectAbortDragging_() = 0;
};

} // namespace MR