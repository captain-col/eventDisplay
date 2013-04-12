#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TEventChangeManager.hxx"
#include "TTrajectoryChangeHandler.hxx"
#include "TG4HitChangeHandler.hxx"

#include <TCaptLog.hxx>

#include <TEveManager.h>
#include <TColor.h>

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

    // This is accessed through the GUI() method.
    fGUIManager = new TGUIManager();

    // Create the event display manager.  This needs the GUI, so it has to be
    // done after TGUIManager is created.
    fEventChangeManager = new TEventChangeManager();

    fEventChangeManager->AddHandler(new TTrajectoryChangeHandler());
    fEventChangeManager->AddHandler(new TG4HitChangeHandler());

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
