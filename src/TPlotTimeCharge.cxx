#include "TPlotTimeCharge.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TEvent.hxx>
#include <THit.hxx>
#include <THandle.hxx>
#include <THitSelection.hxx>
#include <TEventFolder.hxx>
#include <CaptGeomId.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TGraphErrors.h>

#include <iostream>

CP::TPlotTimeCharge::TPlotTimeCharge()
    : fXPlaneGraph(NULL), fVPlaneGraph(NULL), fUPlaneGraph(NULL) {
}

CP::TPlotTimeCharge::~TPlotTimeCharge() {}

void CP::TPlotTimeCharge::DrawTimeCharge() {

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");

    if (!hits) return;

    double minTime = 1E+22;
    double maxTime = -1E+22;
    double minCharge = 1E+22;
    double maxCharge = -1E+22;
    
    // Fill the graph for the X hits.
    const int maxPoints = 10000;
    double time[maxPoints];
    double timeRMS[maxPoints];
    double charge[maxPoints];
    double chargeUnc[maxPoints];
    int points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsXWire((*h)->GetGeomId())) continue;
        time[points] = (*h)->GetTime();
        timeRMS[points] = (*h)->GetTimeRMS();
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
    }
    if (fXPlaneGraph) delete fXPlaneGraph;
    fXPlaneGraph
        = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);

    points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsVWire((*h)->GetGeomId())) continue;
        time[points] = (*h)->GetTime();
        timeRMS[points] = (*h)->GetTimeRMS();
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
    }
    if (fVPlaneGraph) delete fVPlaneGraph;
    fVPlaneGraph
        = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);

    points = 0;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (!CP::GeomId::Captain::IsUWire((*h)->GetGeomId())) continue;
        time[points] = (*h)->GetTime();
        timeRMS[points] = (*h)->GetTimeRMS();
        charge[points] = (*h)->GetCharge();
        chargeUnc[points] = (*h)->GetChargeUncertainty();
        ++points;
        minTime = std::min((*h)->GetTime(),minTime);
        maxTime = std::max((*h)->GetTime(),maxTime);
        minCharge = std::min((*h)->GetCharge(),minCharge);
        maxCharge = std::max((*h)->GetCharge(),maxCharge);
    }
    if (fUPlaneGraph) delete fUPlaneGraph;
    fUPlaneGraph
        = new TGraphErrors(points,time,charge,timeRMS,chargeUnc);

    TCanvas* canvas = NULL;
    canvas = (TCanvas*) gROOT->FindObject("canvasTimeCharge");
    if (!canvas) {
        canvas=new TCanvas("canvasTimeCharge","Hit Times and Charges",500,300);
    }

    gPad->DrawFrame(minTime,maxTime,minCharge,maxCharge,"Times and Charges");

    fXPlaneGraph->SetMarkerColor(kRed);
    fXPlaneGraph->SetMarkerStyle(23);
    fXPlaneGraph->Draw("AP");

    fVPlaneGraph->SetMarkerColor(kBlue);
    fVPlaneGraph->SetMarkerStyle(22);
    fVPlaneGraph->Draw("P");

    fUPlaneGraph->SetMarkerColor(kBlue);
    fUPlaneGraph->SetMarkerStyle(23);
    fUPlaneGraph->Draw("P");
}
