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

    /// Trigger an event change.  This is connected to the GUI buttons.
    void ChangeEvent(int change=1);

    /// This updates the event display for a new event using the event change
    /// handlers.
    void EventChanged();

    /// Add an event change handler (taking ownership of the handler).
    void AddHandler(CP::TVEventChangeHandler* handler);
    
private:

    /// The input source of events.
    TVInputFile* fEventSource;

    typedef std::vector<CP::TVEventChangeHandler*> Handlers;

    /// The event change handlers.
    Handlers fHandlers;

    ClassDef(TEventChangeManager,0);
};

#endif
