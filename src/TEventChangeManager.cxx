#include "TEventChangeManager.hxx"
#include "TVEventChangeHandler.hxx"
#include "TGUIManager.hxx"
#include "TEventDisplay.hxx"

#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <CaptGeomId.hxx>
#include <TManager.hxx>
#include <TGeomIdManager.hxx>
#include <TChannelInfo.hxx>

#include <TQObject.h>
#include <TGButton.h>
#include <TGeoManager.h>
#include <TGeoPgon.h>
#include <TEveGeoShape.h>
#include <TEveManager.h>

#include <iostream>

ClassImp(CP::TEventChangeManager);

namespace {

    /// This takes a geometry id and "clones" it into the Eve display.
    TEveGeoShape* GeometryClone(CP::TGeometryId id) {
        std::cout << id << " " << id.GetName() << std::endl;
        if (!CP::TManager::Get().GeomId().CdId(id)) return NULL;

        TGeoNode* current = gGeoManager->GetCurrentNode();
        TGeoMatrix* currMat = gGeoManager->GetCurrentMatrix();
        TGeoShape* currShape = current->GetVolume()->GetShape();

        TEveGeoShape *fakeShape = new TEveGeoShape(id.GetName().c_str());
        
        TGeoMatrix* mat = currMat->MakeClone();
        fakeShape->SetTransMatrix(*mat);
        
        // Clone the shape so that it can be displayed.  This has to play some
        // fancy footsie to get the gGeoManager memory management right.  It
        // first saves the current manager, then gets an internal geometry
        // manager used by TEveGeoShape, and then resets the old manager once
        // the shape is created.
        TGeoManager* saveGeom = gGeoManager;
        gGeoManager = fakeShape->GetGeoMangeur();
        TGeoShape* clonedShape 
            = dynamic_cast<TGeoShape*> (currShape->Clone("fakeShape"));
        fakeShape->SetShape(clonedShape);
        gGeoManager = saveGeom;
        
        return fakeShape;
    }

    /// This is called when a new geometry is loaded.  The implementation is
    /// sloppy since it assumes it's only going to be called once.  That's
    /// actually a reasonable assumption for the usual use case, but it should
    /// probably be fixed.  The problem is that the TEveElementList is created
    /// everytime this class is used.
    class GeometryChangeCallback: public CP::TManager::GeometryChange {
    public:
        void Callback(const CP::TEvent* const event) {
            CaptError("New geometry loaded " << gGeoManager);

            if (!CP::TEventDisplay::Get().EventChange().GetShowGeometry()) {
                return;
            }

            TEveElementList* simple = new TEveElementList("simplifiedGeometry");

            // Add the drift region.
            TEveGeoShape *shape = GeometryClone(CP::GeomId::Captain::Drift());
            shape->SetMainColor(kCyan);
            shape->SetMainTransparency(80);
            simple->AddElement(shape);

            // Add the pmts.  There are up to about 24, but this has a large
            // limit so that it catches any expansions.
            for (int i=0; i<200; ++i) {
                shape = GeometryClone(CP::GeomId::Captain::Photosensor(i));
                if (!shape) break;
                shape->SetMainColor(kYellow);
                simple->AddElement(shape);
            }

            gEve->AddGlobalElement(simple);
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
    NewEvent();
    UpdateEvent();
}

void CP::TEventChangeManager::AddNewEventHandler(
    CP::TVEventChangeHandler* handler) {
    fNewEventHandlers.push_back(handler);
}

void CP::TEventChangeManager::AddUpdateHandler(
    CP::TVEventChangeHandler* handler) {
    fUpdateHandlers.push_back(handler);
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

    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) {
        CaptError("Invalid event");
        return;
    }

    CaptLog("Event: " << event->GetContext());

    // Let the database handlers know about the new event context.
    CP::TChannelInfo::Get().SetContext(event->GetContext());

    // Run through all of the handlers.
    for (Handlers::iterator h = fNewEventHandlers.begin();
         h != fNewEventHandlers.end(); ++h) {
        (*h)->Apply();
    }

}

void CP::TEventChangeManager::UpdateEvent() {
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();

    if (!event) {
        CaptError("Invalid event");
        return;
    }

    // Make sure that the event geometry is updated.
    CP::TManager::Get().Geometry();
    
    // Run through all of the handlers.
    for (Handlers::iterator h = fUpdateHandlers.begin();
         h != fUpdateHandlers.end(); ++h) {
        (*h)->Apply();
    }

    // Make sure EVE is up to date.
    gEve->Redraw3D(kTRUE);
}
