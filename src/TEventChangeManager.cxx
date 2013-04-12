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
            TGeoShape* clonedShape 
                = dynamic_cast<TGeoShape*> (currShape->Clone("fakeShape"));

            fakeDrift->SetShape(clonedShape);
            fakeDrift->SetTransMatrix(*currMat);

            fakeDrift->SetMainColor(kCyan);
            fakeDrift->SetMainTransparency(80);

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

CP::TEventChangeManager::~TEventChangeManager() {
}

void CP::TEventChangeManager::SetEventSource(CP::TVInputFile* source) {
    if (!source) {
        CaptError("Invalid event source");
        return;
    }
    fEventSource = source;
    fEventSource->FirstEvent();
    EventChanged();
}

void CP::TEventChangeManager::AddHandler(CP::TVEventChangeHandler* handler) {
    fHandlers.push_back(handler);
}

void CP::TEventChangeManager::ChangeEvent(int change) {
    if (!GetEventSource()) {
        CaptError("Event source is not available");
        EventChanged();
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

    EventChanged();
}

void CP::TEventChangeManager::EventChanged() {
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    if (!event) {
        CaptError("Invalid event");
        return;
    }

    CaptLog("Event: " << event->GetContext());

    // Make sure that the event geometry is updated.
    CP::TManager::Get().Geometry();
    
    // Run through all of the handlers.
    for (Handlers::iterator h = fHandlers.begin(); h != fHandlers.end(); ++h) {
        (*h)->Apply();
    }

    // Make sure EVE is up to date.
    gEve->Redraw3D(kTRUE);
}
