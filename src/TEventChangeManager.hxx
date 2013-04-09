#ifndef TEventChangeManager_hxx_seen
#define TEventChangeManager_hxx_seen
#include <TVInputFile.hxx>

#include <TObject.h>

namespace CP {
    class TEventChangeManager;
};

/// A class to handle a new event becoming available to the event display.
/// There is a single instance of this class owned by TEventDisplay.  This
/// must be created after the GUI has been initialized.
class CP::TEventChangeManager: public TObject {
public:
    TEventChangeManager();
    virtual ~TEventChangeManager();

    /// Set or get the event source.
    /// @{
    void SetEventSource(TVInputFile* source) {fEventSource = source;}
    TVInputFile* GetEventSource() {return fEventSource;}
    /// @}

    /// Trigger an event change.  This is connected to the GUI buttons.
    void ChangeEvent(int change=1);

    /// This updates the event display for a new event.
    void EventChanged();
    
private:

    /// The input source of events.
    TVInputFile* fEventSource;

    ClassDef(TEventChangeManager,0);
};

#endif
