#ifndef TReconTrackElement_hxx_seen
#define TReconTrackElement_hxx_seen

#include <TReconTrack.hxx>

#include <TEveElement.h>

namespace CP {
    class TReconTrackElement;
};


/// A Eve Element object to represent TReconTrack.
class CP::TReconTrackElement: public TEveElementList {
public:
    TReconTrackElement(CP::TReconTrack& track,
                       bool showUncertainty,
                       bool showDirection);
    virtual ~TReconTrackElement();
};
#endif
