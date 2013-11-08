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
    void ShowReconCluster(const CP::THandle<CP::TReconCluster> obj);
    
    /// A method to draw a TReconShower.
    void ShowReconShower(const CP::THandle<CP::TReconShower> obj);

    /// A method to draw a TReconTrack.
    void ShowReconTrack(const CP::THandle<CP::TReconTrack> obj);

    /// A method to draw a TReconPID
    void ShowReconPID(const CP::THandle<CP::TReconPID> obj);

    /// A method to draw a TReconVertex
    void ShowReconVertex(const CP::THandle<CP::TReconVertex> obj);

    /// A method to draw a TReconObjectContainer
    void ShowReconObjects(const CP::THandle<CP::TReconObjectContainer> obj);

    /// The reconstruction objects to draw in the event.
    TEveElementList* fFitList;

    /// The hits to draw in the event.
    TEveElementList* fHitList;

    /// A boolean to flag if hits associated with reconstructed objects should
    /// be drawn.
    bool fShowFitsHits;

};
#endif
