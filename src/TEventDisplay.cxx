#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TEventChangeManager.hxx"

#include <TCaptLog.hxx>

#include <TEveManager.h>

CP::TEventDisplay* CP::TEventDisplay::fEventDisplay = NULL;

CP::TEventDisplay& CP::TEventDisplay::Get(void) {
    if (!fEventDisplay) {
        fEventDisplay = new CP::TEventDisplay();
        fEventDisplay->Init();
    }
    return *fEventDisplay;
}

CP::TEventDisplay::TEventDisplay() {}

void CP::TEventDisplay::Init() {
    TEveManager::Create();

    // This is accessed through the GUI() method.
    fGUIManager = new TGUIManager();

    // Create the event display manager.  This needs the GUI, so it has to be
    // done after TGUIManager is created.
    fEventChangeManager = new TEventChangeManager();

    CaptLog("Event display constructed");
}

CP::TEventDisplay::~TEventDisplay() {
    CaptLog("Event display deconstructed");
}
