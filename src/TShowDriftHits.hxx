#ifndef TShowDriftHits_hxx_seen
#define TShowDriftHits_hxx_seen

#include <THitSelection.hxx>

namespace CP {
    class TShowDriftHits;
};

class TEveElementList;

/// Add a set of hits from a hit selection to the provided elements list.  If
/// there is a problem, this will return false. 
class CP::TShowDriftHits {
public:

    /// Construct an object to show the hits using a drift velocity.  The
    /// drift is assumed to be on the Z axis.
    explicit TShowDriftHits(double velocity);

    /// Show the hits in the selection using a particular t0.  The element
    /// list is mutated by adding elements that will actually show the hit
    /// (nominally, this adds a box set.
    bool operator () (TEveElementList* elements, 
                      const CP::THitSelection& hits,
                      double t0);
private:

    /// The drift velocity used to plot the hits.
    double fDriftVelocity;
};
#endif

