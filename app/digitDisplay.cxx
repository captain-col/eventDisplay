#include <TROOT.h>

#include <TEvent.hxx>
#include <TDigitContainer.hxx>
#include <TPulseDigit.hxx>
#include <THandle.hxx>
#include <TCaptLog.hxx>
#include <TChannelId.hxx>
#include <TMCChannelId.hxx>

#include <eventLoop.hxx>

#include <TH2F.h>
#include <TPad.h>
#include <TStyle.h>
#include <TColor.h>

/// This draws 2D plots of the charge vs time for each of the wire planes.
class TShowDigits: public CP::TEventLoopFunction {
public:
    TShowDigits() {}

    virtual ~TShowDigits() {};

    void Usage(void) {
        std::cout << "    No options."
                  << std::endl;
    }

    virtual bool SetOption(std::string option,std::string value="") {
        return false;
    }

    bool operator () (CP::TEvent& event) {
        CP::THandle<CP::TDigitContainer> pmt
            = event.Get<CP::TDigitContainer>("~/digits/pmt");
        CP::THandle<CP::TDigitContainer> drift
            = event.Get<CP::TDigitContainer>("~/digits/drift");
        
        if (!pmt) {
            CaptLog("No PMT signals for this event " << event.GetContext());
            return false;
        }

        if (!drift) {
            CaptLog("No drift signals for this event " << event.GetContext());
            return false;
        }

        int startTime = 1E+6;
        for (CP::TDigitContainer::const_iterator d = pmt->begin();
             d != pmt->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            startTime = std::min(startTime, pulse->GetFirstSample());
        }

        int signalEnd = startTime;
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            signalEnd = std::max(signalEnd, pulse->GetFirstSample());
        }

        int signalStart = signalEnd;
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            signalStart = std::min(signalStart, pulse->GetFirstSample());
        }

        int center = (signalEnd+signalStart)/2;
        int spread = 0.66*(signalEnd-signalStart);
        
        std::cout << startTime 
                  << " " << signalStart 
                  << " " << signalEnd 
                  << " " << center
                  << " " << spread
                  << std::endl;
        
        Double_t r[]    = {1., 0.};
        Double_t g[]    = {1., 0.};
        Double_t b[]    = {1., 0.};
        Double_t stop[] = {0., 1.};
        TColor::CreateGradientColorTable(2, stop, r, g, b, 100);

        gStyle->SetOptStat(false);
        std::string drawOption("colz");

        double maxWire = 0;
        double minWire = 10000;
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire=-1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber();
                if (mc.GetSequence() != 0) continue;
            }
            minWire = std::min(minWire,wire);
            maxWire = std::max(maxWire,wire);
        }

        TH2F* xPlane 
            = new TH2F("xPlane", "Charge on the X wires",
                       maxWire-minWire+1, minWire, maxWire+1,
                       2*spread,center-spread,center+spread);        
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire = -1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber()+0.5;
                if (mc.GetSequence() != 0) continue;
            }
            for (int i = 0; i < pulse->GetNumberOfSamples(); ++i) {
                int tbin = pulse->GetFirstSample() + i;
                xPlane->Fill(wire,tbin+0.5,pulse->GetADC(i)-2000.0);
            }
        }
        xPlane->Draw(drawOption.c_str());
        gPad->Print("xplane.png");


        maxWire = 0;
        minWire = 10000;
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire=-1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber();
                if (mc.GetSequence() != 1) continue;
            }
            minWire = std::min(minWire,wire);
            maxWire = std::max(maxWire,wire);
        }

        TH2F* uPlane 
            = new TH2F("uPlane", "Charge on the U wires",
                       maxWire-minWire+1, minWire, maxWire+1,
                       2*spread,center-spread,center+spread);        
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire = -1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber()+0.5;
                if (mc.GetSequence() != 1) continue;
            }
            for (int i = 0; i < pulse->GetNumberOfSamples(); ++i) {
                int tbin = pulse->GetFirstSample() + i;
                uPlane->Fill(wire,tbin+0.5,pulse->GetADC(i)-2000.0);
            }
        }
        uPlane->Draw(drawOption.c_str());
        gPad->Print("uplane.png");


        maxWire = 0;
        minWire = 10000;
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire=-1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber();
                if (mc.GetSequence() != 2) continue;
            }
            minWire = std::min(minWire,wire);
            maxWire = std::max(maxWire,wire);
        }

        TH2F* vPlane 
            = new TH2F("vPlane", "Charge on the V wires",
                       maxWire-minWire+1, minWire, maxWire+1,
                       2*spread,center-spread,center+spread);        
        for (CP::TDigitContainer::const_iterator d = drift->begin();
             d != drift->end(); ++d) {
            const CP::TPulseDigit* pulse 
                = dynamic_cast<const CP::TPulseDigit*>(*d);
            if (!pulse) continue;
            double wire = -1;
            if (pulse->GetChannelId().IsMCChannel()) {
                CP::TMCChannelId mc(pulse->GetChannelId());
                wire = mc.GetNumber()+0.5;
                if (mc.GetSequence() != 2) continue;
            }
            for (int i = 0; i < pulse->GetNumberOfSamples(); ++i) {
                int tbin = pulse->GetFirstSample() + i;
                vPlane->Fill(wire,tbin+0.5,pulse->GetADC(i)-2000.0);
            }
        }
        vPlane->Draw(drawOption.c_str());
        gPad->Print("vplane.png");

        return true;
    }

private:
};

int main(int argc, char **argv) {
    TShowDigits userCode;
    CP::eventLoop(argc,argv,userCode,1);
    return 0;
}

