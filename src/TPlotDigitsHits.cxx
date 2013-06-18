#include "TPlotDigitsHits.hxx"

#include <HEPUnits.hxx>
#include <TCaptLog.hxx>
#include <CaptGeomId.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <TPulseDigit.hxx>
#include <TMCChannelId.hxx>
#include <TRuntimeParameters.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TH2F.h>
#include <TColor.h>
#include <TPolyLine.h>
#include <TMarker.h>

#include <cmath>
#include <algorithm>

CP::TPlotDigitsHits::TPlotDigitsHits()
    : fXPlaneHist(NULL), fVPlaneHist(NULL), fUPlaneHist(NULL) {
    Double_t r[]    = {0.0, 1., 0.};
    Double_t g[]    = {0.2, 1., 0.};
    Double_t b[]    = {0.4, 1., 0.};
    Double_t stop[] = {0., 0.5, 1.};
    TColor::CreateGradientColorTable(3, stop, r, g, b, 100);

    fDigitStep 
        = CP::TRuntimeParameters::Get().GetParameterD(
            "eventDisplay.digitization.step");

    fDigitOffset 
        = CP::TRuntimeParameters::Get().GetParameterD(
            "eventDisplay.digitization.offset");

}

CP::TPlotDigitsHits::~TPlotDigitsHits() {}

void CP::TPlotDigitsHits::DrawDigits(int projection) {

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    CP::THandle<CP::TDigitContainer> pmt
        = event->Get<CP::TDigitContainer>("~/digits/pmt");
    CP::THandle<CP::TDigitContainer> drift
        = event->Get<CP::TDigitContainer>("~/digits/drift");
    
    if (!pmt) {
        CaptLog("No PMT signals for this event");
        return;
    }
    
    if (!drift) {
        CaptLog("No drift signals for this event"); 
        return;
    }
    
    std::vector<int> times;
    std::vector<int> samples;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(*d);
        if (!pulse) continue;
        times.push_back(pulse->GetFirstSample());
        times.push_back(pulse->GetFirstSample()+pulse->GetSampleCount());
        for (std::size_t i = 0; i < pulse->GetSampleCount(); ++i) {
            samples.push_back(pulse->GetSample(i));
        }
    }
    std::sort(times.begin(),times.end());
    std::sort(samples.begin(),samples.end());
    
    int signalStart = times[0.05*times.size()];
    int signalEnd = times[0.95*times.size()];
    int signalSpread = signalEnd-signalStart;
    int signalBuffer = 0.05*signalSpread;
    int medianSample = samples[0.5*samples.size()];

    TH2F* digitPlot = NULL;

    switch (projection) {
    case 0:
        if (fXPlaneHist) delete fXPlaneHist;
        fXPlaneHist = digitPlot
            = new TH2F("xPlane", "Charge on the X wires",
                       660, 0, 660,
                       signalSpread+2*signalBuffer,
                       signalStart-signalBuffer,
                       signalEnd+signalBuffer);
        digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("X Wire");
        break;
    case 1:
        if (fVPlaneHist) delete fVPlaneHist;
        fVPlaneHist = digitPlot
            = new TH2F("vPlane", "Charge on the V wires",
                       660, 0, 660,
                       signalSpread+2*signalBuffer,
                       signalStart-signalBuffer,
                       signalEnd+signalBuffer);
        digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("V Wire");
        break;
        
    case 2:
        if (fUPlaneHist) delete fUPlaneHist;
        fUPlaneHist = digitPlot
            = new TH2F("uPlane", "Charge on the U wires",
                             660, 0, 660,
                       signalSpread+2*signalBuffer,
                       signalStart-signalBuffer,
                       signalEnd+signalBuffer);
        digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("U Wire");
        break;
    }    

    int maxVal = 0;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(*d);
        if (!pulse) continue;
        double wire = -1;
        // Figure out if this is in the right projection, and get the wire
        // number.
        if (pulse->GetChannelId().IsMCChannel()) {
            CP::TMCChannelId mc(pulse->GetChannelId());
            wire = mc.GetNumber()+0.5;
            if (mc.GetSequence() != (unsigned) projection) continue;
        }
        for (std::size_t i = 0; i < pulse->GetSampleCount(); ++i) {
            int tbin = pulse->GetFirstSample() + i;
            int s = pulse->GetSample(i)-medianSample;
            digitPlot->Fill(wire,tbin+0.5,pulse->GetSample(i)-medianSample);
            maxVal = std::max(maxVal,std::abs(s));
        }
    }

    digitPlot->SetMinimum(-maxVal);
    digitPlot->SetMaximum(maxVal+1);
    digitPlot->SetContour(100);
    digitPlot->SetStats(false);
    digitPlot->Draw("colz");
    
    gPad->Update();

    ////////////////////////////////////////////////////////////
    // Now plot the 2D hits on the histogram.
    ////////////////////////////////////////////////////////////
    
    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");

    if (!hits) return;

    for (std::vector<TObject*>::iterator g = fGraphicsDelete.begin();
         g != fGraphicsDelete.end(); ++g) {
        delete (*g);
    }
    fGraphicsDelete.clear();

    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        TGeometryId id = (*h)->GetGeomId();
        if (CP::GeomId::Captain::GetWirePlane(id) != projection) continue;
        // The wire number (offset for the middle of the bin).
        double wire = CP::GeomId::Captain::GetWireNumber(id) + 0.5;
        // The hit time.
        double time = (*h)->GetTime();
        // The digitized hit time.
        double dTime = (time+fDigitOffset)/fDigitStep;
        // The hit RMS.
        double rms = (*h)->GetTimeRMS();
        // The digitized RMS
        double dRMS = -1;
        if (rms<10*unit::microsecond) dRMS = rms/fDigitStep;
        // The hit uncertainty
        double sig = (*h)->GetTimeUncertainty();
        // The digitized hit uncertainty
        double dSig = sig/fDigitStep;

        TMarker* vtx = new TMarker(wire, dTime, 6);
        vtx->SetMarkerSize(1);
        vtx->SetMarkerColor(kRed);
        vtx->Draw();
        fGraphicsDelete.push_back(vtx);

        if (dRMS > 0) {
            int n=0;
            double px[10];
            double py[10];
            px[n] = wire;
            py[n++] = dTime-dRMS;
            px[n] = wire;
            py[n++] = dTime+dRMS;
            TPolyLine* pline = new TPolyLine(n,px,py);
            pline->SetLineWidth(1);
            pline->SetLineColor(kRed);
            pline->Draw();
            fGraphicsDelete.push_back(pline);
        }
    }
    
    gPad->Update();
    
}
