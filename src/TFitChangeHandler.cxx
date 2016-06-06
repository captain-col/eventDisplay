#include "TFitChangeHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TShowDriftHits.hxx"
#include "TMatrixElement.hxx"
#include "TReconTrackElement.hxx"
#include "TReconShowerElement.hxx"
#include "TReconClusterElement.hxx"

#include <TCaptLog.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>
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
#include <TGLViewer.h>
#include <TGLCamera.h>

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
    fShowFitsHits = true;
    fShowFitsObjects = true;
}

CP::TFitChangeHandler::~TFitChangeHandler() {
}

void CP::TFitChangeHandler::Apply() {

    fHitList->DestroyElements();
    fFitList->DestroyElements();
    fCameraWeight = 0.0;
    
    if (!CP::TEventDisplay::Get().GUI().GetShowFitsButton()->IsOn()
        && !CP::TEventDisplay::Get().GUI().GetShowFitsHitsButton()->IsOn()) {
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

    if (CP::TEventDisplay::Get().GUI().GetShowFitsButton()->IsOn()) {
        CaptLog("Showing Fit Objecs");
        fShowFitsObjects = true;
    }
    else {
        fShowFitsObjects = false;
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
        index = ShowReconObjects(fFitList,objects,index);
    }

    if (fCameraWeight > 1 
        && CP::TEventDisplay::Get().GUI().GetRecalculateViewButton()->IsOn()) {
        TGLViewer* glViewer = gEve->GetDefaultGLViewer();
        fCameraCenter *= 1.0/fCameraWeight;
        glViewer->SetDrawCameraCenter(kTRUE);
        glViewer->CurrentCamera().SetExternalCenter(kTRUE);
        glViewer->CurrentCamera().SetCenterVecWarp(fCameraCenter.X(),
                                                   fCameraCenter.Y(),
                                                   fCameraCenter.Z());
    }

}

int CP::TFitChangeHandler::ShowReconCluster(
    TEveElementList* list,
    CP::THandle<CP::TReconCluster> obj,
    int index,
    bool forceUncertainty) {
    if (!obj) return index;

    CP::THandle<CP::TClusterState> state = obj->GetState();
    if (!state) {
        CaptError("TClusterState missing!");
        return index;
    }

    // Increment the index to get a new value for the names.
    ++index;

    if (CP::TEventDisplay::Get().GUI()
        .GetShowClusterUncertaintyButton()->IsOn()) {
        forceUncertainty = true;
    }

    CP::TReconClusterElement *eveCluster
        = new CP::TReconClusterElement(*obj,forceUncertainty);

    list->AddElement(eveCluster);
    
    if (CP::TEventDisplay::Get().GUI()
        .GetShowClusterHitsButton()->IsOn()) {
        // Draw the hits.
        CP::TShowDriftHits showDrift;
        showDrift(eveCluster, *(obj->GetHits()), obj->GetPosition().T());
    }

    return index;
}

int CP::TFitChangeHandler::ShowReconShower(
    TEveElementList* list,
    CP::THandle<CP::TReconShower> obj,
    int index) {
    if (!obj) return index;

    CP::THandle<CP::TShowerState> state = obj->GetState();
    if (!state) {
        CaptError("TShowerState missing!");
        return index;
    }

    // Get a new index
    ++index;

    CP::TReconShowerElement *eveShower = new CP::TReconShowerElement(*obj,true);

    list->AddElement(eveShower);

    // Draw the clusters.
    if (CP::TEventDisplay::Get().GUI()
        .GetShowConstituentClustersButton()->IsOn()) {
        for (CP::TReconNodeContainer::iterator n = obj->GetNodes().begin();
             n != obj->GetNodes().end(); ++n) {
            index = ShowReconObject(eveShower,(*n)->GetObject(),index, false);
        }
    }

    CP::TCaptLog::DecreaseIndentation();

    return index;
}

int CP::TFitChangeHandler::ShowReconTrack(
    TEveElementList* list,
    CP::THandle<CP::TReconTrack> obj,
    int index) {
    if (!obj) return index;
    CP::THandle<CP::TTrackState> frontState = obj->GetState();
    if (!frontState) {
        CaptError("TTrackState missing!");
        return index;
    }

    // Get a new index
    ++index;

    TReconTrackElement *eveTrack = new TReconTrackElement(*obj,true);
    list->AddElement(eveTrack);

    // Draw the clusters.
    if (CP::TEventDisplay::Get().GUI()
        .GetShowConstituentClustersButton()->IsOn()) {
        for (CP::TReconNodeContainer::iterator n = obj->GetNodes().begin();
             n != obj->GetNodes().end(); ++n) {
            index = ShowReconObject(eveTrack,(*n)->GetObject(),index, true);
        }
    }

    return index;
}

int CP::TFitChangeHandler::ShowReconPID(
    TEveElementList* list,
    CP::THandle<CP::TReconPID> obj, 
    int index) {
    if (!obj) return index;
    CaptError("ShowReconPID not Implemented");
    return index;
}

int CP::TFitChangeHandler::ShowReconVertex(
    TEveElementList* list,
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

    CaptLog("Vertex(" << obj->GetUniqueID() << ") @ " 
            << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
            <<", "<<unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
            <<", "<<unit::AsString(pos.Z(),std::sqrt(var.Z()),"length"));
    
    CP::THandle<CP::TReconObjectContainer> 
        constituents = obj->GetConstituents();
    if (constituents) {
        CP::TCaptLog::IncreaseIndentation();
        CaptLog("Constituent objects: " << constituents->size());
        CP::TCaptLog::IncreaseIndentation();
        index = ShowReconObjects(list,constituents,index);
        CP::TCaptLog::DecreaseIndentation();
        CP::TCaptLog::DecreaseIndentation();
    }

    return index;
}

int CP::TFitChangeHandler::ShowReconObject(TEveElementList* list,
                                           CP::THandle<CP::TReconBase> obj,
                                           int index,
                                           bool forceUncertainty) {
    if (!obj) return index;
    // Add this object to the estimated center.
    CP::THandle<CP::THitSelection> hits = obj->GetHits();
    if (hits) {
        for (CP::THitSelection::iterator h = hits->begin();
             h != hits->end(); ++h) {
            fCameraCenter += (*h)->GetCharge()*(*h)->GetPosition();
            fCameraWeight += (*h)->GetCharge();
        }
    }
    CP::THandle<CP::TReconVertex> vertex = obj;
    if (vertex) {
        index = ShowReconVertex(list, vertex, index);
        return index;
    }
    if (!fShowFitsObjects) return index;
    CP::THandle<CP::TReconCluster> cluster = obj;
    if (cluster) {
        index = ShowReconCluster(
            list, cluster, index, forceUncertainty);
        return index;
    }
    CP::THandle<CP::TReconShower> shower = obj;
    if (shower) {
        index = ShowReconShower(list, shower, index);
        return index;
    }
    CP::THandle<CP::TReconTrack> track = obj;
    if (track) {
        index = ShowReconTrack(list, track, index);
        return index;
    }
    CP::THandle<CP::TReconPID> pid = obj;
    if (pid) {
        index = ShowReconPID(list, pid, index);
        return index;
    }
    return index;
}

int CP::TFitChangeHandler::ShowReconObjects(
    TEveElementList* list,
    CP::THandle<CP::TReconObjectContainer> objects,
    int index) {
    if (!objects) return index;
    CaptLog("Show " << objects->size() << " objects in " << objects->GetName());
    CP::TCaptLog::IncreaseIndentation();
    for (CP::TReconObjectContainer::iterator obj = objects->begin();
         obj != objects->end(); ++obj) {
        index = ShowReconObject(list,*obj, index, false);
        if (fShowFitsHits) {
            // Draw the hits.
            CP::TShowDriftHits showDrift;
            showDrift(fHitList, *((*obj)->GetHits()),0.0);
        }
    }
    CP::TCaptLog::DecreaseIndentation();
    return index;
}
