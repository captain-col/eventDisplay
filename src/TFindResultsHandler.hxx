#ifndef TFindResultsHandler_hxx_seen
#define TFindResultsHandler_hxx_seen

#include "TVEventChangeHandler.hxx"

#include <string>

namespace CP {
    class TFindResultsHandler;
};

class TEveElementList;

/// Look through the TAlgorithmResults saved in an event, and add them to the
/// GUI so they can be selected.
class CP::TFindResultsHandler: public TVEventChangeHandler {
public:
    TFindResultsHandler();
    ~TFindResultsHandler();
    
    /// Draw fit information into the current scene.
    virtual void Apply();

};

#endif
