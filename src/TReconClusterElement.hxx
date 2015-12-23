#ifndef TReconClusterElement_hxx_seen
#define TReconClusterElement_hxx_seen

#include <TReconCluster.hxx>

#include <TEveElement.h>

namespace CP {
    class TReconClusterElement;
};


/// A Eve Element object to represent TReconCluster.
class CP::TReconClusterElement: public TEveElementList {
public:
    TReconClusterElement(CP::TReconCluster& cluster, bool showUncertainty);
    virtual ~TReconClusterElement();
};
#endif
