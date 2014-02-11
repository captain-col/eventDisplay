#include "TFitChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TShowDriftHits.hxx"

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

#include <sstream>

CP::TFitChangeHandler::TFitChangeHandler() {
    fHitList = new TEveElementList("HitList","Reconstructed 3D Hits");
    fHitList->SetMainColor(kYellow);
    fHitList->SetMainAlpha(1.0);
    gEve->AddElement(fHitList);

    fFitList = new TEveElementList("FitList","Reconstructed Objects");
    fFitList->SetMainColor(kGreen);
    fFitList->SetMainAlpha(0.5);
    gEve->AddElement(fFitList);

    fEnergyPerCharge
        = CP::TRuntimeParameters::Get().GetParameterD(
            "eventDisplay.fits.energyPerCharge");
}

CP::TFitChangeHandler::~TFitChangeHandler() {
}

void CP::TFitChangeHandler::Apply() {

    fHitList->DestroyElements();
    fFitList->DestroyElements();

    if (!CP::TEventDisplay::Get().GUI().GetShowFitsButton()->IsOn()) {
        CaptLog("Fits display disabled");
        return;
    }

    if (CP::TEventDisplay::Get().GUI().GetShowFitsHitsButton()->IsOn()) {
        CaptLog("Showing Fit Hits");
        fShowFitsHits = true;
    }
    else {
        fShowFitsHits = false;
    }

    CaptLog("Handle the fit information");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    // Get a TList of all of the selected entries.
    TList selected;
    CP::TEventDisplay::Get().GUI().GetResultsList()
        ->GetSelectedEntries(&selected);

    // Iterate through the list of selected entries.
    TIter next(&selected);
    TGLBEntry* lbEntry;
    int index = 0;
    while ((lbEntry = (TGLBEntry*) next())) {
        std::string objName(lbEntry->GetTitle());
        CP::THandle<CP::TReconObjectContainer> objects 
            = event->Get<CP::TReconObjectContainer>(objName.c_str());
        index = ShowReconObjects(objects,index);
    }

}

int CP::TFitChangeHandler::ShowReconCluster(
    CP::THandle<CP::TReconCluster> obj,
    int index) {
    if (!obj) return index;

    double minEnergy = 0.18*unit::MeV/unit::mm;
    double maxEnergy = 3.0*unit::MeV/unit::mm;

    CP::THandle<CP::TClusterState> state = obj->GetState();
    if (!state) {
        CaptError("TClusterState missing!");
        return index;
    }
    TLorentzVector var = state->GetPositionVariance();
    TLorentzVector pos = state->GetPosition();

    // Increment the index to get a new value for the names.
    ++index;

    CaptNamedInfo("cluster","Cluster(" << index << ") @ " 
            << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
            << ", " << unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
            << ", " << unit::AsString(pos.Z(),std::sqrt(var.Z()),"length"));

    CP::TCaptLog::IncreaseIndentation();
    
    CaptNamedInfo("cluster","Time " 
                  << unit::AsString(pos.T(),std::sqrt(var.T()),"time"));

    // Find the orientation of the cluster from the eigen vectors.
    TMatrixD eigenDirs(3,3);

    TVector3 longAxis = obj->GetLongAxis();
    double length = longAxis.Mag();
    double longExtent = obj->GetLongExtent();

    longAxis = longAxis.Unit();
    eigenDirs(0,2) = longAxis.X();
    eigenDirs(1,2) = longAxis.Y();
    eigenDirs(2,2) = longAxis.Z();

    TVector3 majorAxis = obj->GetMajorAxis();
    double major = majorAxis.Mag();
    majorAxis = majorAxis.Unit();
    eigenDirs(0,0) = majorAxis.X();
    eigenDirs(1,0) = majorAxis.Y();
    eigenDirs(2,0) = majorAxis.Z();

    TVector3 minorAxis = obj->GetMinorAxis();
    double minor = minorAxis.Mag();
    minorAxis = minorAxis.Unit();
    eigenDirs(0,1) = minorAxis.X();
    eigenDirs(1,1) = minorAxis.Y();
    eigenDirs(2,1) = minorAxis.Z();

    CaptNamedInfo("cluster","Long Axis (" 
                  << unit::AsString(length,-1,"length") << ")"
                  << " (" << unit::AsString(longAxis.X(),-1,"direction")
                  << ", " << unit::AsString(longAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(longAxis.Z(),-1,"direction") 
                  << ")"
                  << "  Extent: " << unit::AsString(longExtent,-1,"length"));

    CaptNamedInfo("cluster","Major     (" 
                  << unit::AsString(major,-1,"length") << ")"
                  << " (" << unit::AsString(majorAxis.X(),-1,"direction")
                  << ", " << unit::AsString(majorAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(majorAxis.Z(),-1,"direction") 
                  << ")");

    CaptNamedInfo("cluster","Minor     ("
                  << unit::AsString(minor,-1,"length") << ")"
                  << " (" << unit::AsString(minorAxis.X(),-1,"direction")
                  << ", " << unit::AsString(minorAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(minorAxis.Z(),-1,"direction") 
                  << ")");
    
    CP::TCaptLog::DecreaseIndentation();

    TEveGeoShape *clusterShape = new TEveGeoShape("cluster");

    std::ostringstream objName;
    objName << "cluster(" << index << ")";
    clusterShape->SetName(objName.str().c_str());

    // Build the cluster title.
    std::ostringstream title;
    title << "Cluster(" << index << ") @ ";
    title << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
          << ", " << unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
          << ", " << unit::AsString(pos.Z(),std::sqrt(var.Z()),"length");
    title << std::endl
          << "  Long Axis: " << unit::AsString(length,-1,"length")
          << "  Major Axis: " << unit::AsString(major,-1,"length")
          << "  Minor Axis: " << unit::AsString(minor,-1,"length");

    int color = kCyan-9;
    // EDeposit is in measured charge.
    double energy = obj->GetEDeposit();
    // A rough conversion to energy.
    energy *= fEnergyPerCharge*unit::eV/unit::eplus; 
    double dEdX = energy;
    if (longExtent > 1*unit::mm) {
        dEdX /= 2.0*longExtent;              // Get charge per length;
        color = TEventDisplay::Get().LogColor(dEdX,
                                              minEnergy,maxEnergy,2.0);
    }
    title << std::endl
          << "  Energy Deposit: " << unit::AsString(energy,-1,"energy")
          << "  dEdX (per cm) " << unit::AsString(dEdX*unit::cm,-1,"energy");
        
    clusterShape->SetTitle(title.str().c_str());
    clusterShape->SetMainColor(color);
    bool transparentClusters = true;
    if (transparentClusters) clusterShape->SetMainTransparency(60);
    else clusterShape->SetMainTransparency(0);

    // Create the rotation matrix.
    TGeoRotation rot;
    rot.SetMatrix(eigenDirs.GetMatrixArray());

    // Set the translation
    TGeoTranslation trans(obj->GetPosition().X(),
                          obj->GetPosition().Y(),
                          obj->GetPosition().Z());
    
    // Finally set the transform for the object.
    TGeoCombiTrans rotTrans(trans,rot);
    clusterShape->SetTransMatrix(rotTrans);

    // Create the shape to display.  This has to play some fancy footsie to
    // get the gGeoManager memory management right.  It first saves the
    // current manager, then gets an internal geometry manager used by
    // TEveGeoShape, and then resets the old manager once the shape is
    // created.  You gotta love global variables...
    TGeoManager* saveGeom = gGeoManager;
    gGeoManager = clusterShape->GetGeoMangeur();
    TGeoShape* geoShape = new TGeoEltu(major,minor,std::max(length,longExtent));
    clusterShape->SetShape(geoShape);
    gGeoManager = saveGeom;

    fFitList->AddElement(clusterShape);
    
    if (fShowFitsHits) {
        // Draw the hits.
        CP::TShowDriftHits showDrift;
        showDrift(fHitList, *(obj->GetHits()), obj->GetPosition().T());
    }

    return index;
}

int CP::TFitChangeHandler::ShowReconShower(
    CP::THandle<CP::TReconShower> obj,
    int index) {
    if (!obj) return index;

    CP::THandle<CP::TShowerState> state = obj->GetState();
    if (!state) {
        CaptError("TShowerState missing!");
        return index;
    }
    TLorentzVector pos = state->GetPosition();
    TLorentzVector var = state->GetPositionVariance();
    TVector3 dir = state->GetDirection().Unit();
    TVector3 dvar = state->GetDirectionVariance();

    // Get a new index
    ++index;

    // This is used as the annotation, so it needs to be better.
    std::ostringstream title;
    title << "Shower(" << index << ") --" 
          << " Nodes: " << obj->GetNodes().size()
          << ",  Energy Deposit: " << obj->GetEDeposit()
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
          << "   Algorithm: " << obj->GetAlgorithmName()
          << " w/ goodness: " << obj->GetQuality()
          << "/" << obj->GetNDOF();

    CaptNamedLog("shower",title.str());

    CP::TReconNodeContainer& nodes = obj->GetNodes();
    CaptNamedInfo("nodes", "Shower Nodes " << nodes.size());
    CP::TCaptLog::IncreaseIndentation();
    
    TEveElementList *eveShower = new TEveElementList();
    eveShower->SetMainColor(kRed);
    std::ostringstream objName;
    objName << obj->GetName() << "(" << index << ")";
    eveShower->SetName(objName.str().c_str());
    eveShower->SetTitle(title.str().c_str());

    TEveLine* showerLine = new TEveLine(nodes.size());
    showerLine->SetName(objName.str().c_str()); 
    showerLine->SetTitle(title.str().c_str());
    showerLine->SetLineColor(kRed);
    showerLine->SetLineStyle(1);
    showerLine->SetLineWidth(2);

    int p=0;

    // Start at the front state of the shower
    if (state) {
        TLorentzVector frontPos = state->GetPosition();
        TLorentzVector frontVar = state->GetPositionVariance();
        showerLine->SetPoint(p++, frontPos.X(), frontPos.Y(), frontPos.Z());
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
                      << unit::AsString(state->GetDirection()));
        CP::TCaptLog::DecreaseIndentation();
    }

    // Draw the line down the center of the shower.
    for (CP::TReconNodeContainer::iterator n = nodes.begin();
         n != nodes.end(); ++n) {
        CP::THandle<CP::TShowerState> nodeState = (*n)->GetState();
        if (!nodeState) continue;
        TLorentzVector nodePos = nodeState->GetPosition();
        showerLine->SetPoint(p++, nodePos.X(), nodePos.Y(), nodePos.Z());
    }

    eveShower->AddElement(showerLine);

    // Draw spheres at the nodes.
    for (CP::TReconNodeContainer::iterator n = nodes.begin();
         n != nodes.end(); ++n) {
        CP::THandle<CP::TShowerState> nodeState = (*n)->GetState();
        if (!nodeState) continue;
        TLorentzVector nodePos = nodeState->GetPosition();
        double nodeWidth = nodeState->GetCone();
        TEveGeoShape *nodeShape = new TEveGeoShape("showerNode");
        nodeShape->SetName(objName.str().c_str());
        nodeShape->SetTitle(title.str().c_str());
        nodeShape->SetMainColor(kRed);
        // Set the translation
        TGeoTranslation trans(nodeState->GetPosition().X(),
                              nodeState->GetPosition().Y(),
                              nodeState->GetPosition().Z());
        nodeShape->SetTransMatrix(trans);
        TGeoManager* saveGeom = gGeoManager;
        gGeoManager = nodeShape->GetGeoMangeur();
        TGeoShape* geoShape = new TGeoSphere(0.0, nodeWidth);
        nodeShape->SetShape(geoShape);
        gGeoManager = saveGeom;
        eveShower->AddElement(nodeShape);
    }

    fFitList->AddElement(eveShower);

    CP::TCaptLog::DecreaseIndentation();

    return index;
}

int CP::TFitChangeHandler::ShowReconTrack(
    CP::THandle<CP::TReconTrack> obj,
    int index) {
    if (!obj) return index;

    CP::THandle<CP::TTrackState> state = obj->GetState();
    if (!state) {
        CaptError("TTrackState missing!");
        return index;
    }
    TLorentzVector pos = state->GetPosition();
    TLorentzVector var = state->GetPositionVariance();
    TVector3 dir = state->GetDirection().Unit();
    TVector3 dvar = state->GetDirectionVariance();

    // Get a new index
    ++index;

    // This is used as the annotation, so it needs to be better.
    std::ostringstream title;
    title << "Track(" << index << ") --" 
          << " Nodes: " << obj->GetNodes().size()
          << ",  Energy Deposit: " << obj->GetEDeposit()
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
          << "   Algorithm: " << obj->GetAlgorithmName()
          << " w/ goodness: " << obj->GetQuality()
          << "/" << obj->GetNDOF();

    CP::THandle<CP::TTrackState> backState = obj->GetBack();
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

    CP::TReconNodeContainer& nodes = obj->GetNodes();
    CaptNamedInfo("nodes", "Track Nodes " << nodes.size());
    CP::TCaptLog::IncreaseIndentation();
    
    TEveLine* eveTrack = new TEveLine(nodes.size());

    std::ostringstream objName;
    objName << obj->GetName() << "(" << index << ")";
    eveTrack->SetName(objName.str().c_str()); 

    eveTrack->SetTitle(title.str().c_str());
    eveTrack->SetLineColor(kBlue);
    eveTrack->SetLineStyle(1);
    eveTrack->SetLineWidth(2);

    int p=0;

    // Start at the front state of the track
    if (state) {
        TLorentzVector frontPos = state->GetPosition();
        TLorentzVector frontVar = state->GetPositionVariance();
        eveTrack->SetPoint(p++, frontPos.X(), frontPos.Y(), frontPos.Z());
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
                      << unit::AsString(state->GetDirection()));
        CP::TCaptLog::DecreaseIndentation();
    }

    for (CP::TReconNodeContainer::iterator n = nodes.begin();
         n != nodes.end(); ++n) {
        CP::THandle<CP::TTrackState> nodeState = (*n)->GetState();
        CP::THandle<CP::TReconBase> nodeObject = (*n)->GetObject();
        if (!nodeState) continue;
        TLorentzVector nodePos = nodeState->GetPosition();
        TLorentzVector nodeVar = nodeState->GetPositionVariance();
        eveTrack->SetPoint(p++, nodePos.X(), nodePos.Y(), nodePos.Z());
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

    // finish at the back state of the track
    if (backState) {
        TLorentzVector backPos = backState->GetPosition();
        TLorentzVector backVar = backState->GetPositionVariance();
        eveTrack->SetPoint(p++, backPos.X(), backPos.Y(), backPos.Z());
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

    fFitList->AddElement(eveTrack);

    if (fShowFitsHits) {
        // Draw the hits.
        CP::TShowDriftHits showDrift;
        showDrift(fHitList, *(obj->GetHits()), obj->GetPosition().T());
    }

    CP::TCaptLog::DecreaseIndentation();
    return index;
}

int CP::TFitChangeHandler::ShowReconPID(
    CP::THandle<CP::TReconPID> obj, 
    int index) {
    if (!obj) return index;
    CaptError("ShowReconPID not Implemented");
    return index;
}

int CP::TFitChangeHandler::ShowReconVertex(
    CP::THandle<CP::TReconVertex> obj,
    int index) {
    if (!obj) return index;

    CP::THandle<CP::TVertexState> state = obj->GetState();
    if (!state) {
        CaptError("TVertexState missing!");
        return index;
    }
    TLorentzVector pos = state->GetPosition();
    TLorentzVector var = state->GetPositionVariance();

    ++index;

    CaptLog("Vertex(" << index << ") @ " 
            << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
            <<", "<<unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
            <<", "<<unit::AsString(pos.Z(),std::sqrt(var.Z()),"length"));
    
    CP::THandle<CP::TReconObjectContainer> 
        constituents = obj->GetConstituents();
    if (constituents) {
        CP::TCaptLog::IncreaseIndentation();
        CaptLog("Constituent objects: " << constituents->size());
        CP::TCaptLog::IncreaseIndentation();
        index = ShowReconObjects(constituents,index);
        CP::TCaptLog::DecreaseIndentation();
        CP::TCaptLog::DecreaseIndentation();
    }

    return index;
}

int CP::TFitChangeHandler::ShowReconObjects(
    CP::THandle<CP::TReconObjectContainer> objects,
    int index) {
    if (!objects) return index;
    CaptLog("Show " << objects->size() << " objects in " << objects->GetName());
    CP::TCaptLog::IncreaseIndentation();
    for (CP::TReconObjectContainer::iterator obj = objects->begin();
         obj != objects->end(); ++obj) {
        CP::THandle<CP::TReconCluster> cluster = *obj;
        if (cluster) {
            index = ShowReconCluster(cluster, index);
            continue;
        }
        CP::THandle<CP::TReconShower> shower = *obj;
        if (shower) {
            index = ShowReconShower(shower, index);
            continue;
        }
        CP::THandle<CP::TReconTrack> track = *obj;
        if (track) {
            index = ShowReconTrack(track, index);
            continue;
        }
        CP::THandle<CP::TReconPID> pid = *obj;
        if (pid) {
            index = ShowReconPID(pid, index);
            continue;
        }
        CP::THandle<CP::TReconVertex> vertex = *obj;
        if (vertex) {
            index = ShowReconVertex(vertex, index);
            continue;
        }

    }
    CP::TCaptLog::DecreaseIndentation();
    return index;
}
