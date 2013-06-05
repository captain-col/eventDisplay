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
    typedef std::vector<std::string> FitList;
    TFindResultsHandler();
    ~TFindResultsHandler();
    
    /// Draw fit information into the current scene.
    virtual void Apply();

    /// Add a fit to the list of fits to be displayed.
    void AddFit(std::string);

    /// Clear the list of fits to be displayed.
    void ClearFits();

};

#endif
