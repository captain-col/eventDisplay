#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TPlotDigitsHits.hxx"
#include "TEventChangeManager.hxx"
#include "TFindResultsHandler.hxx"
#include "TTrajectoryChangeHandler.hxx"
#include "TG4HitChangeHandler.hxx"

#include <TCaptLog.hxx>

#include <TEveManager.h>
#include <TColor.h>
#include <TGLViewer.h>

#include <algorithm>
#include <cmath>

CP::TEventDisplay* CP::TEventDisplay::fEventDisplay = NULL;

CP::TEventDisplay& CP::TEventDisplay::Get(void) {
    if (!fEventDisplay) {
        fEventDisplay = new CP::TEventDisplay();
        fEventDisplay->Init();
    }
    return *fEventDisplay;
}

CP::TEventDisplay::TEventDisplay() {}

void CP::TEventDisplay::Init() {
    TEveManager::Create();

    TGLViewer* glViewer = gEve->GetDefaultGLViewer();
    glViewer->SetCurrentCamera(TGLViewer::kCameraPerspXOY);
    glViewer->SetGuideState(TGLUtil::kAxesEdge,kTRUE,kFALSE,0);

    // This is accessed through the GUI() method.
    fGUIManager = new TGUIManager();

    // Create the event display manager.  This needs the GUI, so it has to be
    // done after TGUIManager is created.
    fEventChangeManager = new TEventChangeManager();
    fEventChangeManager->AddNewEventHandler(new TFindResultsHandler());
    fEventChangeManager->AddUpdateHandler(new TTrajectoryChangeHandler());
    fEventChangeManager->AddUpdateHandler(new TG4HitChangeHandler());

    // Connect the class to draw digits to the GUI.
    fPlotDigitsHits = new TPlotDigitsHits();
    CP::TEventDisplay::Get().GUI().GetDrawXDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=0)");
    
    CP::TEventDisplay::Get().GUI().GetDrawVDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=1)");
    
    CP::TEventDisplay::Get().GUI().GetDrawUDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=2)");
    

    CaptLog("Event display constructed");
}

CP::TEventDisplay::~TEventDisplay() {
    CaptLog("Event display deconstructed");
}

int CP::TEventDisplay::LinearColor(double value, double minVal, double maxVal) {
    int nCol = TColor::GetNumberOfColors();
    int iValue = nCol*(value - minVal)/(maxVal - minVal);
    nCol = std::max(0,std::min(iValue,nCol));
    return TColor::GetColorPalette(nCol);
}

int CP::TEventDisplay::LogColor(double value, double minVal, double maxVal) {
    int nCol = TColor::GetNumberOfColors();
    value = std::max(0.0,std::min((value - minVal)/(maxVal - minVal),1.0));
    int iValue = nCol*std::log10(1.0+1000*value)/3.0;
    nCol = std::max(0,std::min(iValue,nCol));
    return TColor::GetColorPalette(nCol);
}
