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

    /// Field to select a specific event. Selected event number is read
    /// when the return key is pressed.
    void SelectEvent();

    /// Add a handler (taking ownership of the handler) for when the event
    /// changes (e.g. a new event is read).  These handlers are for
    /// "once-per-event" actions and are executed by the NewEvent() method.
    void AddNewEventHandler(CP::TVEventChangeHandler* handler);
    
    /// Add a handler (taking ownership of the handler) for when the event
    /// needs to be redrawn.  These handlers are executed by the UpdateEvent()
    /// method.
    void AddUpdateHandler(CP::TVEventChangeHandler* handler);
    
    /// Set the flag to show (or not show) the geometry
    void SetShowGeometry(bool f) {fShowGeometry = f;}
    bool GetShowGeometry() const {return fShowGeometry;}

private:

    /// This updates the event display for a new event using the event change
    /// handlers.  It is used to do once-per-event initializations and does
    /// not call UpdateEvent().
    void NewEvent();

    /// This updates the event display for an event using the event change
    /// handlers.
    void UpdateEvent();

    /// The input source of events.
    TVInputFile* fEventSource;

    typedef std::vector<CP::TVEventChangeHandler*> Handlers;

    /// The event update handlers.
    Handlers fUpdateHandlers;

    /// The new event handlers.
    Handlers fNewEventHandlers;

    /// Flag to determine if the geometry will be drawn.
    bool fShowGeometry;

    ClassDef(TEventChangeManager,0);
};

#endif
