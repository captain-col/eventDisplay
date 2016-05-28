#include "TPMTChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TShowPMTHits.hxx"

#include <TCaptLog.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>
#include <TRuntimeParameters.hxx>
#include <TUnitsTable.hxx>

#include <TGeoManager.h>
#include <TGeoShape.h>
#include <TGeoEltu.h>
#include <TGeoSphere.h>
#include <TGeoMatrix.h>

#include <TVectorF.h>
#include <TMatrixF.h>

#include <TGButton.h>
#include <TGListBox.h>
#include <TCollection.h>

#include <TEveManager.h>
#include <TEveGeoShape.h>
#include <TEveLine.h>
#include <TEveRGBAPalette.h>

#include <sstream>

CP::TPMTChangeHandler::TPMTChangeHandler() {

    fPMTList = new TEveElementList("PMTList","Reconstructed Objects");
    fPMTList->SetMainColor(kGreen);
    fPMTList->SetMainAlpha(0.5);
    gEve->AddElement(fPMTList);

    fPalette = new TEveRGBAPalette(0,15,true,true,false);

}

CP::TPMTChangeHandler::~TPMTChangeHandler() {}

void CP::TPMTChangeHandler::Apply() {

    fPMTList->DestroyElements();

#ifdef USE_GUI_PMTS
    if (!CP::TEventDisplay::Get().GUI().GetShowPMTsButton()->IsOn()) {
        CaptLog("PMTs display disabled");
        return;
    }
#endif

    CaptLog("Handle the PMT information");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    CP::THandle<CP::THitSelection> pmts
        = event->Get<CP::THitSelection>("~/hits/pmt");
    if (!pmts) return;

    // Draw the hits.
    CP::TShowPMTHits showPMTs(fPalette);
    showPMTs(fPMTList, *pmts);

}

