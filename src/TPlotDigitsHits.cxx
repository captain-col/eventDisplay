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
#include <TUnitsTable.hxx>

#include <TChannelInfo.hxx>
#include <TChannelCalib.hxx>
#include <TGeometryInfo.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TH2F.h>
#include <TColor.h>
#include <TPolyLine.h>
#include <TMarker.h>
#include <TBox.h>
#include <TROOT.h>
#include <TLegend.h>

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
        if (calib) return calib->GetFirstSample()/unit::microsecond;
        return 0.0;
    }

    double GetDigitLastTime(const CP::TDigit* d) {
        const CP::TPulseDigit* pulse 
            = dynamic_cast<const CP::TPulseDigit*>(d);
        if (pulse) return pulse->GetFirstSample()+pulse->GetSampleCount();
        const CP::TCalibPulseDigit* calib 
            = dynamic_cast<const CP::TCalibPulseDigit*>(d);
        if (calib) return calib->GetLastSample()/unit::microsecond;
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

    // This will be 1 for raw digits and 500ns for calibrated digits.
    double GetDigitSampleStep(const CP::TDigit* d) {
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
    switch(plane) {
    case 0: fCurrentGraphicsDelete = &fGraphicsXDelete; break;
    case 1: fCurrentGraphicsDelete = &fGraphicsVDelete; break;
    case 2: fCurrentGraphicsDelete = &fGraphicsUDelete; break;
    }
    
    for (std::vector<TObject*>::iterator g = fCurrentGraphicsDelete->begin();
         g != fCurrentGraphicsDelete->end(); ++g) {
        delete (*g);
    }
    fCurrentGraphicsDelete->clear();

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    // True if the sample values are times (not number of samples).
    bool samplesInTime = false;

    // Get the default digits to be drawn.
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
    
    // True if the sample values should be filled into the histogram (slow)
    bool showDigitSamples = true;
    if (!CP::TEventDisplay::Get().GUI().GetShowDigitSamplesButton()->IsOn()) {
        showDigitSamples = false;
    }

    if (!drift) {
        CaptLog("No drift signals for this event"); 
        showDigitSamples = false;
    }

    double digitSampleStep = 1;
    double signalStart = 1E+6;
    double signalEnd = -1E+6;
    int signalBins = 0;

    // Find the Z axis range for the histogram.  The median samples the
    // middle.  The max and min are the "biggest" distance from the median.
    std::vector<double> samples;
    double medianSample = 0.0;
    if (drift) {
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
        // Crash prevention.  It shouldn't be possible to have digits without
        // any samples, but...
        if (samples.empty()) return;
        
        std::sort(samples.begin(),samples.end());
        medianSample = samples[0.5*samples.size()];
        double maxSample = std::abs(samples[0.99*samples.size()]-medianSample);
        maxSample = std::max(
            maxSample,
            std::abs(samples[0.01*samples.size()]-medianSample));
        
        // Find the time axis range based on the times of the bins with a
        // signal.
        std::vector<double> times;
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
            digitSampleStep = GetDigitSampleStep(*d);
            double maxSignal = 0.0;
            for (std::size_t i = 0; i < GetDigitSampleCount(*d); ++i) {
                double s = GetDigitSample(*d,i);
                if (!std::isfinite(s)) continue;
                maxSignal = std::max(
                    maxSignal,
                    std::abs(GetDigitSample(*d,i)-medianSample));
            }
            if (maxSignal < 0.25*maxSample) continue;
            times.push_back(GetDigitFirstTime(*d));
            times.push_back(GetDigitLastTime(*d));
        }
        std::sort(times.begin(),times.end());
    
        signalStart = times[0.01*times.size()];
        signalEnd = times[0.99*times.size()];
        signalBins = (signalEnd-signalStart)/digitSampleStep;
    }
    else if (event->Get<CP::THitSelection>("~/hits/drift")) {
        CP::THandle<CP::THitSelection> hits
            = event->Get<CP::THitSelection>("~/hits/drift");
        for (CP::THitSelection::iterator h = hits->begin();
             h != hits->end(); ++h) {
            if (signalStart>(*h)->GetTime()) signalStart = (*h)->GetTime();
            if (signalEnd<(*h)->GetTime()) signalEnd = (*h)->GetTime();
        }
        signalBins = 1000;
        wireTimeStep = 500;
        digitSampleStep = 0.5;
        signalStart /= unit::microsecond;
        signalEnd /= unit::microsecond;
        samplesInTime = true;
    }
    
    // Find the number of wires to be shown on the plot of digits and hits.
    int wireCount = CP::TGeometryInfo::Get().GetWireCount(plane);
    std::ostringstream histTitle;
    histTitle << "Event " << event->GetContext().GetRun()
              << "." << event->GetContext().GetEvent() << ":";

    // Find the number of time bins to be shown on the plot of digits and
    // hits.  The oversampling reduces the number of samples shown so that the
    // display goes faster.
    int overSampling = 1.0;
    if (!showDigitSamples) {
        // In this case, the digits won't be shown, but we still need to have
        // enough bins to allow zooming.
        overSampling = 50.0;
    }
    else if (!CP::TEventDisplay::Get().GUI().GetShowFullDigitsButton()->IsOn()){
        overSampling = 20.0;
    }
    signalBins /= overSampling;
    
    // Connect to the histogram to show.
    TH2F* digitPlot = NULL;
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

    double maxVal = 10;
    if (drift) {
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
                wireTimeOffset=chanCalib.GetTimeConstant(
                    digit->GetChannelId(),0);
                wireTimeStep=chanCalib.GetTimeConstant(
                    digit->GetChannelId(),1);
            }
            // Plot the digits for this channel.
            if (!showDigitSamples) continue;
            double wire = CP::GeomId::Captain::GetWireNumber(id) + 0.5;
            for (std::size_t i = 0; i < GetDigitSampleCount(digit); ++i) {
                double tbin = GetDigitFirstTime(digit) 
                    + GetDigitSampleStep(digit)*i;
                double s = GetDigitSample(digit,i)-medianSample;
                if (!std::isfinite(s)) continue;
                if (samplesInTime) {
                    int bin = digitPlot->Fill(wire,tbin+1E-6,s);
                    digitPlot->SetBinError(bin,1.0);
                }
                else {
                    // There may be more than one sample per histogram bin (to
                    // make the plotting faster), so plot the maximum sample
                    // value in the histogram bin.
                    int bin = digitPlot->FindFixBin(wire,tbin+1E-6);
                    double v = digitPlot->GetBinContent(bin);
                    if (std::abs(s) > std::abs(v)) {
                        digitPlot->SetBinContent(bin,s);
                        digitPlot->SetBinError(bin,1.0);
                    }
                }
                int bin = digitPlot->FindFixBin(wire,tbin+1E-6);
                maxVal = std::max(maxVal,
                                  std::abs(digitPlot->GetBinContent(bin)));
            }
        }
    }
    
#ifdef USE_ABSOLUTE_MAXIMUM
    maxVal= std::min(maxVal,overSampling*10000.0);
#else
    double maxRMS = 0.0;
    double maxSamples = 1.0;
    for (int ix = 1; ix < digitPlot->GetNbinsX(); ++ix) {
        double channelRMS = 0.0;
        double channelAvg = 0.0;
        double channelSamples = 1.0;
        for (int iy = 1; iy < digitPlot->GetNbinsY(); ++iy) {
            double s = digitPlot->GetBinContent(ix,iy);
            channelRMS += s*s;
            channelAvg += s;
            channelSamples += 1.0;
        }
        channelRMS /= channelSamples;
        channelAvg /= channelSamples;
        channelRMS = channelRMS - channelAvg*channelAvg;
        maxRMS += std::sqrt(std::abs(channelRMS));
        maxSamples += 1.0;
    }
    maxRMS /= maxSamples;
    maxVal = std::max(5.0*maxRMS,5.0);
#endif
    
    // Get the right canvas to update.
    TCanvas* canvas = NULL;
    switch (plane) {
    case 0: canvas = (TCanvas*) gROOT->FindObject("canvasXDigits"); break;
    case 1: canvas = (TCanvas*) gROOT->FindObject("canvasVDigits"); break;
    case 2: canvas = (TCanvas*) gROOT->FindObject("canvasUDigits"); break;
    default: CaptError("Invalid canvas plane " << plane);
    }

    // The canvas is missing, so create a new one.
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
    case 0: canvasTitle << " X"; break;
    case 1: canvasTitle << " V"; break;
    case 2: canvasTitle << " U"; break;
    default: CaptError("Invalid canvas plane " << plane);
    }

    if (samplesInTime) {
        canvasTitle << " Time vs Wire";
    }
    else {
        canvasTitle << " Sample Number vs Wire";
    }
    canvas->SetTitle(canvasTitle.str().c_str());

    canvas->cd();
    
    digitPlot->SetMinimum(-maxVal);
    digitPlot->SetMaximum(maxVal+1);
    digitPlot->SetContour(100);
    digitPlot->SetStats(false);
    digitPlot->Draw("colz");

    ////////////////////////////////////////////////////////////
    // Now plot the PMT hit times on the histogram.
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
            double dTime = hTime/(wireTimeStep/digitSampleStep);
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
            fCurrentGraphicsDelete->push_back(pline);
        }
    }

    DrawTPCHits(plane, (wireTimeStep/digitSampleStep));

    gPad->Update();
}

void CP::TPlotDigitsHits::DrawTPCHits(int plane, double timeUnit) {
    CP::TChannelCalib chanCalib;
    
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");

    if (hits && hits->size()>1) {
        TBox* box1 = new TBox(0.0, 0.0, 1.0, 1.0);
        box1->SetFillColor(kGreen-7);
        fCurrentGraphicsDelete->push_back(box1);
        TBox* box2 = new TBox(0.0, 0.0, 1.0, 1.0);
        box2->SetFillColor(kGreen+2);
        fCurrentGraphicsDelete->push_back(box2);
        TBox* box3 = new TBox(0.0, 0.0, 1.0, 1.0);
        box3->SetFillColor(kCyan-7);
        fCurrentGraphicsDelete->push_back(box3);
        TBox* box4 = new TBox(0.0, 0.0, 1.0, 1.0);
        box4->SetFillColor(kCyan+1);
        fCurrentGraphicsDelete->push_back(box4);
        TBox* box5 = new TBox(0.0, 0.0, 1.0, 1.0);
        box5->SetFillColor(kBlue);
        fCurrentGraphicsDelete->push_back(box5);
        TBox* box6 = new TBox(0.0, 0.0, 1.0, 1.0);
        box6->SetFillColor(kBlue-7);
        fCurrentGraphicsDelete->push_back(box6);
        TBox* box7 = new TBox(0.0, 0.0, 1.0, 1.0);
        box7->SetFillColor(kRed);
        fCurrentGraphicsDelete->push_back(box7);
        TBox* box8 = new TBox(0.0, 0.0, 1.0, 1.0);
        box8->SetFillColor(kRed+1);
        fCurrentGraphicsDelete->push_back(box8);
        TBox* box9 = new TBox(0.0, 0.0, 1.0, 1.0);
        box9->SetFillColor(kMagenta);
        fCurrentGraphicsDelete->push_back(box9);
        TBox* box0 = new TBox(0.0, 0.0, 1.0, 1.0);
        box0->SetFillColor(kBlack+1);
        fCurrentGraphicsDelete->push_back(box0);

        TLegend* hitChargeLegend = new TLegend(0.8,0.8,1.0,1.0);
        std::ostringstream label;
        label << unit::AsString(0.0,"charge")
              << " - "
              << unit::AsString(4000.0,"charge");
        hitChargeLegend->AddEntry(box1,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(4000.0,"charge")
              << " - "
              << unit::AsString(6000.0,"charge");
        hitChargeLegend->AddEntry(box2,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(6000.0,"charge")
              << " - "
              << unit::AsString(8000.0,"charge");
        hitChargeLegend->AddEntry(box3,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(8000.0,"charge")
              << " - "
              << unit::AsString(10000.0,"charge");
        hitChargeLegend->AddEntry(box4,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(10000.0,"charge")
              << " - "
              << unit::AsString(13000.0,"charge");
        hitChargeLegend->AddEntry(box5,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(13000.0,"charge")
              << " - "
              << unit::AsString(16000.0,"charge");
        hitChargeLegend->AddEntry(box6,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(16000.0,"charge")
              << " - "
              << unit::AsString(20000.0,"charge");
        hitChargeLegend->AddEntry(box7,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(20000.0,"charge")
              << " - "
              << unit::AsString(25000.0,"charge");
        hitChargeLegend->AddEntry(box8,label.str().c_str(),"f");
        label.str("");;
        
        label << unit::AsString(25000.0,"charge")
              << " - "
              << unit::AsString(32000.0,"charge");
        hitChargeLegend->AddEntry(box9,label.str().c_str(),"f");
        label.str("");;
        
        fCurrentGraphicsDelete->push_back(hitChargeLegend);
        hitChargeLegend->Draw();

        for (CP::THitSelection::iterator h = hits->begin();
             h != hits->end(); ++h) {
            TGeometryId id = (*h)->GetGeomId();
            TChannelId cid = (*h)->GetChannelId();
            if (CP::GeomId::Captain::GetWirePlane(id) != plane) continue;
            double wireTimeOffset = chanCalib.GetTimeConstant(cid,0);
            // The wire number (offset for the middle of the bin).
            double wire = CP::GeomId::Captain::GetWireNumber(id) + 0.5;
            // The hit charge
            double charge = (*h)->GetCharge();
            // The hit time.
            double hTime = (*h)->GetTime();
            hTime = hTime + gPad->GetUymin()*timeUnit - wireTimeOffset;
            // The digitized hit time.
            double dTime = hTime/timeUnit;
            // The digitized hit start time.
            double dStartTime = ((*h)->GetTimeStart()-(*h)->GetTime());
            dStartTime /= timeUnit;
            dStartTime += dTime;
            // The digitized hit start time.
            double dStopTime = ((*h)->GetTimeStop()-(*h)->GetTime());
            dStopTime /= timeUnit;
            dStopTime += dTime;
            // The hit RMS.
            double rms = (*h)->GetTimeRMS();
            // The digitized RMS
            double dRMS = -1;
            
            int color = kRed;
            if (charge < 4000.0) {
                color = box1->GetFillColor();
            }
            else if (charge < 6000.0) {
                color = box2->GetFillColor();
            }
            else if (charge < 8000.0) {
                color = box3->GetFillColor();
            }
            else if (charge < 10000.0) {
                color = box4->GetFillColor();
            }
            else if (charge < 13000.0) {
                color = box5->GetFillColor();
            }
            else if (charge < 16000.0) {
                color = box6->GetFillColor();
            }
            else if (charge < 20000.0) {
                color = box7->GetFillColor();
            }
            else if (charge < 25000.0) {
                color = box8->GetFillColor();
            }
            else if (charge < 32000.0) {
                color = box9->GetFillColor();
            }
            else {
                color = box0->GetFillColor();
            }
            dRMS = rms/timeUnit;

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
                fCurrentGraphicsDelete->push_back(pline);
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
                fCurrentGraphicsDelete->push_back(pline);
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
                fCurrentGraphicsDelete->push_back(pline);
            }
        }
    }

}
