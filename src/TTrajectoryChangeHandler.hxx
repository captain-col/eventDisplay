#ifndef TTrajectoryChangeHandler_hxx_seen
#define TTrajectoryChangeHandler_hxx_seen

#include "TVEventChangeHandler.hxx"

namespace CP {
    class TTrajectoryChangeHandler;
};

class TEveElementList;

/// Handle drawing the trajectories.
class CP::TTrajectoryChangeHandler: public TVEventChangeHandler {
public:
    TTrajectoryChangeHandler();
    ~TTrajectoryChangeHandler();

    /// Draw the trajectories into the current scene.
   virtual void Apply();

private:

    /// The trajectories to draw in the event.
    TEveElementList* fTrajectoryList;

};

#endif
