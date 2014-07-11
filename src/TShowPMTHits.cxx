#include <TShowPMTHits.hxx>

#include <TEveManager.h>
#include <TEveBoxSet.h>

CP::TShowPMTHits::TShowPMTHits() {}

bool CP::TShowPMTHits::operator () (TEveElementList* elements, 
                                    const CP::THitSelection& hits) {
    
    TEveBoxSet* boxes = new TEveBoxSet(hits.GetName());
    boxes->Reset(TEveBoxSet::kBT_AABox, kTRUE, 128);

    TVector3 pos; 
    for (CP::THitSelection::const_iterator h = hits.begin();
         h != hits.end(); ++h) {
        double charge = (*h)->GetCharge();
        if (charge<1.0) charge = 1.0;
        pos = (*h)->GetPosition();
        double xyHalf = std::sqrt(charge)*20*unit::mm;
        boxes->AddBox(pos.X()-xyHalf, 
                      pos.Y()-xyHalf, 
                      pos.Z()-10*unit::mm,
                      2*xyHalf, 
                      2*xyHalf,
                      20*unit::mm);
        boxes->DigitValue(50.0);
    }
    boxes->RefitPlex();
    
    elements->AddElement(boxes);

    return true;
}
