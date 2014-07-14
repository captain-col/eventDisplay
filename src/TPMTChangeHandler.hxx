#ifndef TPMTChangeHandler_hxx_seen
#define TPMTChangeHandler_hxx_seen

#include "TVEventChangeHandler.hxx"

namespace CP {
    class TPMTChangeHandler;
};

class TEveElementList;
class TEveRGBAPalette;

/// Handle drawing the TAlgorithmResults saved in the event.
class CP::TPMTChangeHandler: public TVEventChangeHandler {
public:

    TPMTChangeHandler();
    ~TPMTChangeHandler();
    
    /// Draw fit information into the current scene.
    virtual void Apply();

private:

    /// The hits to draw in the event.
    TEveElementList* fPMTList;

    /// The palette to draw with.
    TEveRGBAPalette* fPalette;

    /// A boolean to flag if hits should be drawn.
    bool fShowPMTsHits;
};
#endif
