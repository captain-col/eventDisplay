#include "TEventChangeManager.hxx"
#include "TVEventChangeHandler.hxx"
#include "TGUIManager.hxx"
#include "TEventDisplay.hxx"

#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <CaptGeomId.hxx>
#include <TManager.hxx>
#include <TGeomIdManager.hxx>

#include <TQObject.h>
#include <TGButton.h>
#include <TGeoManager.h>
#include <TGeoPgon.h>
#include <TEveGeoShape.h>
#include <TEveManager.h>

#include <iostream>

ClassImp(CP::TEventChangeManager);

namespace {

    /// This is called when a new geometry is loaded.  The implementation is
    /// sloppy since it assumes it's only going to be called once.  That's
    /// actually a reasonable assumption for the usual use case, but it should
    /// probably be fixed.  The problem is that the TEveElementList is created
    /// everytime this class is used.
    class GeometryChangeCallback: public CP::TManager::GeometryChange {
    public:
        void Callback(const CP::TEvent* const event) {
            CaptError("New geometry loaded " << gGeoManager);
            // Use geometry identifiers to change to the drift region.
            CP::TGeometryId id = CP::GeomId::Captain::Drift();
            CP::TManager::Get().GeomId().CdId(id); 
            TGeoNode* current = gGeoManager->GetCurrentNode();
            TGeoMatrix* currMat = gGeoManager->GetCurrentMatrix();
            TGeoShape* currShape = current->GetVolume()->GetShape();

            TEveElementList* fakeGeometry = new TEveElementList("fakeGeometry");

            TEveGeoShape *fakeDrift = new TEveGeoShape("fakeDrift");
            
            fakeDrift->SetMainColor(kCyan);
            fakeDrift->SetMainTransparency(80);

            TGeoMatrix* mat = currMat->MakeClone();
            fakeDrift->SetTransMatrix(*mat);

            // Clone the shape for the drift region so that it can be
            // displayed.  This has to play some fancy footsie to get the
            // gGeoManager memory management right.  It first saves the
            // current manager, then gets an internal geometry manager used by
            // TEveGeoShape, and then resets the old manager once the shape is
            // created.
            TGeoManager* saveGeom = gGeoManager;
            gGeoManager = fakeDrift->GetGeoMangeur();
            TGeoShape* clonedShape 
                = dynamic_cast<TGeoShape*> (currShape->Clone("fakeShape"));
            fakeDrift->SetShape(clonedShape);
            gGeoManager = saveGeom;

            fakeGeometry->AddElement(fakeDrift);

            gEve->AddGlobalElement(fakeGeometry);
        }
    };
};


CP::TEventChangeManager::TEventChangeManager() {
    TGButton* button = CP::TEventDisplay::Get().GUI().GetNextEventButton();
    if (button) {
        button->Connect("Clicked()",
                        "CP::TEventChangeManager", 
                        this,
                        "ChangeEvent(=1)");
    }

    button = CP::TEventDisplay::Get().GUI().GetDrawEventButton();
    if (button) {
        button->Connect("Clicked()",
                        "CP::TEventChangeManager", 
                        this,
                        "ChangeEvent(=0)");
    }

    button = CP::TEventDisplay::Get().GUI().GetPrevEventButton();
    if (button) {
        button->Connect("Clicked()",
                        "CP::TEventChangeManager", 
                        this,
                        "ChangeEvent(=-1)");
    }

    // Register a geometry change manager to handle when a new geometry
    // becomes available
    CP::TManager::Get().RegisterGeometryCallback(new GeometryChangeCallback);
}

CP::TEventChangeManager::~TEventChangeManager() {}

void CP::TEventChangeManager::SetEventSource(CP::TVInputFile* source) {
    if (!source) {
        CaptError("Invalid event source");
        return;
    }
    fEventSource = source;
    fEventSource->FirstEvent();
    UpdateEvent();
}

void CP::TEventChangeManager::AddHandler(
    CP::TVEventChangeHandler* handler) {
    fHandlers.push_back(handler);
}

void CP::TEventChangeManager::ChangeEvent(int change) {
    CaptError("Change Event by " << change << " entries");
    if (!GetEventSource()) {
        CaptError("Event source is not available");
        UpdateEvent();
        return;
    }
    CP::TEvent* currentEvent = CP::TEventFolder::GetCurrentEvent();
    CP::TEvent* nextEvent = NULL;
    if (change > 0) {
        nextEvent = GetEventSource()->NextEvent(change-1);
        if (!nextEvent) {
            nextEvent = GetEventSource()->PreviousEvent();
        }
        if (nextEvent && currentEvent) delete currentEvent;
    }
    if (change < 0) {
        nextEvent = GetEventSource()->PreviousEvent(change+1);
        if (!nextEvent) nextEvent = GetEventSource()->NextEvent();
        if (nextEvent && currentEvent) delete currentEvent;
    }
    currentEvent = CP::TEventFolder::GetCurrentEvent();
    if (!currentEvent) {
        CaptLog("No Current Event");
        return;
    }

    if (change != 0) NewEvent(); 

    UpdateEvent();
}

void CP::TEventChangeManager::NewEvent() {
    CaptError("New Event");
    return;

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) {
        CaptError("Invalid event");
        return;
    }

    CaptLog("Event: " << event->GetContext());

}

void CP::TEventChangeManager::UpdateEvent() {
    CaptError("Update Event");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    CaptError("Got current event");

    if (!event) {
        CaptError("Invalid event");
        return;
    }

    // Make sure that the event geometry is updated.
    CP::TManager::Get().Geometry();
    CaptError("Got current geometry");
    
    // Run through all of the handlers.
    for (Handlers::iterator h = fHandlers.begin();
         h != fHandlers.end(); ++h) {
        CaptError("Apply Handler");
        (*h)->Apply();
    }

    // Make sure EVE is up to date.
    gEve->Redraw3D(kTRUE);

}
