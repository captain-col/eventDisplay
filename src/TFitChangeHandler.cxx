#include "TFitChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TShowDriftHits.hxx"

#include <TCaptLog.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>

#include <TGeoManager.h>
#include <TGButton.h>
#include <TGListBox.h>
#include <TCollection.h>

#include <TEveManager.h>
#include <TEveLine.h>

#include <sstream>

CP::TFitChangeHandler::TFitChangeHandler() {
    fHitList = new TEveElementList("HitList","Reconstructed 3D Hits");
    fHitList->SetMainColor(kYellow);
    fHitList->SetMainAlpha(1.0);
    gEve->AddElement(fHitList);
}

CP::TFitChangeHandler::~TFitChangeHandler() {
}

void CP::TFitChangeHandler::Apply() {

    fHitList->DestroyElements();

    if (!CP::TEventDisplay::Get().GUI().GetShowFitsButton()->IsOn()) {
        CaptLog("Fits display disabled");
        return;
    }

    CaptLog("Handle the fit information");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    // Get a TList of all of the selected entries.
    TList selected;
    CP::TEventDisplay::Get().GUI().GetResultsList()
        ->GetSelectedEntries(&selected);

    CP::TShowDriftHits showDrift(1.6*unit::mm/unit::microsecond);

    // Iterate through the list of selected entries.
    TIter next(&selected);
    TGLBEntry* lbEntry;
    while ((lbEntry = (TGLBEntry*) next())) {
        std::string objName(lbEntry->GetTitle());
        CP::THandle<CP::TReconObjectContainer> objects 
            = event->Get<CP::TReconObjectContainer>(objName.c_str());
        if (!objects) {
            CaptError("No object container " << objName);
            continue;
        }            
        std::cout << " objects " << objects->GetName() << std::endl;
        for (CP::TReconObjectContainer::iterator obj = objects->begin();
             obj != objects->end(); ++obj) {
            std::cout << "    object " << (*obj)->GetTitle() << std::endl;
            std::cout << "      hits " << (*obj)->GetHits()->size() 
                      << std::endl;
            double t0 = (*obj)->GetHits()->front()->GetTime();
            showDrift(fHitList, *(*obj)->GetHits(), t0);
        }
    }

}
