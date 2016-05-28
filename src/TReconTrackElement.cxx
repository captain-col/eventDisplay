#include "TReconTrackElement.hxx"
#include "TMatrixElement.hxx"
#include "TEventDisplay.hxx"

#include <TCaptLog.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>
#include <TUnitsTable.hxx>

#include <TReconCluster.hxx>
#include <TTrackState.hxx>
#include <THandle.hxx>

#include <TEveLine.h>

#include <sstream>

CP::TReconTrackElement::~TReconTrackElement() {}

CP::TReconTrackElement::TReconTrackElement(CP::TReconTrack& track,
                                           bool showUncertainty)
    : TEveElementList() {
    
    CP::THandle<CP::TTrackState> frontState = track.GetState();
    if (!frontState) {
        CaptError("TTrackState missing!");
    }
    TLorentzVector pos = frontState->GetPosition();
    TLorentzVector var = frontState->GetPositionVariance();
    TVector3 dir = frontState->GetDirection().Unit();
    TVector3 dvar = frontState->GetDirectionVariance();

    // This is used as the annotation, so it needs to be better.
    std::ostringstream title;
    title << "Track(" << track.GetUniqueID() << ") --" 
          << " Nodes: " << track.GetNodes().size()
          << " Hits: " << track.GetHits()->size()
          << ",  Energy Deposit: " << track.GetEDeposit()
          << std::endl
          << "   Position:  (" 
          << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
          << ", "<<unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
          << ", "<<unit::AsString(pos.Z(),std::sqrt(var.Z()),"length")
          << ")";
    
    title << std::endl
          << "   Direction: (" 
          << unit::AsString(dir.X(), dvar.X(),"direction")
          << ", " << unit::AsString(dir.Y(), dvar.Y(),"direction")
          << ", " << unit::AsString(dir.Z(), dvar.Z(),"direction")
          << ")";
    
    title << std::endl 
          << "   Algorithm: " << track.GetAlgorithmName()
          << " w/ goodness: " << track.GetQuality()
          << " / " << track.GetNDOF();

    CP::THandle<CP::TTrackState> backState = track.GetBack();
    if (backState) {
        TLorentzVector v = backState->GetPositionVariance();
        TLorentzVector p = backState->GetPosition();
        TVector3 d = backState->GetDirection().Unit();
        TVector3 dv = backState->GetDirectionVariance();
        title << std::endl
              << "   Back Pos:  " 
              << unit::AsString(p.X(),std::sqrt(v.X()),"length")
              <<", "<<unit::AsString(p.Y(),std::sqrt(v.Y()),"length")
              <<", "<<unit::AsString(p.Z(),std::sqrt(v.Z()),"length");
        title << std::endl
              << "   Back Dir: (" 
              << unit::AsString(d.X(), dv.X(),"direction")
              << ", " << unit::AsString(d.Y(), dv.Y(),"direction")
              << ", " << unit::AsString(d.Z(), dv.Z(),"direction")
              << ")";
    }
    else {
        title << std::endl
              << "      BACK STATE IS MISSING";
    }

    CaptNamedLog("track",title.str());

    CP::TReconNodeContainer& nodes = track.GetNodes();
    CaptNamedInfo("nodes", "Track Nodes " << nodes.size());
    CP::TCaptLog::IncreaseIndentation();

    std::ostringstream objName;
    objName << track.GetName() << "(" << track.GetUniqueID() << ")";
    
    SetMainColor(kBlue);
    SetName(objName.str().c_str());
    SetTitle(title.str().c_str());

    TEveLine* trackLine = new TEveLine(nodes.size());

    trackLine->SetName(objName.str().c_str()); 

    trackLine->SetTitle(title.str().c_str());
    trackLine->SetLineColor(kBlue);
    trackLine->SetLineStyle(1);
    trackLine->SetLineWidth(2);

    int p=0;

    // Start at the front state of the track
#define SHOW_FRONT_STATE
#ifdef SHOW_FRONT_STATE
    if (frontState) {
        TLorentzVector frontPos = frontState->GetPosition();
        TLorentzVector frontVar = frontState->GetPositionVariance();
        trackLine->SetPoint(p++, frontPos.X(), frontPos.Y(), frontPos.Z());
        CaptNamedInfo("nodes","Front:" 
                      << unit::AsString(frontPos.X(),
                                        std::sqrt(frontVar.X()),"length")
                      <<", "<<unit::AsString(frontPos.Y(),
                                             std::sqrt(frontVar.Y()),"length")
                      <<", "<<unit::AsString(frontPos.Z(),
                                             std::sqrt(frontVar.Z()),"length"));
        CP::TCaptLog::IncreaseIndentation();
        CaptNamedInfo("nodes",
                      "Front Dir: " 
                      << unit::AsString(frontState->GetDirection()));
        CP::TCaptLog::DecreaseIndentation();
    }
#endif
    
    for (CP::TReconNodeContainer::iterator n = nodes.begin();
         n != nodes.end(); ++n) {
        CP::THandle<CP::TTrackState> nodeState = (*n)->GetState();
        CP::THandle<CP::TReconBase> nodeObject = (*n)->GetObject();
        if (!nodeState) continue;
        if (nodeState == frontState) {
            CaptError("Node is front state");
        }
        if (nodeState == backState) {
            CaptError("Node is back state");
        }
        TLorentzVector nodePos = nodeState->GetPosition();
        TLorentzVector nodeVar = nodeState->GetPositionVariance();
        trackLine->SetPoint(p++, nodePos.X(), nodePos.Y(), nodePos.Z());
        CaptNamedInfo("nodes","Pos:" 
                      << unit::AsString(nodePos.X(),
                                        std::sqrt(nodeVar.X()),"length")
                      <<", "<<unit::AsString(nodePos.Y(),
                                             std::sqrt(nodeVar.Y()),"length")
                      <<", "<<unit::AsString(nodePos.Z(),
                                             std::sqrt(nodeVar.Z()),"length"));
        CP::TCaptLog::IncreaseIndentation();
        CP::THandle<CP::TReconCluster> cluster = nodeObject;
        if (cluster) {
            double delta = (cluster->GetPosition().Vect()-nodePos.Vect()).Mag();
            CaptNamedInfo("nodes","Cluster: " 
                          << unit::AsString(cluster->GetPosition().Vect(),
                                            "length")
                          << "  diff: " << unit::AsString(delta,"length"));
        }
        CaptNamedInfo("nodes",
                      "Dir: " << unit::AsString(nodeState->GetDirection()));
        CP::TCaptLog::DecreaseIndentation();
    }

#define SHOW_BACK_STATE
#ifdef SHOW_BACK_STATE
    // finish at the back state of the track
    if (backState) {
        TLorentzVector backPos = backState->GetPosition();
        TLorentzVector backVar = backState->GetPositionVariance();
        trackLine->SetPoint(p++, backPos.X(), backPos.Y(), backPos.Z());
        CaptNamedInfo("nodes","Back:" 
                      << unit::AsString(backPos.X(),
                                        std::sqrt(backVar.X()),"length")
                      <<", "<<unit::AsString(backPos.Y(),
                                             std::sqrt(backVar.Y()),"length")
                      <<", "<<unit::AsString(backPos.Z(),
                                             std::sqrt(backVar.Z()),"length"));
        CP::TCaptLog::IncreaseIndentation();
        CaptNamedInfo("nodes",
                      "Back Dir: " 
                      << unit::AsString(backState->GetDirection()));
        CP::TCaptLog::DecreaseIndentation();
    }
#endif
    
    AddElement(trackLine);

    CP::TCaptLog::DecreaseIndentation();

    if (!showUncertainty) return;
    
#ifdef SHOW_FRONT_STATE
    if (frontState) {
        TLorentzVector nodePos = frontState->GetPosition();
        TMatrixD nodeVar(3,3);
        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                nodeVar(i,j) = frontState->GetPositionCovariance(i,j);
            }
        }
        CP::TMatrixElement* uncertainty
            = new CP::TMatrixElement("Uncertainty",
                                     nodePos.Vect(),
                                     nodeVar,
                                     false);
        uncertainty->SetMainColor(kCyan-9);
        AddElement(uncertainty);
    }
#endif

    // Add the uncertainty.
    for (CP::TReconNodeContainer::iterator n = nodes.begin();
         n != nodes.end(); ++n) {
        CP::THandle<CP::TTrackState> nodeState = (*n)->GetState();
        CP::THandle<CP::TReconBase> nodeObject = (*n)->GetObject();
        if (!nodeState) {
            CaptError("Node is missing");
            continue;
        }
        TLorentzVector nodePos = nodeState->GetPosition();
        TMatrixD nodeVar(3,3);
        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                nodeVar(i,j) = nodeState->GetPositionCovariance(i,j);
            }
        }
        CP::TMatrixElement* uncertainty
            = new CP::TMatrixElement("Uncertainty",
                                     nodePos.Vect(),
                                     nodeVar,
                                     false);
        int color = kBlue;
        double length = 0;
        if (n == nodes.begin()) {
            CP::THandle<CP::TTrackState> b = (*(n+1))->GetState();
            length = 0.75*(frontState->GetPosition().Vect()
                          - b->GetPosition().Vect()).Mag();
        }
        else if ((n+1) == nodes.end()) {
            CP::THandle<CP::TTrackState> b = (*(n-1))->GetState();
            length = 0.75*(backState->GetPosition().Vect()
                          - b->GetPosition().Vect()).Mag();
        }
        else {
            CP::THandle<CP::TTrackState> a = (*(n-1))->GetState();
            CP::THandle<CP::TTrackState> b = (*(n+1))->GetState();
            length = 0.5*(a->GetPosition().Vect()
                          - b->GetPosition().Vect()).Mag();
        }
        // EDeposit is in measured charge.
        CP::THandle<CP::TReconCluster> cluster = nodeObject;
        double energy
            = CP::TEventDisplay::Get().CrudeEnergy(cluster->GetEDeposit());
        double dEdX = energy;
        if (length > 1*unit::mm) {
            dEdX /= length;              // Get charge per length;
            double minEnergy = 0.18*unit::MeV/unit::mm;
            double maxEnergy = 3.0*unit::MeV/unit::mm;
            color = TEventDisplay::Get().LogColor(dEdX,
                                                  minEnergy,maxEnergy,2.0);
        }
        uncertainty->SetMainColor(color);
        AddElement(uncertainty);
    }

#ifdef SHOW_BACK_STATE
    if (backState) {
        TLorentzVector nodePos = backState->GetPosition();
        TMatrixD nodeVar(3,3);
        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                nodeVar(i,j) = backState->GetPositionCovariance(i,j);
            }
        }
        CP::TMatrixElement* uncertainty
            = new CP::TMatrixElement("Uncertainty",
                                     nodePos.Vect(),
                                     nodeVar,
                                     false);
        uncertainty->SetMainColor(kGreen+2);
        AddElement(uncertainty);
    }
#endif


}

