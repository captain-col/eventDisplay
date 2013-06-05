#ifndef TFitChangeHandler_hxx_seen
#define TFitChangeHandler_hxx_seen

#include "TVEventChangeHandler.hxx"

#include <string>

namespace CP {
    class TFitChangeHandler;
};

class TEveElementList;

/// Handle drawing the TAlgorithmResults saved in the event.
class CP::TFitChangeHandler: public TVEventChangeHandler {
public:
    typedef std::vector<std::string> FitList;
    TFitChangeHandler();
    ~TFitChangeHandler();
    
    /// Draw fit information into the current scene.
    virtual void Apply();

    /// Add a fit to the list of fits to be displayed.
    void AddFit(std::string);

    /// Clear the list of fits to be displayed.
    void ClearFits();

private:

    /// A list of TAlgorithmResult names to be displayed.  If the result isn't
    /// available, then it is skipped.
    FitList fFitList;

    /// The hits to draw in the event.
    TEveElementList* fHitList;

};

#endif
