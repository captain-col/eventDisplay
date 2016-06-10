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
        const TVector3& half = (*h)->GetRMS();
        boxes->AddBox(pos.X()-half.X(), 
                      pos.Y()-half.Y(), 
                      pos.Z()-half.Z(),
                      2*half.X(), 
                      2*half.Y(),
                      2*half.Z());
        boxes->DigitValue((*h)->GetCharge());
        boxes->DigitId(&(*(*h)));
    }
    boxes->RefitPlex();
    
    elements->AddElement(boxes);

    return true;
}
