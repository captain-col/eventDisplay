#include "TPlotTimeCharge.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TEvent.hxx>
#include <TEventContext.hxx>
#include <THit.hxx>
#include <THandle.hxx>
#include <THitSelection.hxx>
#include <TEventFolder.hxx>
#include <CaptGeomId.hxx>
#include <HEPUnits.hxx>
#include <TUnitsTable.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TH1F.h>
#include <TF1.h>
#include <TVirtualFitter.h>
#include <TFitResultPtr.h>
#include <TFitResult.h>
#include <TList.h>

#include <iostream>
#include <sstream>

CP::TPlotTimeCharge::TPlotTimeCharge()
    : fXPlaneGraph(NULL), fVPlaneGraph(NULL), fUPlaneGraph(NULL),
      fGraphLegend(NULL), fElectronLifeFunction(NULL) {
}

CP::TPlotTimeCharge::~TPlotTimeCharge() {}

namespace {
    
    void PlotTimeChargeFitFCN(Int_t &npar, Double_t * /*gin*/, Double_t &f,
                              Double_t *u, Int_t /*flag*/) {
        // This implements a maximum goodness fitter.  This uses a goodness
        // fuction of exp(0.5*((x-f)/sigma(x))^2) which is just a Gaussian.
        // This has the nice feature that it behaves like chi-squared for
        // points that are close to the expected value, but ignores points
        // that are far away.  It has the downside that it can have local
        // minima so the starting point values need to be fairly close.
        Double_t cu,eu,ey,fu,fsum;
        Double_t x[1];
        Int_t bin, npfits=0;
        
        TVirtualFitter *grFitter = TVirtualFitter::GetFitter();
        TGraph *gr     = (TGraph*)grFitter->GetObjectFit();
        TF1 *f1   = (TF1*)grFitter->GetUserFunc();
        Int_t n        = gr->GetN();
        Double_t *gx   = gr->GetX();
        Double_t *gy   = gr->GetY();
        npar = f1->GetNpar();
        f = 0;
        for (bin=0;bin<n;bin++) {
            f1->InitArgs(x,u); // Inside since TF1::Derivative calls InitArgs.
            x[0] = gx[bin];
            if (!f1->IsInside(x)) continue;
            cu   = gy[bin];
            TF1::RejectPoint(kFALSE);
            fu   = f1->EvalPar(x,u);
            if (TF1::RejectedPoint()) {
                continue;
            }
            fsum = (cu-fu);
            npfits++;
            if (fsum < 0) {
                ey = gr->GetErrorYhigh(bin);
            }
            else {
                ey = gr->GetErrorYlow(bin);
            }
            if (ey < 0)  ey  = 0;
            eu = ey*ey;
            if (eu <= 0) eu = 1;
            f += 1.0 - exp(-0.5*fsum*fsum/eu);
            // Add a penalty for wanting a large constant term.
            f += u[2]*u[2]/2000.0/2000.0;
        }
        f1->SetNumberFitPoints(npfits);
    }
}

void CP::TPlotTimeCharge::FitTimeCharge() {
    TCanvas* canvas = (TCanvas*) gROOT->FindObject("canvasTimeCharge");
    if (!canvas) return;
    
    // Determine the range of the fit.  This fits the hits that are in the
    // zoomed histogram.
    double xMin = canvas->GetUxmin();
    double xMax = canvas->GetUxmax();
    double yMin = canvas->GetUymin();
    double yMax = canvas->GetUymax();

    // If the x plane hits are drawn, fit them.  Then try the V and U planes.
    TGraphErrors* graph = fXPlaneGraph;
    if (!graph) graph = fVPlaneGraph;
    if (!graph) graph = fUPlaneGraph;
    if (!graph) {
        CaptError("No hits to fit");
        return;
    }
    
    // Find the time zero for the fit. 
    double timeZero = xMin;
    double maxCharge = 0.0;
    for (int i= 0; i< graph->GetN(); ++i) {
        double x, y;
        graph->GetPoint(i,x,y);
        if (y > yMax) continue;
        if (y < maxCharge) continue;
        timeZero = x;
        maxCharge = y;
    }

    double minCharge = maxCharge;
    for (int i= 0; i< graph->GetN(); ++i) {
        double x, y;
        graph->GetPoint(i,x,y);
        if (y > minCharge) continue;
        minCharge = y;
    }
    if (minCharge < 1000) minCharge = 1000;
    
    for (int i= 0; i< graph->GetN(); ++i) {
        double x, y;
        graph->GetPoint(i,x,y);
        if (maxCharge < y) {
            timeZero = x;
            maxCharge = y;
        }
    }

    // Find the range of "normalizations to use".
    double closestPoint = 1E+10;
    int closestNorm = 10000;
    for (int i= 0; i< graph->GetN(); ++i) {
        double x, y;
        graph->GetPoint(i,x,y);
        if (x<xMin) continue;
        if (x<closestPoint) {
            closestPoint = x;
            closestNorm = y;
        }
    }

    if (fElectronLifeFunction) delete fElectronLifeFunction;
    std::ostringstream functionString;
    functionString << "[2]+[1]*exp(-(x-" << timeZero << ")/[0])";
        
    fElectronLifeFunction = new TF1("electronLifeFunction",
                                    functionString.str().c_str());
    fElectronLifeFunction->SetParName(0,"Electron Lifetime");
    fElectronLifeFunction->SetParName(1,"Normalization");
    fElectronLifeFunction->SetParName(2,"Baseline");
    fElectronLifeFunction->SetRange(xMin+(xMax-xMin)/20.0,
                                    xMax-(xMax-xMin)/20.0);

    // Limit the range of electron lifetimes to fit (5us to 6 ms)
    fElectronLifeFunction->SetParLimits(0, 5.0, 6000.0);
    // Limit the range of normalizations
    fElectronLifeFunction->SetParLimits(1,minCharge,maxCharge);

    // Set the initial parameter values.
    TVirtualFitter* fitter = TVirtualFitter::Fitter(graph);
    fitter->SetFCN(PlotTimeChargeFitFCN);
    fitter->SetObjectFit(graph);
    fitter->SetUserFunc(fElectronLifeFunction);

    const double minLife = 5.0; // microseconds;
    const double maxLife = 6000.0; //microseconds;

    double bestValue = 1E+30;
    double bestLife = 0.0;
    double bestNorm = 0.0;
    double bestBase = 0.0;
    
    double chargeStep = std::log(maxCharge/minCharge)/100.0;
    for (double logCharge = std::log(minCharge);
         logCharge < std::log(maxCharge);
         logCharge += chargeStep) {
        double minAngle = std::atan(1.0/maxLife);
        double maxAngle = std::atan(1.0/minLife);
        double angleStep = (maxAngle-minAngle)/100.0;
        for (double angle = minAngle; angle<maxAngle; angle += angleStep) {
            int npar = 2;
            double u[2];
            double value;
            u[0] = 1.0/std::tan(angle);
            u[1] = std::exp(logCharge);
            PlotTimeChargeFitFCN(npar,NULL,value,u,0);
            if (value < bestValue) {
                bestValue = value;
                bestLife = 1.0/std::tan(angle);
                bestNorm = std::exp(logCharge);
            }
        }
    }

    fElectronLifeFunction->SetParameters(bestLife, bestNorm, bestBase);
    TFitResultPtr result = graph->Fit(fElectronLifeFunction,"su","",
                                      xMin,
                                      xMax);

    // Show the fit results in the legend
    if (!fGraphLegend || !fGraphLegend->GetListOfPrimitives()) {
        gPad->Update();
        return;
    }
    std::ostringstream lifeLabel;
    lifeLabel << "Life: "
              << unit::AsString(unit::microsecond*result->Parameter(0),
                                unit::microsecond*result->Error(0), "time");
    std::ostringstream normLabel;
    normLabel << "Norm: "
              << unit::AsString(result->Parameter(1),
                                result->Error(1), "electrons")
              << " @ " << unit::AsString(unit::microsecond*timeZero,"time");
    std::ostringstream baseLabel;
    baseLabel << "Baseline: "
              << unit::AsString(result->Parameter(2),
                                result->Error(2), "electrons");
    bool foundLifeLabel = false;
    TIter entries(fGraphLegend->GetListOfPrimitives());
    while (TObject *o = entries()) {
        TLegendEntry* entry = dynamic_cast<TLegendEntry*>(o);
        std::string label(entry->GetLabel());
        if (label.find("Life:") != std::string::npos) {
            foundLifeLabel=true;
            entry->SetLabel(lifeLabel.str().c_str());
        }
        if (label.find("Norm:") != std::string::npos) {
            entry->SetLabel(normLabel.str().c_str());
        }
        if (label.find("Baseline:") != std::string::npos) {
            entry->SetLabel(baseLabel.str().c_str());
        }
    }
    if (!foundLifeLabel) {
        fGraphLegend->AddEntry((TObject*)NULL,lifeLabel.str().c_str(),"");
        fGraphLegend->AddEntry((TObject*)NULL,normLabel.str().c_str(),"");
        fGraphLegend->AddEntry((TObject*)NULL,baseLabel.str().c_str(),"");
    }
                            
    gPad->Update();
}

void CP::TPlotTimeCharge::DrawTimeCharge() {

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");
    if (!hits) {
        CaptError("No hits to draw");
        return;
    }

    bool drawXHits
        = CP::TEventDisplay::Get().GUI().GetShowXTimeChargeButton()->IsOn();
    bool drawVHits
        = CP::TEventDisplay::Get().GUI().GetShowVTimeChargeButton()->IsOn();
    bool drawUHits 
        = CP::TEventDisplay::Get().GUI().GetShowUTimeChargeButton()->IsOn();

    double minTime = 1E+22;
    double maxTime = -1E+22;
    double minCharge = 1.0;
    double maxCharge = -1E+22;
    
    const int maxPoints = 10000;
    double time[maxPoints];
    double timeRMS[maxPoints];
    double charge[maxPoints];
    double chargeUnc[maxPoints];
    int points;

    // Check to see if the hits have been plotted on time vs wire.
    TCanvas* canvasUDigits = (TCanvas*) gROOT->FindObject("canvasUDigits");
    TCanvas* canvasVDigits = (TCanvas*) gROOT->FindObject("canvasVDigits");
    TCanvas* canvasXDigits = (TCanvas*) gROOT->FindObject("canvasXDigits");
    
    // Find the time range shown on the time vs wire plot.  If the plot is in
    // sample vs wire, then this will need to be corrected to be in time.
    double minDigitTime = 1E+22;
    double maxDigitTime = -1E+22;
    if (!canvasUDigits && !canvasVDigits && !canvasXDigits) {
        minDigitTime = -1E+22;
        maxDigitTime = 1E+22;
    }
    if (canvasUDigits) {
        double s = 1.0;
        double b = 0.0;
        std::string title = canvasUDigits->GetTitle();
        if (title.find("Sample") != std::string::npos) {
            // Set *VERY* rough calibration coefficients.
            s=0.5;
            b=1600.0;
        }
        minDigitTime = std::min(s*canvasUDigits->GetUymin()-b,minDigitTime);
        maxDigitTime = std::max(s*canvasUDigits->GetUymax()-b,maxDigitTime);
    }
    if (canvasVDigits) {
        double s = 1.0;
        double b = 0.0;
        std::string title = canvasVDigits->GetTitle();
        if (title.find("Sample") != std::string::npos) {
            // Set *VERY* rough calibration coefficients.
            s=0.5;
            b=1600.0;
        }
        minDigitTime = std::min(s*canvasVDigits->GetUymin()-b,minDigitTime);
        maxDigitTime = std::max(s*canvasVDigits->GetUymax()-b,maxDigitTime);
    }
    if (canvasXDigits) {
        double s = 1.0;
        double b = 0.0;
        std::string title = canvasXDigits->GetTitle();
        if (title.find("Sample") != std::string::npos) {
            // Set *VERY* rough calibration coefficients.
            s=0.5;
            b=1600.0;
        }
        minDigitTime = std::min(s*canvasXDigits->GetUymin()-b,minDigitTime);
        maxDigitTime = std::max(s*canvasXDigits->GetUymax()-b,maxDigitTime);
    }

    // Convert to time.
    minDigitTime *= unit::microsecond;
    maxDigitTime *= unit::microsecond;

    // Fill the graph for the U hits
    points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsUWire((*h)->GetGeomId())) continue;
        if (!drawUHits) continue;
        if ((*h)->GetTime() < minDigitTime) continue;
        if ((*h)->GetTime() > maxDigitTime) continue;
        if (canvasUDigits) {
            double wire = CP::GeomId::Captain::GetWireNumber((*h)->GetGeomId());
            if (wire < canvasUDigits->GetUxmin()) continue;
            if (wire > canvasUDigits->GetUxmax()) continue;
        }
        // The histogram is labeled in microseconds.
        time[points] = (*h)->GetTime()/unit::microsecond;
        timeRMS[points] = (*h)->GetTimeRMS()/unit::microsecond;
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
        if (points>=maxPoints) break;
    }
    if (fUPlaneGraph) delete fUPlaneGraph;
    fUPlaneGraph = NULL;
    if (points > 0) {
        fUPlaneGraph
            = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);
        fUPlaneGraph->SetName("GraphUPlaneTimeCharge");
    }

    // Fill the graph for the V hits.
    points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsVWire((*h)->GetGeomId())) continue;
        if (!drawVHits) continue;
        if ((*h)->GetTime() < minDigitTime) continue;
        if ((*h)->GetTime() > maxDigitTime) continue;
        if (canvasVDigits) {
            double wire = CP::GeomId::Captain::GetWireNumber((*h)->GetGeomId());
            if (wire < canvasVDigits->GetUxmin()) continue;
            if (wire > canvasVDigits->GetUxmax()) continue;
        }
        // The histogram is labeled in microseconds.
        time[points] = (*h)->GetTime()/unit::microsecond;
        timeRMS[points] = (*h)->GetTimeRMS()/unit::microsecond;
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
        if (points>=maxPoints) break;
    }
    if (fVPlaneGraph) delete fVPlaneGraph;
    fVPlaneGraph = NULL;
    if (points > 0) {
        fVPlaneGraph
            = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);
        fVPlaneGraph->SetName("GraphVPlaneTimeCharge");
    }
    
    // Fill the graph for the X hits.
    points=0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsXWire((*h)->GetGeomId())) continue;
        if (!drawXHits) continue;
        if ((*h)->GetTime() < minDigitTime) continue;
        if ((*h)->GetTime() > maxDigitTime) continue;
        if (canvasXDigits) {
            double wire = CP::GeomId::Captain::GetWireNumber((*h)->GetGeomId());
            if (wire < canvasXDigits->GetUxmin()) continue;
            if (wire > canvasXDigits->GetUxmax()) continue;
        }
        // The histogram is labeled in microseconds.
        time[points] = (*h)->GetTime()/unit::microsecond;
        timeRMS[points] = (*h)->GetTimeRMS()/unit::microsecond;
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        chargeUnc[points] += 0.4*charge[points];
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
        if (points>=maxPoints) break;
    }
    if (fXPlaneGraph) delete fXPlaneGraph;
    fXPlaneGraph = NULL;
    if (points > 0) {
        fXPlaneGraph
            = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);
        fXPlaneGraph->SetName("GraphXPlaneTimeCharge");
    }

    TCanvas* canvas = NULL;
    canvas = (TCanvas*) gROOT->FindObject("canvasTimeCharge");
    if (!canvas) {
        canvas=new TCanvas("canvasTimeCharge","Hit Times and Charges",500,300);
    }
    canvas->SetTopMargin(0.07);
    canvas->SetRightMargin(0.04);
    canvas->cd();
    
    if (!fXPlaneGraph && !fVPlaneGraph && !fUPlaneGraph) {
        gPad->Update();
        return;
    }
    
    std::ostringstream titleStream;
    titleStream << "Hit Times and Charges"
                << " (Run: " << event->GetContext().GetRun()
                << " Event: " << event->GetContext().GetEvent()
                << ")";
    titleStream << ";Time #pm #Deltat_{rms} (#mus)";
    titleStream << ";Charge #pm #sigma (electrons)";

    // The histogram is labeled in microseconds.
    TH1F* frame = gPad->DrawFrame(minTime/unit::microsecond-5,
                                  minCharge,
                                  maxTime/unit::microsecond+5,
                                  1.05*maxCharge,
                                  titleStream.str().c_str());
    frame->GetYaxis()->SetTitleOffset(1.5);
    frame->GetXaxis()->SetTitleOffset(1.2);
    int bins = (maxTime-minTime)/unit::microsecond;
    frame->SetBins(bins,minTime/unit::microsecond,maxTime/unit::microsecond);

    if (fUPlaneGraph) {
        fUPlaneGraph->SetMarkerColor(kGreen+2);
        fUPlaneGraph->SetMarkerStyle(21);
        fUPlaneGraph->Draw("P");
    }

    if (fVPlaneGraph) {
        fVPlaneGraph->SetMarkerColor(kBlue);
        fVPlaneGraph->SetMarkerStyle(21);
        fVPlaneGraph->Draw("P");
    }

    if (fXPlaneGraph) {
        fXPlaneGraph->SetMarkerColor(kRed);
        fXPlaneGraph->SetMarkerStyle(20);
        fXPlaneGraph->Draw("P");
    }

    fGraphLegend = new TLegend(0.15,0.88,0.90,0.92);
    fGraphLegend->SetNColumns(5);
    if (fXPlaneGraph) fGraphLegend->AddEntry(fXPlaneGraph,"X Hits","p");
    if (fVPlaneGraph) fGraphLegend->AddEntry(fVPlaneGraph,"V Hits","p");
    if (fUPlaneGraph) fGraphLegend->AddEntry(fUPlaneGraph,"U Hits","p");
    fGraphLegend->Draw();

    gPad->Update();
}
