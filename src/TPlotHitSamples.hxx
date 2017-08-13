#ifndef TPlotHitSamples_hxx_seen
#define TPlotHitSamples_hxx_seen
#include <vector>

namespace CP {
    class TPlotHitSamples;
};

class TObject;

/// Plot the digits and 2D hits on a canvas for the current event.  This can
/// be connected to buttons in the event display GUI.
class CP::TPlotHitSamples {
public:
    /// Create the object to do the plotting. 
    explicit TPlotHitSamples();
    ~TPlotHitSamples();

    /// Draw the charge vs time in a graph.
    void DrawHitSamples();

private:

    /// Things to delete.
    std::vector<TObject*> fGraphicsDelete;

};

#endif
