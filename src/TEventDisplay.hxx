#ifndef TEventDisplay_hxx_seen
#define TEventDisplay_hxx_seen

namespace CP {
    class TEventDisplay;
    class TGUIManager;
    class TEventChangeManager;
};

/// A singleton class for an event display based on EVE.
class CP::TEventDisplay {
public: 

    /// Get a pointer to the singleton instance of the event display.  This
    /// creates the event display the first time it is called.
    static TEventDisplay& Get(void);

    /// Deconstruct the event display.
    virtual ~TEventDisplay();

    /// Return a pointer to the gui manager.
    CP::TGUIManager& GUI() {return *fGUIManager;}

    /// Return a pointer to the event change manager.
    CP::TEventChangeManager& EventChange() {return *fEventChangeManager;}

    /// Get a color from the palette using a linear value scale.
    int LinearColor(double val, double minVal, double maxVal);

    /// Get a color from the palette using a logarithmic value scale.
    int LogColor(double val, double minVal, double maxVal);

private:
    // Prevent direct construction.
    TEventDisplay();

    // Actually initialize the event display
    void Init();

    // Hold the static instance of the event display.
    static CP::TEventDisplay* fEventDisplay;

    // The gui manager.
    TGUIManager* fGUIManager;

    // The event change manager.
    TEventChangeManager* fEventChangeManager;

};

#endif
