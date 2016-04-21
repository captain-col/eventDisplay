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

#include <TChannelInfo.hxx>
#include <TChannelCalib.hxx>
#include <TGeometryInfo.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TH2F.h>
#include <TColor.h>
#include <TPolyLine.h>
#include <TMarker.h>
#include <TROOT.h>

#include <cmath>
#include <algorithm>
#include <sstream>

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

}

CP::TPlotDigitsHits::~TPlotDigitsHits() {}

void CP::TPlotDigitsHits::DrawDigits(int plane) {
    CP::TChannelCalib chanCalib;
    double wireTimeStep = -1.0;
    double wireTimeOffset = 0.0;

    ///////////////////////////////////////////////////////////////////
    // Delete any objects that were plotted on the canvas.
    ///////////////////////////////////////////////////////////////////
    std::vector<TObject*> *graphicsDelete = NULL;
    switch(plane) {
    case 0: graphicsDelete = &fGraphicsXDelete; break;
    case 1: graphicsDelete = &fGraphicsVDelete; break;
    case 2: graphicsDelete = &fGraphicsUDelete; break;
    }
    
    for (std::vector<TObject*>::iterator g = graphicsDelete->begin();
         g != graphicsDelete->end(); ++g) {
        delete (*g);
    }
    graphicsDelete->clear();

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

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
    
    if (!drift) {
        CaptLog("No drift signals for this event"); 
        return;
    }

    // Find the Z axis range for the histogram.  The median sample s the
    // middle.  The max is the "biggest" distance from the medial.
    std::vector<double> samples;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        // Figure out if this is in the right plane, and get the wire
        // number.
        const CP::TDigit* digit 
            = dynamic_cast<const CP::TDigit*>(*d);
        if (!digit) continue;
        CP::TGeometryId id 
            = CP::TChannelInfo::Get().GetGeometry(digit->GetChannelId());
        if (CP::GeomId::Captain::GetWirePlane(id) != plane) continue;
        // Save the sample to find the median.
        for (std::size_t i = 0; i < GetDigitSampleCount(*d); ++i) {
            double s = GetDigitSample(*d,i);
            if (!std::isfinite(s)) continue;
            samples.push_back(s);
        }
    }

    if (samples.empty()) return;
    
    std::sort(samples.begin(),samples.end());
    double medianSample = samples[0.5*samples.size()];

    double maxSample = std::abs(samples[0.99*samples.size()]-medianSample);
    maxSample = std::max(maxSample,
                         std::abs(samples[0.01*samples.size()]-medianSample));

    // Find the time axis range based on the times of the bins with a signal.
    std::vector<double> times;
    double digitSampleTime = 1;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        // Figure out if this is in the right plane, and get the wire
        // number.
        const CP::TDigit* digit 
            = dynamic_cast<const CP::TDigit*>(*d);
        if (!digit) continue;
        CP::TGeometryId id 
            = CP::TChannelInfo::Get().GetGeometry(digit->GetChannelId());
        if (CP::GeomId::Captain::GetWirePlane(id) != plane) continue;
        // Find the time range.
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

    // Connect to the histogram to show.
    TH2F* digitPlot = NULL;
    int wireCount = CP::TGeometryInfo::Get().GetWireCount(plane);
    std::ostringstream histTitle;
    histTitle << "Event " << event->GetContext().GetRun()
              << "." << event->GetContext().GetEvent() << ":";

    int overSampling = 1.0;
    if (!CP::TEventDisplay::Get().GUI().GetShowFullDigitsButton()->IsOn()) {
        overSampling = 20.0;
    }
    signalBins /= overSampling;
    
    switch (plane) {
    case 0:
        if (fXPlaneHist) delete fXPlaneHist;
        fXPlaneHist = digitPlot
            = new TH2F("xPlane", "Charge on the X wires",
                       wireCount, 0, wireCount,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) {
            digitPlot->SetYTitle("Sample Time (us)");
            histTitle << " Calibrated charge on X wires";
        }
        else {
            digitPlot->SetYTitle("Sample Number");
            histTitle << " Uncalibrated ADC value X wires";
        }
        digitPlot->SetXTitle("X Wire");
        digitPlot->SetTitle(histTitle.str().c_str());
        break;

    case 1:
        if (fVPlaneHist) delete fVPlaneHist;
        fVPlaneHist = digitPlot
            = new TH2F("vPlane", "Charge on the V wires",
                       wireCount, 0, wireCount,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) {
            digitPlot->SetYTitle("Sample Time (us)");
            histTitle << " Calibrated charge on V wires";
        }
        else {
            digitPlot->SetYTitle("Sample Number");
            histTitle << " Uncalibrated ADC value V wires";
        }
        digitPlot->SetXTitle("V Wire");
        digitPlot->SetTitle(histTitle.str().c_str());
        break;
        
    case 2:
        if (fUPlaneHist) delete fUPlaneHist;
        fUPlaneHist = digitPlot
            = new TH2F("uPlane", "Charge on the U wires",
                       wireCount, 0, wireCount,
                       signalBins, signalStart, signalEnd);
        if (samplesInTime) {
            digitPlot->SetYTitle("Sample Time (us)");
            histTitle << " Calibrated charge on U wires";
        }
        else {
            digitPlot->SetYTitle("Sample Number");
            histTitle << " Uncalibrated ADC value U wires";
        }
        digitPlot->SetXTitle("U Wire");
        digitPlot->SetTitle(histTitle.str().c_str());
        break;
    }    

    double maxVal = 0;
    for (CP::TDigitContainer::const_iterator d = drift->begin();
         d != drift->end(); ++d) {
        // Figure out if this is in the right plane, and get the wire
        // number.
        const CP::TDigit* digit 
            = dynamic_cast<const CP::TDigit*>(*d);
        if (!digit) continue;
        CP::TGeometryId id 
            = CP::TChannelInfo::Get().GetGeometry(digit->GetChannelId());
        if (CP::GeomId::Captain::GetWirePlane(id) != plane) continue;
        if (wireTimeStep < 0.0) {
            wireTimeOffset=chanCalib.GetTimeConstant(digit->GetChannelId(),0);
            wireTimeStep=chanCalib.GetTimeConstant(digit->GetChannelId(),1);
        }
        // Plot the digits for this channel.
        double wire = CP::GeomId::Captain::GetWireNumber(id) + 0.5;
        for (std::size_t i = 0; i < GetDigitSampleCount(digit); ++i) {
            double tbin = GetDigitFirstTime(digit) 
                + GetDigitSampleTime(digit)*i;
            double s = GetDigitSample(digit,i)-medianSample;
            if (!std::isfinite(s)) continue;
            if (samplesInTime) {
                int bin = digitPlot->Fill(wire,tbin+1E-6,s);
                digitPlot->SetBinError(bin,1.0);
            }
            else {
                int bin = digitPlot->FindFixBin(wire,tbin+1E-6);
                double v = digitPlot->GetBinContent(bin);
                if (std::abs(s) > std::abs(v)) {
                    digitPlot->SetBinContent(bin,s);
                    digitPlot->SetBinError(bin,1.0);
                }
            }
            int bin = digitPlot->FindFixBin(wire,tbin+1E-6);
            maxVal = std::max(maxVal,std::abs(digitPlot->GetBinContent(bin)));
        }
    }
    
    maxVal= std::min(maxVal,overSampling*10000.0);

    TCanvas* canvas = NULL;
    switch (plane) {
    case 0: canvas = (TCanvas*) gROOT->FindObject("canvasXDigits"); break;
    case 1: canvas = (TCanvas*) gROOT->FindObject("canvasVDigits"); break;
    case 2: canvas = (TCanvas*) gROOT->FindObject("canvasUDigits"); break;
    default: CaptError("Invalid canvas plane " << plane);
    }

    if (!canvas) {
        switch (plane) {
        case 0: canvas=new TCanvas("canvasXDigits","X Digits",500,300); break;
        case 1: canvas=new TCanvas("canvasVDigits","V Digits",500,300); break;
        case 2: canvas=new TCanvas("canvasUDigits","U Digits",500,300); break;
        default: CaptError("Invalid canvas plane " << plane);
        }
    }

    // Update the canvas title.
    std::ostringstream canvasTitle;
    canvasTitle << "Event " << event->GetContext().GetRun()
                << "." << event->GetContext().GetEvent() << ":";

    switch (plane) {
    case 0: canvasTitle << " X Digits"; break;
    case 1: canvasTitle << " V Digits"; break;
    case 2: canvasTitle << " U Digits"; break;
    default: CaptError("Invalid canvas plane " << plane);
    }
    canvas->SetTitle(canvasTitle.str().c_str());

    canvas->cd();
    
    digitPlot->SetMinimum(-maxVal);
    digitPlot->SetMaximum(maxVal+1);
    digitPlot->SetContour(100);
    digitPlot->SetStats(false);
    digitPlot->Draw("colz");

    gPad->Update();

    ////////////////////////////////////////////////////////////
    // Now plot the PMT hits on the histogram.
    ////////////////////////////////////////////////////////////
    
    CP::THandle<CP::THitSelection> pmts
        = event->Get<CP::THitSelection>("~/hits/pmt");
    if (pmts) {
        for (CP::THitSelection::iterator h = pmts->begin();
             h != pmts->end(); ++h) {
            TGeometryId id = (*h)->GetGeomId();
            TChannelId cid = (*h)->GetChannelId();
            double hTime = (*h)->GetTime();
            if (!samplesInTime) hTime -= wireTimeOffset;
            double dTime = hTime/(wireTimeStep/digitSampleTime);
            int n=0;
            double px[10];
            double py[10];
            px[n] = 1;
            py[n++] = dTime;
            px[n] = wireCount-1;
            py[n++] = dTime;
            TPolyLine* pline = new TPolyLine(n,px,py);
            pline->SetLineWidth(1);
            pline->SetLineColor(kGreen);
            pline->Draw();
            graphicsDelete->push_back(pline);
        }
    }
    
    ////////////////////////////////////////////////////////////
    // Now plot the 2D hits on the histogram.
    ////////////////////////////////////////////////////////////
    
    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");

    if (hits) {
        for (CP::THitSelection::iterator h = hits->begin();
             h != hits->end(); ++h) {
            TGeometryId id = (*h)->GetGeomId();
            TChannelId cid = (*h)->GetChannelId();
            if (CP::GeomId::Captain::GetWirePlane(id) != plane) continue;
            // The wire number (offset for the middle of the bin).
            double wire = CP::GeomId::Captain::GetWireNumber(id) + 0.5;
            // The hit charge
            double charge = (*h)->GetCharge();
            // The hit time.
            double hTime = (*h)->GetTime();
            if (!samplesInTime) hTime -= wireTimeOffset;
            // The digitized hit time.
            double dTime = hTime/(wireTimeStep/digitSampleTime);
            // The digitized hit start time.
            double dStartTime = ((*h)->GetTimeStart()-(*h)->GetTime());
            dStartTime /= (wireTimeStep/digitSampleTime);
            dStartTime += dTime;
            // The digitized hit start time.
            double dStopTime = ((*h)->GetTimeStop()-(*h)->GetTime());
            dStopTime /= (wireTimeStep/digitSampleTime);
            dStopTime += dTime;
            // The hit RMS.
            double rms = (*h)->GetTimeRMS();
            // The digitized RMS
            double dRMS = -1;
            
            int color = kRed;
            if (charge < 4000.0) {
                color = kGreen-7;
            }
            else if (charge < 6000.0) {
                color = kGreen+2;
            }
            else if (charge < 8000.0) {
                color = kCyan-7;
            }
            else if (charge < 10000.0) {
                color = kCyan+1;
            }
            else if (charge < 13000.0) {
                color = kBlue;
            }
            else if (charge < 16000.0) {
                color = kBlue-7;
            }
            else if (charge < 20000.0) {
                color = kRed;
            }
            else if (charge < 25000.0) {
                color = kRed+1;
            }
            else if (charge < 32000.0) {
                color = kMagenta;
            }
            else {
                color = kBlack+1;
            }
            
            dRMS = rms/(wireTimeStep/digitSampleTime);
            
            {
                int n=0;
                double px[10];
                double py[10];
                px[n] = wire-0.5;
                py[n++] = dTime;
                px[n] = wire+0.5;
                py[n++] = dTime;
                TPolyLine* pline = new TPolyLine(n,px,py);
                pline->SetLineWidth(1);
                pline->SetLineColor(color);
                pline->Draw();
                graphicsDelete->push_back(pline);
            }
            
            if (dStartTime < dTime && dTime < dStopTime) {
                int n=0;
                double px[10];
                double py[10];
                px[n] = wire;
                py[n++] = dStartTime;
                px[n] = wire;
                py[n++] = dStopTime;
                TPolyLine* pline = new TPolyLine(n,px,py);
                pline->SetLineWidth(1);
                pline->SetLineStyle(3);
                pline->SetLineColor(color);
                pline->Draw();
                graphicsDelete->push_back(pline);
            }

            if (dRMS > 0) {
                int n=0;
                double px[10];
                double py[10];
                px[n] = wire;
                py[n++] = dTime-dRMS;
                px[n] = wire;
                py[n++] = dTime+dRMS;
                TPolyLine* pline = new TPolyLine(n,px,py);
                pline->SetLineWidth(3);
                pline->SetLineColor(color);
                pline->Draw();
                graphicsDelete->push_back(pline);
            }
        }
    }

    gPad->Update();
    
}
