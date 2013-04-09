#include "TEventChangeManager.hxx"
#include "TGUIManager.hxx"
#include "TEventDisplay.hxx"

#include <TEvent.hxx>
#include <TEventFolder.hxx>

#include <TQObject.h>
#include <TGButton.h>

#include <iostream>

ClassImp(CP::TEventChangeManager);

CP::TEventChangeManager::TEventChangeManager() {
    TGButton* button = CP::TEventDisplay::Get().GUI().GetNextEventButton();
    if (button) {
        button->Connect("Clicked()",
                        "CP::TEventChangeManager", 
                        this,
                        "ChangeEvent(=1)");
    }

    button = CP::TEventDisplay::Get().GUI().GetPrevEventButton();
    if (button) {
        button->Connect("Clicked()",
                        "CP::TEventChangeManager", 
                        this,
                        "ChangeEvent(=-1)");
    }

}

CP::TEventChangeManager::~TEventChangeManager() {
}

void CP::TEventChangeManager::ChangeEvent(int change) {
    std::cout << "Change Event " << change << std::endl;
    if (!GetEventSource()) {
        CaptLog("Event source is not available");
        EventChanged();
        return;
    }
    CP::TEvent* currentEvent = CP::TEventFolder::GetCurrentEvent();
    CP::TEvent* nextEvent = NULL;
    if (change > 0) {
        nextEvent = GetEventSource()->NextEvent(change-1);
        if (!nextEvent) nextEvent = GetEventSource()->PreviousEvent();
    }
    if (change < 0) {
        nextEvent = GetEventSource()->PreviousEvent(change+1);
        if (!nextEvent) nextEvent = GetEventSource()->NextEvent();
    }
    if (currentEvent) delete currentEvent;

    currentEvent = CP::TEventFolder::GetCurrentEvent();
    CaptLog("New Event: " << currentEvent->GetContext());
    
    EventChanged();
}

void CP::TEventChangeManager::EventChanged() {
}
