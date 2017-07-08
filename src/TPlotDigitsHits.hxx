#ifndef TPlotDigitsHits_hxx_seen
#define TPlotDigitsHits_hxx_seen
#include <vector>

namespace CP {
    class TPlotDigitsHits;
};
class TH2F;
class TObject;

/// Plot the digits and 2D hits on a canvas for the current event.  This can
/// be connected to buttons in the event display GUI.
class CP::TPlotDigitsHits {
public:
    /// Create the object to do the plotting. 
    explicit TPlotDigitsHits();
    ~TPlotDigitsHits();

    /// Draw the digits.  The "proj" chooses which projection to draw (0 is x,
    /// 1 is v, 2 is u, and 3 is all).  Each projection will be drawn in a
    /// separate canvas.
    void DrawDigits(int proj);

private:

    void DrawTPCHits(int plane, double timeUnit);
    
    /// The time digitization step.
    double fDigitStep;

    /// The time digitization offset.
    double fDigitOffset;

    /// Histograms for each projection.
    TH2F* fXPlaneHist;
    TH2F* fVPlaneHist;
    TH2F* fUPlaneHist;

    /// A vectors of objects that needs to be eventually deleted from the U,
    /// V, and X canvas (respectively).
    std::vector<TObject*> fGraphicsUDelete;
    std::vector<TObject*> fGraphicsVDelete;
    std::vector<TObject*> fGraphicsXDelete;

    /// The objects that will need to be deleted from the current working
    /// canvas (a pointer to one of fGraphics[UVX]Delete;
    std::vector<TObject*> *fCurrentGraphicsDelete;
};

#endif
