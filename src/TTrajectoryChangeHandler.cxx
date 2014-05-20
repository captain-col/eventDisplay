#include "TTrajectoryChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TCaptLog.hxx>
#include <TG4Trajectory.hxx>
#include <TG4TrajectoryPoint.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>

#include <TGeoManager.h>
#include <TGButton.h>

#include <TEveManager.h>
#include <TEveLine.h>

#include <sstream>

CP::TTrajectoryChangeHandler::TTrajectoryChangeHandler() {
    fTrajectoryList = new TEveElementList("g4Trajectories",
                                          "Geant4 Trajectories");
    fTrajectoryList->SetMainColor(kYellow);
    fTrajectoryList->SetMainAlpha(1.0);
    gEve->AddElement(fTrajectoryList);
}

CP::TTrajectoryChangeHandler::~TTrajectoryChangeHandler() {
}

void CP::TTrajectoryChangeHandler::Apply() {

    fTrajectoryList->DestroyElements();

    if (!CP::TEventDisplay::Get().GUI().GetShowTrajectoriesButton()->IsOn()) {
        CaptLog("Trajectories disabled");
        return;
    }

    CaptLog("Handle the trajectories");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    CP::THandle<CP::TG4TrajectoryContainer> trajectories
        = event->Get<CP::TG4TrajectoryContainer>("truth/G4Trajectories");
    
    if (!trajectories) {
        CaptLog("No trajectories in event");
        return;
    }

    for (CP::TG4TrajectoryContainer::iterator tPair = trajectories->begin();
         tPair != trajectories->end();
         ++tPair) {
        CP::TG4Trajectory& traj = tPair->second;
        const CP::TG4Trajectory::Points& points = traj.GetTrajectoryPoints();
        const TParticlePDG *pdg = traj.GetParticle();

        bool charged = false;
        if (pdg) {
            charged = (std::abs(pdg->Charge()) > 0.1);
        }

        std::ostringstream label;
        label << traj.GetParticleName() 
              << " (" << traj.GetInitialMomentum().E()/unit::MeV << " MeV)";

        TEveLine *track = new TEveLine();
        track->SetName("trajectory");
        track->SetTitle(label.str().c_str());
        if (charged) {
            track->SetLineColor(kYellow);
            track->SetLineStyle(3);
        }
        else {
            track->SetLineColor(kYellow+4);
            track->SetLineStyle(4);
        }

        for (std::size_t p = 0; p < points.size(); ++p) {
            gGeoManager->PushPath();
            gGeoManager->FindNode(points[p].GetPosition().X(),
                                  points[p].GetPosition().Y(),
                                  points[p].GetPosition().Z());
            std::string path(gGeoManager->GetPath());
            gGeoManager->PopPath();
            if (path.find("/Liquid_") == std::string::npos) continue;
            track->SetPoint(p, 
                            points[p].GetPosition().X(),
                            points[p].GetPosition().Y(),
                            points[p].GetPosition().Z());
        }
        fTrajectoryList->AddElement(track);
    }
}
