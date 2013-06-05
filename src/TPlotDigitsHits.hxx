#ifndef TPlotDigitsHits_hxx_seen
#define TPlotDigitsHits_hxx_seen

namespace CP {
    class TPlotDigitsHits;
};
class TH2F;

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

    TH2F* fXPlaneHist;
    TH2F* fVPlaneHist;
    TH2F* fUPlaneHist;
};

#endif
