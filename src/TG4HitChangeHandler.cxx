#include "TG4HitChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TCaptLog.hxx>
#include <TG4HitSegment.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>

#include <TGeoManager.h>
#include <TGButton.h>

#include <TEveManager.h>
#include <TEveLine.h>

#include <sstream>




CP::TG4HitChangeHandler::TG4HitChangeHandler() {
    fG4HitList = new TEveElementList("g4HitList","Geant4 Truth Hits");
    fG4HitList->SetMainColor(kYellow);
    fG4HitList->SetMainAlpha(1.0);
    gEve->AddElement(fG4HitList);
}

CP::TG4HitChangeHandler::~TG4HitChangeHandler() {
}

void CP::TG4HitChangeHandler::Apply() {

    fG4HitList->DestroyElements();

    if (!CP::TEventDisplay::Get().GUI().GetShowG4HitsButton()->IsOn()) {
        CaptLog("G4 hits disabled");
        return;
    }

    CaptLog("Handle the geant4 truth hits");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    CP::THandle<CP::TDataVector> truthHits 
        = event->Get<CP::TDataVector>("truth/g4Hits");

    if (!truthHits) {
        CaptLog("No truth hits in event");
        return;
    }

    for (CP::TDataVector::iterator h = truthHits->begin();
         h != truthHits->end();
         ++h) {
        CP::THandle<CP::TG4HitContainer> g4Hits =
            (*h)->Get<CP::TG4HitContainer>(".");
        if (!g4Hits) {
            CaptError("truth/g4Hits object that is not a TG4HitContainer: "
                       << (*h)->GetName());
            continue;
        }

        for (CP::TG4HitContainer::const_iterator h = g4Hits->begin(); 
             h != g4Hits->end();
             ++h) {
            const CP::TG4HitSegment* seg 
                = dynamic_cast<const CP::TG4HitSegment*>((*h));
            
            if (!seg) {
                CaptWarn("Not showing TG4Hit not castable as a TG4HitSegment.");
                continue;
            }

            double energy = seg->GetEnergyDeposit();
            if (seg->GetTrackLength()>0.01*unit::mm) {
                energy /= seg->GetTrackLength();
            }
            
            TEveLine* eveHit = new TEveLine(2);
            eveHit->SetName((*h)->GetName());
            std::ostringstream title;
            title << "G4 Hit " << std::fixed << std::setprecision(1)
                  << energy/(unit::MeV/unit::cm) << " MeV/cm"
                  << " for " << seg->GetTrackLength()/unit::mm << " mm";
            eveHit->SetTitle(title.str().c_str());
            double minEnergy = 0.1*unit::MeV;
            double maxEnergy = 20.0*unit::MeV;
            eveHit->SetLineColor(TEventDisplay::Get().LogColor(energy,
                                                               minEnergy,
                                                               maxEnergy));
            eveHit->SetPoint(0,
                             seg->GetStartX(),
                             seg->GetStartY(),
                             seg->GetStartZ());
            eveHit->SetPoint(1,
                             seg->GetStopX(),
                             seg->GetStopY(),
                             seg->GetStopZ());
            fG4HitList->AddElement(eveHit);
        }

    }

}
