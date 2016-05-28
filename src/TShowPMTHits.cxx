#include <TShowPMTHits.hxx>

#include <THitSelection.hxx>
#include <THit.hxx>
#include <TGeometryId.hxx>

#include <TEveManager.h>
#include <TEveBoxSet.h>
#include <TEveRGBAPalette.h>

#include <map>

CP::TShowPMTHits::TShowPMTHits(TEveRGBAPalette* palette)
    : fPalette(palette) {}

bool CP::TShowPMTHits::operator () (TEveElementList* elements, 
                                    const CP::THitSelection& hits) {
    
    TEveBoxSet* boxes = new TEveBoxSet(hits.GetName());
    boxes->Reset(TEveBoxSet::kBT_AABox, kTRUE, 128);

    std::map<CP::TGeometryId, double > hitCharges;
    std::map<CP::TGeometryId, CP::THandle<CP::THit> > firstHits;

    double maxCharge = 1.0;
    for (CP::THitSelection::const_iterator h = hits.begin();
         h != hits.end(); ++h) {
        CP::THandle<CP::THit> firstHit = firstHits[(*h)->GetGeomId()];
        if (!firstHit || ((*h)->GetTime() < firstHit->GetTime())) {
            firstHits[(*h)->GetGeomId()] = *h;
        }
        hitCharges[(*h)->GetGeomId()] += (*h)->GetCharge();
        maxCharge = std::max(maxCharge, hitCharges[(*h)->GetGeomId()]);
    }

    TVector3 pos; 
    for (std::map<CP::TGeometryId,double>::iterator id = hitCharges.begin();
         id != hitCharges.end(); ++id) {
        CP::TGeometryId geomId = id->first;
        double charge = id->second;
        if (charge<1.0) charge = 1.0;
        pos = firstHits[geomId]->GetPosition();
        double xyHalf = 10*unit::mm;
        double zHalf = 5*(1.0+9.0*std::log(1.0+charge)/std::log(10.0))*unit::mm;
        double top = 0.0;
        if (pos.Z() < -10*unit::cm) top = -1;
        boxes->AddBox(pos.X()-xyHalf, 
                      pos.Y()-xyHalf, 
                      pos.Z()+2*top*zHalf,
                      2*xyHalf, 
                      2*xyHalf,
                      2*zHalf);
        boxes->DigitValue(charge);
    }
    boxes->RefitPlex();
    
    elements->AddElement(boxes);

    return true;
}
