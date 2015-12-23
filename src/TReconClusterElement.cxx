#include "TReconClusterElement.hxx"
#include "TMatrixElement.hxx"
#include "TEventDisplay.hxx"

#include <TCaptLog.hxx>
#include <HEPUnits.hxx>
#include <THandle.hxx>
#include <TUnitsTable.hxx>

#include <TReconCluster.hxx>
#include <TClusterState.hxx>
#include <THandle.hxx>

#include <TEveLine.h>

#include <sstream>

CP::TReconClusterElement::~TReconClusterElement() {}

CP::TReconClusterElement::TReconClusterElement(CP::TReconCluster& cluster,
                                               bool showUncertainty)
    : TEveElementList() {

    double minEnergy = 0.18*unit::MeV/unit::mm;
    double maxEnergy = 3.0*unit::MeV/unit::mm;

    CP::THandle<CP::TClusterState> state = cluster.GetState();
    TLorentzVector var = state->GetPositionVariance();
    TLorentzVector pos = state->GetPosition();
    CaptNamedInfo("cluster","Cluster(" << cluster.GetUniqueID() << ") @ " 
            << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
            << ", " << unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
            << ", " << unit::AsString(pos.Z(),std::sqrt(var.Z()),"length"));

    CP::TCaptLog::IncreaseIndentation();
    
    CaptNamedInfo("cluster","Time " 
                  << unit::AsString(pos.T(),std::sqrt(var.T()),"time"));


    TVector3 longAxis = cluster.GetLongAxis();
    double length = longAxis.Mag();
    double longExtent = cluster.GetLongExtent();

    longAxis = longAxis.Unit();

    TVector3 majorAxis = cluster.GetMajorAxis();
    double major = majorAxis.Mag();
    majorAxis = majorAxis.Unit();

    TVector3 minorAxis = cluster.GetMinorAxis();
    double minor = minorAxis.Mag();
    minorAxis = minorAxis.Unit();

    CaptNamedInfo("cluster","Long Axis (" 
                  << unit::AsString(length,-1,"length") << ")"
                  << " (" << unit::AsString(longAxis.X(),-1,"direction")
                  << ", " << unit::AsString(longAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(longAxis.Z(),-1,"direction") 
                  << ")"
                  << "  Extent: " << unit::AsString(longExtent,-1,"length"));

    CaptNamedInfo("cluster","Major     (" 
                  << unit::AsString(major,-1,"length") << ")"
                  << " (" << unit::AsString(majorAxis.X(),-1,"direction")
                  << ", " << unit::AsString(majorAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(majorAxis.Z(),-1,"direction") 
                  << ")");

    CaptNamedInfo("cluster","Minor     ("
                  << unit::AsString(minor,-1,"length") << ")"
                  << " (" << unit::AsString(minorAxis.X(),-1,"direction")
                  << ", " << unit::AsString(minorAxis.Y(),-1,"direction")
                  << ", " << unit::AsString(minorAxis.Z(),-1,"direction") 
                  << ")");
    
    CP::TCaptLog::DecreaseIndentation();

    // Find the rotation of the object to be displayed.  A cluster is
    // represented as a tube with the long axis along the local Z direction,
    // and the major and minor in the XY plane.
    TMatrixD tubeRot(3,3);
    if (showUncertainty) {
        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                tubeRot(i,j) = state->GetPositionCovariance(i,j);
            }
        }
    }
    else {
        tubeRot(0,0) = majorAxis.X();
        tubeRot(1,0) = majorAxis.Y();
        tubeRot(2,0) = majorAxis.Z();
        tubeRot(0,1) = minorAxis.X();
        tubeRot(1,1) = minorAxis.Y();
        tubeRot(2,1) = minorAxis.Z();
        tubeRot(0,2) = longAxis.X();
        tubeRot(1,2) = longAxis.Y();
        tubeRot(2,2) = longAxis.Z();
    }

    ///////////////////////////////////////////
    // Fill the object to be drawn.
    ///////////////////////////////////////////

    TMatrixElement *eveCluster
        = new TMatrixElement("cluster",
                             cluster.GetPosition().Vect(),
                             tubeRot,
                             true);

    // Set the name.
    std::ostringstream name;
    name << "cluster(" << cluster.GetUniqueID() << ")";
    eveCluster->SetName(name.str().c_str());

    // Set the color.
    int color = kCyan-9;
    // EDeposit is in measured charge.
    double energy = CP::TEventDisplay::Get().CrudeEnergy(cluster.GetEDeposit());
    double dEdX = energy;
    if (longExtent > 1*unit::mm) {
        dEdX /= 2.0*longExtent;              // Get charge per length;
        color = TEventDisplay::Get().LogColor(dEdX,
                                              minEnergy,maxEnergy,2.0);
    }
    eveCluster->SetMainColor(color);

    bool transparentClusters = true;
    if (transparentClusters) eveCluster->SetMainTransparency(60);
    else eveCluster->SetMainTransparency(0);        

    // Build the cluster title.
    std::ostringstream title;
    title << "Cluster(" << cluster.GetUniqueID() << ") @ ";
    title << unit::AsString(pos.X(),std::sqrt(var.X()),"length")
          << ", " << unit::AsString(pos.Y(),std::sqrt(var.Y()),"length")
          << ", " << unit::AsString(pos.Z(),std::sqrt(var.Z()),"length");
    title << std::endl
          << "  Long Axis: " << unit::AsString(length,-1,"length")
          << "  Major Axis: " << unit::AsString(major,-1,"length")
          << "  Minor Axis: " << unit::AsString(minor,-1,"length");


    title << std::endl
          << "  Energy Deposit: " << unit::AsString(energy,-1,"energy")
          << "  dEdX (per cm) " << unit::AsString(dEdX*unit::cm,-1,"energy");
    eveCluster->SetTitle(title.str().c_str());
    
    AddElement(eveCluster);
    
}

