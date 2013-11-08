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

void usage() {
    std::cout << "Usage: event-display.exe [input-file] " << std::endl;
    std::cout << "    The event display: " << std::endl;
    std::cout << "  -g    Toggle showing the geometry." << std::endl;
    std::cout << "  -c    Set the log configuration file." << std::endl;
    std::cout << "  -d    Increase the debug level"
              << std::endl;
    std::cout << "  -D <name>=[error,severe,warn,debug,trace]"
              << std::endl
              << "        Change the named debug level"
              << std::endl;
    std::cout << "  -v    Increase the verbosity"
              << std::endl;
    std::cout << "  -V <name>=[quiet,log,info,verbose]"
              << std::endl
              << "        Change the named log level"
              << std::endl;
}

int main(int argc, char **argv) {
    std::string fileName = "";
    bool showGeometry = false;
    int debugLevel = 0;
    std::map<std::string, CP::TCaptLog::ErrorPriority> namedDebugLevel;
    int logLevel = -1; // Will choose default logging level...
    std::map<std::string, CP::TCaptLog::LogPriority> namedLogLevel;
    char *configName = NULL;
    while (1) {
        int c = getopt(argc, argv, "?hgdD:vV:c:");
        if (c == -1) break;
        switch (c) {
        case 'g': // Show the geometry.
            showGeometry = not showGeometry;
            break;
        case 'c': {
            configName = strdup(optarg);
            break;
        }
        case 'd': {
            // increase the debugging level.
            ++debugLevel;
            break;
        }
        case 'D': {
            // Set the debug level for a named trace.
            std::string arg(optarg);
            std::size_t sep = arg.find("=");
            if (sep != std::string::npos) {
                std::string name = arg.substr(0,sep);
                std::string levelName = arg.substr(sep+1);
                switch (levelName[0]) {
                case 'e': case 'E':
                    namedDebugLevel[name.c_str()] = CP::TCaptLog::ErrorLevel;
                    break;
                case 's': case 'S':
                    namedDebugLevel[name.c_str()] = CP::TCaptLog::SevereLevel;
                    break;
                case 'w': case 'W':
                    namedDebugLevel[name.c_str()] = CP::TCaptLog::WarnLevel;
                    break;
                case 'd': case 'D':
                    namedDebugLevel[name.c_str()] = CP::TCaptLog::DebugLevel;
                    break;
                case 't': case 'T':
                    namedDebugLevel[name.c_str()] = CP::TCaptLog::TraceLevel;
                    break;
                default:
                    usage();
                    return 1;
                }
            }
            break;
        }
        case 'v': {
            // increase the verbosity level.
            if (logLevel>0) ++logLevel;
            else logLevel = 2;
            break;
        }
        case 'V': {
            // Set the debug level for a named trace.
            std::string arg(optarg);
            std::size_t sep = arg.find("=");
            if (sep != std::string::npos) {
                std::string name = arg.substr(0,sep);
                std::string levelName = arg.substr(sep+1);
                switch (levelName[0]) {
                case 'q': case 'Q':
                    namedLogLevel[name.c_str()] = CP::TCaptLog::QuietLevel;
                    break;
                case 'l': case 'L':
                    namedLogLevel[name.c_str()] = CP::TCaptLog::LogLevel;
                    break;
                case 'i': case 'I':
                    namedLogLevel[name.c_str()] = CP::TCaptLog::InfoLevel;
                    break;
                case 'v': case 'V':
                    namedLogLevel[name.c_str()] = CP::TCaptLog::VerboseLevel;
                    break;
                default:
                    usage();
                    return 1;
                }
            }
            break;
        }
        case '?':
        case 'h':
        default:
            usage();
            return 1;
        }
    }
    
    // Set up the logging code.
    CP::TCaptLog::Configure(configName);
    
    if (logLevel == 0) {
        CP::TCaptLog::SetLogLevel(CP::TCaptLog::QuietLevel);
    }
    else if (logLevel == 1) {
        CP::TCaptLog::SetLogLevel(CP::TCaptLog::LogLevel);
        CaptLog("Set log level to LogLevel");
    }
    else if (logLevel == 2) {
        CP::TCaptLog::SetLogLevel(CP::TCaptLog::InfoLevel);
        CaptInfo("Set log level to InfoLevel");
    }
    else if (logLevel >= 3) {
        CP::TCaptLog::SetLogLevel(CP::TCaptLog::VerboseLevel);
        CaptVerbose("Set log level to VerboseLevel");
    }
    
    for (std::map<std::string,CP::TCaptLog::LogPriority>::iterator i 
             = namedLogLevel.begin();
         i != namedLogLevel.end();
         ++i) {
        CP::TCaptLog::SetLogLevel(i->first.c_str(), i->second);
    }
         
    if (debugLevel == 1) {
        CP::TCaptLog::SetDebugLevel(CP::TCaptLog::WarnLevel);
        CaptWarn("Set debug level to WarnLevel");
    }
    else if (debugLevel == 2) {
        CP::TCaptLog::SetDebugLevel(CP::TCaptLog::DebugLevel);
        CaptDebug("Set debug level to DebugLevel");
    }
    else if (debugLevel >= 2) {
        CP::TCaptLog::SetDebugLevel(CP::TCaptLog::TraceLevel);
        CaptTrace("Set debug level to TraceLevel");
    }

    for (std::map<std::string,CP::TCaptLog::ErrorPriority>::iterator i 
             = namedDebugLevel.begin();
         i != namedDebugLevel.end();
         ++i) {
        CP::TCaptLog::SetDebugLevel(i->first.c_str(), i->second);
    }
         
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
        CaptError("Must provide an input file");
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


