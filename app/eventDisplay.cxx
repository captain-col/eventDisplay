#include "TEventDisplay.hxx"
#include "TEventChangeManager.hxx"

#include <TCaptLog.hxx>
#include <TRootInput.hxx>
#include <TVInputFile.hxx>

#include <TROOT.h>
#include <TSystem.h>
#include <TApplication.h>

#include <getopt.h>

#include <iostream>
#include <memory>

void usage();

int main(int argc, char **argv) {
    std::string fileName = "";
    bool showGeometry = false;
    while (1) {
        int c = getopt(argc, argv, "?hg");
        if (c == -1) break;
        switch (c) {
        case 'g': // Show the geometry.
            showGeometry = not showGeometry;
            break;
        case '?':
        case 'h':
            usage();
            return 1;
        }
    }
    
    CP::TCaptLog::Configure();
        
    // Check if there is an input file on the command line.
    if (argc - optind > 0) {
        fileName = argv[optind];
    }

    CP::TVInputFile* eventSource = NULL;
    if (!fileName.empty()) {
        eventSource = new CP::TRootInput(fileName.c_str());
    }
    if (!eventSource) {
        usage();
        CaptLog("Must provide an input file");
        return 1;
    }

    TApplication theApp("EventDisplay", 0, 0);
    theApp.ExitOnException(TApplication::kExit);

    CP::TEventDisplay& ev = CP::TEventDisplay::Get();
    ev.EventChange().SetShowGeometry(showGeometry);
    ev.EventChange().SetEventSource(eventSource);

    theApp.Run(kFALSE);

    return 0;
}


void usage() {
    std::cout << "Usage: event-display.exe [input-file] " << std::endl;
    std::cout << "    The event display: " << std::endl;
    std::cout << "  -g    Toggle showing the geometry." << std::endl;
}

