#include "TGUIManager.hxx"
#include "TEventDisplay.hxx"

#include <TGFrame.h>
#include <TGButton.h>

#include <TEveManager.h>
#include <TEveBrowser.h>

#include <TSystem.h>

CP::TGUIManager::TGUIManager() {
    TEveBrowser* browser = gEve->GetBrowser();

    // Define the generic layout.  The last four parameters are the padding
    // around the widgets.
    TGLayoutHints* layoutHints 
        = new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,
                            2, 2, 2, 2);


    // Imbed the new frame in the event browser
    browser->StartEmbedding(TRootBrowser::kLeft);
    
    // This is embedded.
    TGMainFrame* mainFrame = new TGMainFrame(gClient->GetRoot(), 1000, 600);
    mainFrame->SetWindowName("XX GUI");
    mainFrame->SetCleanup(kDeepCleanup);

    TGVerticalFrame* hf = new TGVerticalFrame(mainFrame);
    TGTextButton* textButton;

    // The general action buttons.
    textButton = new TGTextButton(hf, "Previous Event");
    textButton->SetToolTipText("Go to previous event.");
    textButton->SetTextJustify(36);
    textButton->SetMargins(0,0,0,0);
    textButton->SetWrapLength(-1);
    hf->AddFrame(textButton, layoutHints);
    fPrevEventButton = textButton;

    textButton = new TGTextButton(hf, "Redraw Event");
    textButton->SetToolTipText("Refresh the current view.");
    textButton->SetTextJustify(36);
    textButton->SetMargins(0,0,0,0);
    textButton->SetWrapLength(-1);
    hf->AddFrame(textButton, layoutHints);
    fDrawEventButton = textButton;

    textButton = new TGTextButton(hf, "Next Event");
    textButton->SetToolTipText("Go to previous event.");
    textButton->SetTextJustify(36);
    textButton->SetMargins(0,0,0,0);
    textButton->SetWrapLength(-1);
    hf->AddFrame(textButton, layoutHints);
    fNextEventButton = textButton;

    // Create the buttons to select which types of objects are showed.
    TGCheckButton *checkButton;
    checkButton = new TGCheckButton(hf,"Show Trajectories");
    checkButton->SetToolTipText(
        "Show the GEANT4 trajectories and trajectory points.");
    checkButton->SetTextJustify(36);
    checkButton->SetMargins(0,0,0,0);
    checkButton->SetWrapLength(-1);
    hf->AddFrame(checkButton, layoutHints);
    fShowTrajectoriesButton = checkButton;

    checkButton = new TGCheckButton(hf,"Show G4 Hits");
    checkButton->SetToolTipText(
        "Show the GEANT4 hits.  This shows the energy deposition in the "
        "defined sensitive detectors.");
    checkButton->SetTextJustify(36);
    checkButton->SetMargins(0,0,0,0);
    checkButton->SetWrapLength(-1);
    hf->AddFrame(checkButton, layoutHints);
    fShowG4HitsButton = checkButton;

    // Do the final layout and mapping.
    mainFrame->AddFrame(hf, layoutHints);
    mainFrame->MapSubwindows();
    mainFrame->Resize();
    mainFrame->MapWindow();
    browser->StopEmbedding();
    browser->SetTabTitle("Event Control", 0);
}

CP::TGUIManager::~TGUIManager() { }
