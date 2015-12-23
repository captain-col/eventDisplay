#ifndef TReconShowerElement_hxx_seen
#define TReconShowerElement_hxx_seen

#include <TReconShower.hxx>

#include <TEveElement.h>

namespace CP {
    class TReconShowerElement;
};


/// A Eve Element object to represent TReconShower.
class CP::TReconShowerElement: public TEveElementList {
public:
    TReconShowerElement(CP::TReconShower& shower, bool showUncertainty);
    virtual ~TReconShowerElement();
};
#endif
