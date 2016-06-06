#ifndef TFitChangeHandler_hxx_seen
#define TFitChangeHandler_hxx_seen

#include "TVEventChangeHandler.hxx"

#include <TReconBase.hxx>
#include <TReconCluster.hxx>
#include <TReconShower.hxx>
#include <TReconTrack.hxx>
#include <TReconPID.hxx>
#include <TReconVertex.hxx>
#include <THandle.hxx>

#include <string>

namespace CP {
    class TFitChangeHandler;
};

class TEveElementList;

/// Handle drawing the TAlgorithmResults saved in the event.
class CP::TFitChangeHandler: public TVEventChangeHandler {
public:

    TFitChangeHandler();
    ~TFitChangeHandler();
    
    /// Draw fit information into the current scene.
    virtual void Apply();

private:

    /// A method to draw a TReconCluster.
    int ShowReconCluster(TEveElementList* list,
                         const CP::THandle<CP::TReconCluster> obj,
                         int index,
                         bool forceUncertainty);
    
    /// A method to draw a TReconShower.
    int ShowReconShower(TEveElementList* list,
                        const CP::THandle<CP::TReconShower> obj,
                        int index);

    /// A method to draw a TReconTrack.
    int ShowReconTrack(TEveElementList* list,
                       const CP::THandle<CP::TReconTrack> obj,
                       int index);

    /// A method to draw a TReconPID
    int ShowReconPID(TEveElementList* list,
                     const CP::THandle<CP::TReconPID> obj,
                     int index);

    /// A method to draw a TReconVertex
    int ShowReconVertex(TEveElementList* list,
                        const CP::THandle<CP::TReconVertex> obj,
                        int index);

    /// A method to draw a generic TReconBase
    int ShowReconObject(TEveElementList* list,
                        const CP::THandle<CP::TReconBase> obj,
                        int index,
                        bool forceUncertainty);

    /// A method to draw a TReconObjectContainer
    int ShowReconObjects(TEveElementList* list,
                         const CP::THandle<CP::TReconObjectContainer> obj,
                         int index = 0);

    /// The reconstruction objects to draw in the event.
    TEveElementList* fFitList;

    /// The hits to draw in the event.
    TEveElementList* fHitList;

    /// A boolean to flag if hits associated with the fit should be drawn.
    bool fShowFitsHits;

    /// A boolean to flag if the objects associated with the fit should be
    /// shown.
    bool fShowFitsObjects;
    
    /// The new camera center.
    TVector3 fCameraCenter;

    /// A weight used to calculate the camera center.
    double fCameraWeight;
};
#endif
