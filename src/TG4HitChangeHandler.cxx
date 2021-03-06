#include "TG4HitChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TCaptLog.hxx>
#include <TG4HitSegment.hxx>
#include <TG4Trajectory.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <TUnitsTable.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>
#include <TGeomIdManager.hxx>
#include <TManager.hxx>
#include <CaptGeomId.hxx>

#include <TGButton.h>

#include <TEveManager.h>
#include <TEveLine.h>

#include <sstream>

CP::TG4HitChangeHandler::TG4HitChangeHandler() {
    fG4HitList = new TEveElementList("g4HitList","Geant4 Truth Hits");
    fG4HitList->SetMainColor(kCyan);
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

    CP::THandle<CP::TG4TrajectoryContainer> truthTrajectories
        = event->Get<CP::TG4TrajectoryContainer>("truth/G4Trajectories");

    double minEnergy = 0.18*unit::MeV/unit::mm;
    double maxEnergy = 3.0*unit::MeV/unit::mm;

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
            double length = seg->GetTrackLength();
            double dEdX = energy;
            if (length>0.01*unit::mm) dEdX /= length;

            TGeometryId id;
            bool validId 
                = CP::TManager::Get().GeomId().GetGeometryId(
                    seg->GetStartX(),seg->GetStartY(),seg->GetStartZ(), id);

            // If the hit is outside of drift, only plot the long ones.
            if (validId 
                && id!=CP::GeomId::Captain::Drift() 
                && length < 2*unit::mm) continue;

            
            TEveLine* eveHit = new TEveLine(2);
            eveHit->SetName((*h)->GetName());
            std::ostringstream title;
            title << "G4 Hit";
            if (truthTrajectories) {
                int part = seg->GetContributor(0);
                CP::THandle<CP::TG4Trajectory> traj 
                    = truthTrajectories->GetTrajectory(part);
                if (traj) {
                    title << " " << traj->GetParticleName();
                    title << " (" << 
                        unit::AsString(traj->GetInitialMomentum().P(),
                                       "momentum") << ")";
                }
            }
            title << std::fixed << std::setprecision(2)
                  << " " << dEdX/(unit::MeV/unit::cm) << " MeV/cm";
            title << " for " << unit::AsString(length,"length")
                  << " at (" <<  unit::AsString(seg->GetStartX(), "length")
                  << "," <<  unit::AsString(seg->GetStartY(), "length")
                  << "," <<  unit::AsString(seg->GetStartZ(), "length") << ")";

            eveHit->SetTitle(title.str().c_str());

            if (validId && id==CP::GeomId::Captain::Drift()) {
                eveHit->SetLineColor(TEventDisplay::Get().LogColor(
                                         dEdX,
                                         minEnergy,
                                         maxEnergy,
                                         3));
            }
            else {
                eveHit->SetLineColor(kCyan);
            }

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
