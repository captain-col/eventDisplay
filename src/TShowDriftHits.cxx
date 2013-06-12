#include <TShowDriftHits.hxx>

#include <TEveManager.h>
#include <TEveBoxSet.h>

CP::TShowDriftHits::TShowDriftHits(double velocity) 
    : fDriftVelocity(velocity) {}

bool CP::TShowDriftHits::operator () (TEveElementList* elements, 
                                      const CP::THitSelection& hits,
                                      double t0) {

    TEveBoxSet* boxes = new TEveBoxSet(hits.GetName());
    boxes->Reset(TEveBoxSet::kBT_AABox, kTRUE, 128);

    TVector3 pos; 
    TVector3 drift(0,0,fDriftVelocity);
    for (CP::THitSelection::const_iterator h = hits.begin();
         h != hits.end(); ++h) {
        // The position is the position drifted to the time zero.
        // The size (s) is the rms
        // The value is the charge.
        double deltaT = (*h)->GetTime() - t0;
        pos = (*h)->GetPosition() - deltaT*drift;
        const TVector3& rms = (*h)->GetRMS();
        boxes->AddBox(pos.X(), pos.Y(), pos.Z(), rms.X(), rms.Y(), rms.Z());
        boxes->DigitValue((*h)->GetCharge());
    }
    boxes->RefitPlex();
    
    elements->AddElement(boxes);

    return true;
}
