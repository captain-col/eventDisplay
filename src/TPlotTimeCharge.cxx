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

#include <TCanvas.h>
#include <TPad.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TH1F.h>
#include <TF1.h>

#include <iostream>
#include <sstream>

CP::TPlotTimeCharge::TPlotTimeCharge()
    : fXPlaneGraph(NULL), fVPlaneGraph(NULL), fUPlaneGraph(NULL),
      fGraphLegend(NULL) {
    fElectronLifeFunction = new TF1("electronLifeFunction",
                                    "[2]*exp(-(x-[1])/[0])");
    fElectronLifeFunction->SetParName(0,"Electron Lifetime");
    fElectronLifeFunction->SetParName(1,"Time Offset");
    fElectronLifeFunction->SetParName(2,"Normalization");
}

CP::TPlotTimeCharge::~TPlotTimeCharge() {}

void CP::TPlotTimeCharge::FitTimeCharge() {
    TCanvas* canvas = (TCanvas*) gROOT->FindObject("canvasTimeCharge");
    if (!canvas) return;

    // If the x plane hits are drawn, fit them.  Then try the V and U planes.
    TGraphErrors* graph = fXPlaneGraph;
    if (!graph) graph = fVPlaneGraph;
    if (!graph) graph = fUPlaneGraph;
    if (!graph) {
        CaptError("No hits to fit");
        return;
    }

    // Determine the range of the fit.  This fits the hits that are in the
    // zoomed histogram.
    double xMin = canvas->GetUxmin();
    double xMax = canvas->GetUxmax();
    fElectronLifeFunction->SetRange(xMin+(xMax-xMin)/20.0,
                                    xMax-(xMax-xMin)/20.0);

    // Find the range of "normalizations to use".
    int closestPoint = 1E+10;
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
    
    // Set the initial parameter values.
    fElectronLifeFunction->SetParameters(20.0, xMin, closestNorm);
    // Limit the range of electron lifetimes to fit (5us to 10 ms)
    fElectronLifeFunction->SetParLimits(0, 5.0, 10000.0);
    // Fix the offset of the fit start to the beginning of the fit range.
    fElectronLifeFunction->SetParLimits(1, xMin, xMin-1.0);
    // Limit the range of normalizations
    fElectronLifeFunction->SetParLimits(2,0.7*closestNorm,2.0*closestNorm);

    // Do the fit!
    graph->Fit(fElectronLifeFunction,"R,ROB=0.85");

    gPad->Update();
}

void CP::TPlotTimeCharge::DrawTimeCharge() {

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");
    if (!hits) return;

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

    // Fill the graph for the U hits
    points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsUWire((*h)->GetGeomId())) continue;
        if (!drawUHits) continue;
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
    
    if (!fXPlaneGraph && !fVPlaneGraph && !fUPlaneGraph) return;
    
    std::ostringstream titleStream;
    titleStream << "Hit Times and Charges"
                << " (Run: " << event->GetContext().GetRun()
                << " Event: " << event->GetContext().GetEvent()
                << ")";
    titleStream << ";Time #pm #Deltat_{rms} (#mus)";
    titleStream << ";Charge #pm #sigma (electrons)";
        
    TH1F* frame = gPad->DrawFrame(minTime/unit::microsecond,
                                  minCharge,
                                  maxTime/unit::microsecond,
                                  maxCharge,
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

    fGraphLegend = new TLegend(0.87,0.85,0.97,0.95);
    if (fXPlaneGraph) fGraphLegend->AddEntry(fXPlaneGraph,"X Hits","p");
    if (fVPlaneGraph) fGraphLegend->AddEntry(fVPlaneGraph,"V Hits","p");
    if (fUPlaneGraph) fGraphLegend->AddEntry(fUPlaneGraph,"U Hits","p");
    fGraphLegend->Draw();

    gPad->Update();
}