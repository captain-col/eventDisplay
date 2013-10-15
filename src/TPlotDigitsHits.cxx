#include "TPlotDigitsHits.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <HEPUnits.hxx>
#include <TCaptLog.hxx>
#include <CaptGeomId.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <TPulseDigit.hxx>
#include <TCalibPulseDigit.hxx>
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

namespace {
    double GetDigitFirstTime(const CP::TDigit* d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(d);
        if (pulse) return pulse->GetFirstSample();
        const CP::TCalibPulseDigit* calib 
            = dynamic_cast<const CP::TCalibPulseDigit*>(d);
        if (calib) return calib->GetFirstSample()/1000;
        return 0.0;
    }

    double GetDigitLastTime(const CP::TDigit* d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(d);
        if (pulse) return pulse->GetFirstSample()+pulse->GetSampleCount();
        const CP::TCalibPulseDigit* calib 
            = dynamic_cast<const CP::TCalibPulseDigit*>(d);
        if (calib) return calib->GetLastSample()/1000;
        return 0.0;
    }

    std::size_t GetDigitSampleCount(const CP::TDigit* d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(d);
        if (pulse) return pulse->GetSampleCount();
        const CP::TCalibPulseDigit* calib 
            = dynamic_cast<const CP::TCalibPulseDigit*>(d);
        if (calib) return calib->GetSampleCount();
        return 0;
    }

    double GetDigitSampleTime(const CP::TDigit* d) {
        double diff = GetDigitLastTime(d) - GetDigitFirstTime(d);
        return diff/GetDigitSampleCount(d);
    }

    double GetDigitSample(const CP::TDigit* d, int i) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(d);
        if (pulse) return pulse->GetSample(i);
        const CP::TCalibPulseDigit* calib 
            = dynamic_cast<const CP::TCalibPulseDigit*>(d);
        if (calib) return calib->GetSample(i);
        return 0;
    }


};

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

    // True if the sample values are times (not number of samples).
    bool samplesInTime = false;
    CP::THandle<CP::TDigitContainer> drift
        = event->Get<CP::TDigitContainer>("~/digits/drift");
    // Check if the user wanted to see deconvolved digits, or if normal digits
    // were not found.
    if (!CP::TEventDisplay::Get().GUI().GetShowRawDigitsButton()->IsOn()
        or !drift) {
        CP::THandle<CP::TDigitContainer> tmp
            = event->Get<CP::TDigitContainer>("~/digits/drift-deconv");
        // If deconvolved digits are found, then use them.
        if (tmp) {
            samplesInTime = true;
            drift = tmp;
        }
    }
    
    if (!pmt) {
        CaptLog("No PMT signals for this event");
        return;
    }
    
    if (!drift) {
        CaptLog("No drift signals for this event"); 
        return;
    }

    std::vector<double> samples;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        for (std::size_t i = 0; i < GetDigitSampleCount(*d); ++i) {
            double s = GetDigitSample(*d,i);
            if (!std::isfinite(s)) continue;
            samples.push_back(GetDigitSample(*d,i));
        }
    }
    std::sort(samples.begin(),samples.end());
    double medianSample = samples[0.5*samples.size()];
    double maxSample = std::abs(samples[0.99*samples.size()]-medianSample);
    maxSample = std::max(maxSample,
                         std::abs(samples[0.01*samples.size()]-medianSample));

    std::cout << "Max Sample " << maxSample << std::endl;

    std::vector<double> times;
    double digitSampleTime = 1;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        digitSampleTime = GetDigitSampleTime(*d);
        double maxSignal = 0.0;
        for (std::size_t i = 0; i < GetDigitSampleCount(*d); ++i) {
            double s = GetDigitSample(*d,i);
            if (!std::isfinite(s)) continue;
            maxSignal = std::max(maxSignal,
                                 std::abs(GetDigitSample(*d,i)-medianSample));
        }
        if (maxSignal < 0.25*maxSample) continue;
        times.push_back(GetDigitFirstTime(*d));
        times.push_back(GetDigitLastTime(*d));
    }
    std::sort(times.begin(),times.end());
    
    double signalStart = times[0.01*times.size()];
    double signalEnd = times[0.99*times.size()];
    int signalBins = (signalEnd-signalStart)/digitSampleTime;

    TH2F* digitPlot = NULL;

    switch (projection) {
    case 0:
        if (fXPlaneHist) delete fXPlaneHist;
        fXPlaneHist = digitPlot
            = new TH2F("xPlane", "Charge on the X wires",
                       660, 0, 660,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) digitPlot->SetYTitle("Sample Time (us)");
        else digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("X Wire");
        break;
    case 1:
        if (fVPlaneHist) delete fVPlaneHist;
        fVPlaneHist = digitPlot
            = new TH2F("vPlane", "Charge on the V wires",
                       660, 0, 660,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) digitPlot->SetYTitle("Sample Time (us)");
        else digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("V Wire");
        break;
        
    case 2:
        if (fUPlaneHist) delete fUPlaneHist;
        fUPlaneHist = digitPlot
            = new TH2F("uPlane", "Charge on the U wires",
                       660, 0, 660,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) digitPlot->SetYTitle("Sample Time (us)");
        else digitPlot->SetYTitle("Sample Number");
        digitPlot->SetXTitle("U Wire");
        break;
    }    

    double maxVal = 0;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        const CP::TDigit* digit 
            = dynamic_cast<const CP::TDigit*>(*d);
        if (!digit) continue;
        double wire = -1;
        // Figure out if this is in the right projection, and get the wire
        // number.
        if (digit->GetChannelId().IsMCChannel()) {
            CP::TMCChannelId mc(digit->GetChannelId());
            wire = mc.GetNumber()+0.5;
            if (mc.GetSequence() != (unsigned) projection) continue;
        }
        for (std::size_t i = 0; i < GetDigitSampleCount(digit); ++i) {
            double tbin = GetDigitFirstTime(digit) 
                + GetDigitSampleTime(digit)*i;
            double s = GetDigitSample(digit,i)-medianSample;
            if (!std::isfinite(s)) continue;
            digitPlot->Fill(wire,tbin+1E-6,s);
            maxVal = std::max(maxVal,std::abs(s));
        }
    }
    maxVal= std::min(maxVal,10000.0);

    CaptLog("Maximum Value " << maxVal);

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
        double dTime = (time+fDigitOffset)/(fDigitStep/digitSampleTime);
        // The hit RMS.
        double rms = (*h)->GetTimeRMS();
        // The digitized RMS
        double dRMS = -1;

        if (rms<10*unit::microsecond) dRMS = rms/(fDigitStep/digitSampleTime);

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
