#include "TPlotHitSamples.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TEvent.hxx>
#include <THit.hxx>
#include <THandle.hxx>
#include <THitSelection.hxx>
#include <TEventFolder.hxx>
#include <CaptGeomId.hxx>
#include <HEPUnits.hxx>
#include <TUnitsTable.hxx>

#include <TCanvas.h>
#include <TPad.h>
#include <TGraph.h>
#include <TList.h>
#include <TPolyLine.h>
#include <TBox.h>

#include <iostream>
#include <sstream>

CP::TPlotHitSamples::TPlotHitSamples() {
}

CP::TPlotHitSamples::~TPlotHitSamples() {
    for (std::vector<TObject*>::iterator g = fGraphicsDelete.begin();
         g != fGraphicsDelete.end(); ++g) {
        delete (*g);
    }
    fGraphicsDelete.clear();
}

void CP::TPlotHitSamples::DrawHitSamples() {
    std::cout << "Draw Hit Samples" << std::endl;
    
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    CP::THandle<CP::THitSelection> hits
        = event->Get<CP::THitSelection>("~/hits/drift");
    if (!hits) {
        CaptError("No hits to draw");
        return;
    }

    // Check to see if the hits have been plotted on time vs wire.
    TCanvas* canvasDigits = NULL;
    int dType = -1;
    if (!canvasDigits) {
        std::cout << "Draw X hit" << std::endl;
        canvasDigits = (TCanvas*) gROOT->FindObject("canvasXDigits");
        dType = 0;
    }
    if (!canvasDigits) {
        std::cout << "Draw V hit" << std::endl;
        canvasDigits = (TCanvas*) gROOT->FindObject("canvasVDigits");
        dType = 1;
    }
    if (!canvasDigits) {
        std::cout << "Draw U hit" << std::endl;
        canvasDigits = (TCanvas*) gROOT->FindObject("canvasUDigits");
        dType = 2;
    }

    if (!canvasDigits) {
        std::cout << "No hits to select" << std::endl;
        return;
    }
    
    // Find the time range shown on the time vs wire plot.  If the plot is in
    // sample vs wire, then this will need to be corrected to be in time.
    double minDigitTime = 1E+22;
    double maxDigitTime = -1E+22;
    double minWireNumber = 1E+22;
    double maxWireNumber = -1E+22;
    if (canvasDigits) {
        double s = 1.0;
        double b = 0.0;
        std::string title = canvasDigits->GetTitle();
        if (title.find("Sample") != std::string::npos) {
            // Set *VERY* rough calibration coefficients.
            s=0.5;
            b=1600.0;
        }
        minDigitTime = s*canvasDigits->GetUymin()-b;
        maxDigitTime = s*canvasDigits->GetUymax()-b;
        minWireNumber = canvasDigits->GetUxmin();
        maxWireNumber = canvasDigits->GetUxmax();
    }
    
    // Convert to time.
    minDigitTime *= unit::microsecond;
    maxDigitTime *= unit::microsecond;
    
    std::cout << "Hit between " << minDigitTime
              << " " << maxDigitTime << std::endl;
    std::cout << "    " << minWireNumber << " " << maxWireNumber << std::endl;

    // Find the hit to draw.
    CP::THandle<THit> hit;
    for (CP::THitSelection::iterator h = hits->begin();
         h != hits->end(); ++h) {
        if (dType == 0 && !CP::GeomId::Captain::IsXWire((*h)->GetGeomId())){
            continue;
        }
        if (dType == 1 && !CP::GeomId::Captain::IsVWire((*h)->GetGeomId())){
            continue;
        }
        if (dType == 2 && !CP::GeomId::Captain::IsUWire((*h)->GetGeomId())){
            continue;
        }
        if ((*h)->GetTime() < minDigitTime) continue;
        if ((*h)->GetTime() > maxDigitTime) continue;
        double wire = CP::GeomId::Captain::GetWireNumber((*h)->GetGeomId());
        if (wire < minWireNumber) continue;
        if (wire > maxWireNumber) continue;
        hit = *h;
        maxWireNumber = wire;
    }

    if (!hit) {
        std::cout << "Hit not found" << std::endl;
        return;
    }

    int wireNumber = CP::GeomId::Captain::GetWireNumber(hit->GetGeomId());
    std::cout << "Draw Hit on wire " << wireNumber << std::endl;
    hit->ls();
    
    TCanvas* canvas = NULL;
    canvas = (TCanvas*) gROOT->FindObject("canvasHitSamples");
    if (!canvas) {
        canvas=new TCanvas("canvasHitSamples","Hit Samples",500,300);
    }
    canvas->SetTopMargin(0.07);
    canvas->SetRightMargin(0.04);
    canvas->cd();
    
    std::ostringstream titleStream;
    titleStream << hit->GetChannelId() << " [";
    if (CP::GeomId::Captain::IsXWire(hit->GetGeomId())) {
        titleStream << "X-";
    }
    if (CP::GeomId::Captain::IsVWire(hit->GetGeomId())) {
        titleStream << "V-";
    }
    if (CP::GeomId::Captain::IsUWire(hit->GetGeomId())) {
        titleStream << "U-";
    }

    titleStream << wireNumber << "]"
                << " @ " << unit::AsString(hit->GetTime(),
                                           hit->GetTimeUncertainty(),"time")
                << " (rms: " << unit::AsString(hit->GetTimeRMS(),"time") << ")"
                << " w/ " << unit::AsString(hit->GetCharge(),
                                            hit->GetChargeUncertainty(),
                                            "charge");
    
    std::cout << titleStream.str() << std::endl;

    for (std::vector<TObject*>::iterator g = fGraphicsDelete.begin();
         g != fGraphicsDelete.end(); ++g) {
        delete (*g);
    }
    fGraphicsDelete.clear();

    const int maxPoints = 10000;
    double time[maxPoints];
    double timeBar[maxPoints];
    double charge[maxPoints];
    double chargeUnc[maxPoints];
    int points;
    double startTime = hit->GetTimeStart();
    double stopTime = hit->GetTimeStop();
    double dTime = (stopTime - startTime)/hit->GetTimeSamples();
    double minCharge = 1E+22;
    double maxCharge = -1E+22;
    for (int i=0; i<hit->GetTimeSamples(); ++i) {
        time[points] = startTime + i*dTime;
        charge[points++] = hit->GetTimeSample(i);
        minCharge = std::min(hit->GetTimeSample(i), minCharge);
        maxCharge = std::max(hit->GetTimeSample(i), maxCharge);
    }
    if (maxCharge > 0.0 && minCharge < 0.0) minCharge = 0.0;
    
    double chargeY = hit->GetTimeUpperBound() - hit->GetTimeLowerBound();
    chargeY = dTime*hit->GetCharge()/chargeY;
    double chargeUncY = hit->GetTimeUpperBound() - hit->GetTimeLowerBound();
    chargeUncY= dTime*hit->GetChargeUncertainty()/chargeUncY;

    TGraph* hitGraph =  new TGraph(points,time,charge);
    fGraphicsDelete.push_back(hitGraph);
    hitGraph->SetName("GraphHitSamples");
    hitGraph->SetTitle(titleStream.str().c_str());
    hitGraph->SetMinimum(minCharge - chargeY - 2*chargeUncY);
    hitGraph->Draw("AC*");

    {
        int n=0;
        double px[10];
        double py[10];
        px[n] = hit->GetTime();
        py[n++] = minCharge;
        px[n] = hit->GetTime();
        py[n++] = maxCharge;
        TPolyLine* pline = new TPolyLine(n,px,py);
        pline->SetLineWidth(1);
        pline->SetLineColor(kRed);
        pline->Draw();
        fGraphicsDelete.push_back(pline);
    }

    {
        TBox* box = new TBox(hit->GetTimeLowerBound(),
                             minCharge,
                             hit->GetTimeUpperBound(),
                             minCharge-chargeY);
        fGraphicsDelete.push_back(box);
        box->SetLineStyle(1);
        box->SetFillColor(kRed);
        box->Draw();
        std::cout << hit->GetTimeLowerBound()
                  << " " << hit->GetTimeUpperBound()
                  << " " << hit->GetCharge()
                  << " " << chargeY
                  << std::endl;
    }
    
    {
        TBox* box = new TBox(hit->GetTimeLowerBound(),
                             minCharge+chargeUncY,
                             hit->GetTimeUpperBound(),
                             minCharge-chargeUncY);
        fGraphicsDelete.push_back(box);
        box->SetLineStyle(1);
        box->SetLineWidth(1);
        box->SetLineColor(kBlue);
        box->SetFillStyle(0);
        box->Draw();
        std::cout << hit->GetTimeLowerBound()
                  << " " << hit->GetTimeUpperBound()
                  << " " << hit->GetCharge()
                  << " " << chargeY
                  << std::endl;
    }
    
    {
        int n=0;
        double px[10];
        double py[10];
        px[n] = hit->GetTime()-hit->GetTimeRMS();
        py[n++] = 0.6065*(maxCharge-minCharge) + minCharge;
        px[n] = hit->GetTime()+hit->GetTimeRMS();
        py[n++] = 0.6065*(maxCharge-minCharge) + minCharge;
        TPolyLine* pline = new TPolyLine(n,px,py);
        pline->SetLineWidth(1);
        pline->SetLineColor(kRed);
        pline->Draw();
        fGraphicsDelete.push_back(pline);
    }

    {
        int n=0;
        double px[10];
        double py[10];
        px[n] = hit->GetTime()-hit->GetTimeUncertainty();
        py[n++] = maxCharge;
        px[n] = hit->GetTime()+hit->GetTimeUncertainty();
        py[n++] = maxCharge;
        TPolyLine* pline = new TPolyLine(n,px,py);
        pline->SetLineWidth(1);
        pline->SetLineColor(kRed);
        pline->Draw();
        fGraphicsDelete.push_back(pline);
    }
    
    {
        int n=0;
        double px[10];
        double py[10];
        px[n] = hit->GetTimeLowerBound();
        py[n++] = minCharge;
        px[n] = hit->GetTimeUpperBound();
        py[n++] = minCharge;
        TPolyLine* pline = new TPolyLine(n,px,py);
        pline->SetLineWidth(1);
        pline->SetLineColor(kRed);
        pline->Draw();
        fGraphicsDelete.push_back(pline);
    }

    gPad->Update();
}
