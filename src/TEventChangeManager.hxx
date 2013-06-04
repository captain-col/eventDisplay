#ifndef TEventChangeManager_hxx_seen
#define TEventChangeManager_hxx_seen
#include <TVInputFile.hxx>

#include <TObject.h>

namespace CP {
    class TEventChangeManager;
    class TVEventChangeHandler;

};

/// A class to handle a new event becoming available to the event display.
/// There is a single instance of this class owned by TEventDisplay.  This
/// must be created after the GUI has been initialized.
class CP::TEventChangeManager: public TObject {
public:
    TEventChangeManager();
    virtual ~TEventChangeManager();

    /// Set or get the event source.  When the event source is set, the first
    /// event is read.  @{
    void SetEventSource(TVInputFile* source);
    TVInputFile* GetEventSource() {return fEventSource;}
    /// @}

    /// Trigger an event change.  The argument says how many events to read.
    /// If it's positive, then it reads forward in the file.  If the change is
    /// zero, then the current event is just redrawn.  This is connected to
    /// the GUI buttons.
    void ChangeEvent(int change=1);

    /// Add a handler (taking ownership of the handler) for when the event
    /// needs to be redrawn.
    void AddHandler(CP::TVEventChangeHandler* handler);
    
private:

    /// This updates the event display for a new event using the event change
    /// handlers.  This does not call UpdateEvent().  It is used to do
    /// once-per-event initializations.
    void NewEvent();

    /// This updates the event display for an event using the event change
    /// handlers.
    void UpdateEvent();

    /// The input source of events.
    TVInputFile* fEventSource;

    typedef std::vector<CP::TVEventChangeHandler*> Handlers;

    /// The event update handlers.
    Handlers fHandlers;

    ClassDef(TEventChangeManager,0);
};

#endif
