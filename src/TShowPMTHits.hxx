#ifndef TShowPMTHits_hxx_seen
#define TShowPMTHits_hxx_seen

#include <THitSelection.hxx>
#include <HEPUnits.hxx>

namespace CP {
    class TShowPMTHits;
};

class TEveElementList;

/// Add a set of hits from a hit selection to the provided elements list.  If
/// there is a problem, this will return false. 
class CP::TShowPMTHits {
public:

    /// Construct an object to show the PMT hits.
    explicit TShowPMTHits();

    /// Show the PMT hits in the selection.  The element
    /// list is mutated by adding elements that will actually show the hit
    /// (nominally, this adds a box set).
    bool operator () (TEveElementList* elements, 
                      const CP::THitSelection& hits);
private:

};
#endif

