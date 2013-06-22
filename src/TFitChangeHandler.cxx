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
#include <TGeoShape.h>
#include <TGeoEltu.h>
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
    while ((lbEntry = (TGLBEntry*) next())) {
        std::string objName(lbEntry->GetTitle());
        CP::THandle<CP::TReconObjectContainer> objects 
            = event->Get<CP::TReconObjectContainer>(objName.c_str());
        ShowReconObjects(objects);
    }

}

void CP::TFitChangeHandler::ShowReconCluster(
    CP::THandle<CP::TReconCluster> obj) {
    if (!obj) return;
    CaptLog("Cluster " << obj->GetName());

    TEveGeoShape *clusterShape = new TEveGeoShape("cluster");
    
    clusterShape->SetMainColor(kRed);
    clusterShape->SetMainTransparency(80);

    // Find the orientation of the cluster from the eigen vectors.
    TVectorF eigenValues;
    TMatrixF eigenVectors(obj->GetMoments().EigenVectors(eigenValues));
    TMatrixD eigenDirs(eigenVectors);

    // Long axis goes on Z
    eigenDirs(0,2) = eigenVectors(0,0);
    eigenDirs(1,2) = eigenVectors(1,0);
    eigenDirs(2,2) = eigenVectors(2,0);
    double len = std::sqrt(eigenValues(0));

    // Major axis goes on X
    eigenDirs(0,0) = eigenVectors(0,1);
    eigenDirs(1,0) = eigenVectors(1,1);
    eigenDirs(2,0) = eigenVectors(2,1);
    double major = std::sqrt(eigenValues(1));

    // Minor axis goes on Y
    eigenDirs(0,1) = eigenVectors(0,2);
    eigenDirs(1,1) = eigenVectors(1,2);
    eigenDirs(2,1) = eigenVectors(2,2);
    double minor = std::sqrt(eigenValues(2));
        
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
    TGeoShape* geoShape = new TGeoEltu(major,minor,len);
    clusterShape->SetShape(geoShape);
    gGeoManager = saveGeom;
   
    fFitList->AddElement(clusterShape);
    
    // Draw the hits.
    CP::TShowDriftHits showDrift;
    showDrift(fHitList, *(obj->GetHits()), obj->GetPosition().T());
}

void CP::TFitChangeHandler::ShowReconShower(
    CP::THandle<CP::TReconShower> obj) {
    if (!obj) return;
}

void CP::TFitChangeHandler::ShowReconTrack(
    CP::THandle<CP::TReconTrack> obj) {
    if (!obj) return;
}

void CP::TFitChangeHandler::ShowReconPID(
    CP::THandle<CP::TReconPID> obj) {
    if (!obj) return;
}

void CP::TFitChangeHandler::ShowReconVertex(
    CP::THandle<CP::TReconVertex> obj) {
    if (!obj) return;
}

void CP::TFitChangeHandler::ShowReconObjects(
    CP::THandle<CP::TReconObjectContainer> objects) {
    if (!objects) return;
    CaptLog("Show Objects in " << objects->GetName());
    CP::TCaptLog::IncreaseIndentation();
    for (CP::TReconObjectContainer::iterator obj = objects->begin();
         obj != objects->end(); ++obj) {
        CP::THandle<CP::TReconCluster> cluster = *obj;
        if (cluster) {
            ShowReconCluster(cluster);
            continue;
        }
        CP::THandle<CP::TReconShower> shower = *obj;
        if (shower) {
            ShowReconShower(shower);
            continue;
        }
        CP::THandle<CP::TReconTrack> track = *obj;
        if (track) {
            ShowReconTrack(track);
            continue;
        }
        CP::THandle<CP::TReconPID> pid = *obj;
        if (pid) {
            ShowReconPID(pid);
            continue;
        }
        CP::THandle<CP::TReconVertex> vertex = *obj;
        if (vertex) {
            ShowReconVertex(vertex);
            continue;
        }

    }
    CP::TCaptLog::DecreaseIndentation();
}
