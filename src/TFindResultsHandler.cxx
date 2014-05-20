#include "TFindResultsHandler.hxx"
#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"

#include <TCaptLog.hxx>
#include <TEvent.hxx>
#include <TEventFolder.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>

#include <TGeoManager.h>
#include <TGButton.h>

#include <TEveManager.h>
#include <TEveLine.h>

#include <TPRegexp.h>

#include <sstream>

CP::TFindResultsHandler::TFindResultsHandler() {
}

CP::TFindResultsHandler::~TFindResultsHandler() {
}

void CP::TFindResultsHandler::Apply() {
    CaptError("Find the results");
    CP::TEvent* event = CP::TEventFolder::GetCurrentEvent();
    if (!event) return;

    TGTextEntry* defResult = CP::TEventDisplay::Get().GUI().GetDefaultResult();
    TGListBox* resultsList = CP::TEventDisplay::Get().GUI().GetResultsList();
    
    std::string defaultResult(defResult->GetText());
    TPRegexp regularExp(defResult->GetText());

    resultsList->RemoveAll();
    int id = 0;
    // Forage the results...
    std::vector<CP::TDatum*> stack;
    std::vector<std::string> existingEntries;
    stack.push_back(event);
    while (!stack.empty()) {
        CP::TDatum* current = stack.back();
        stack.pop_back();
        CP::TReconObjectContainer* rc 
            = dynamic_cast<CP::TReconObjectContainer*>(current);
        if (rc) {
            std::string fullName(rc->GetFullName());
            if (std::find(existingEntries.begin(),existingEntries.end(),
                          fullName) != existingEntries.end()) continue;
            existingEntries.push_back(fullName);
            resultsList->AddEntry(rc->GetFullName(),++id);
            // Check to see if this result should be selected
            if (defaultResult.size() == 0) continue;
            if (!regularExp.Match(fullName.c_str())) continue;
            resultsList->Select(id);
            continue;
        }
        CP::TDataVector* dv = dynamic_cast<CP::TDataVector*>(current);
        if (dv) {
            for (CP::TDataVector::iterator d = dv->begin();
                 d != dv->end();
                 ++d) {
                stack.push_back(*d);
            }
        }
    }
    resultsList->Layout();
    resultsList->MapSubwindows();
}
