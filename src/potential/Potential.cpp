#include "python.hpp"

#include "Potential.hpp"

using namespace espresso;
using namespace espresso::potential;

potential::ForceComputerBase::SelfPtr 
Potential::createForceComputer(Property< Real3D >::SelfPtr _forceProperty, bool _computesVirial) {
  return createForceComputer(_forceProperty, _forceProperty, _computesVirial);
}

potential::EnergyComputerBase::SelfPtr
Potential::createEnergyComputer(Property< real >::SelfPtr _energyProperty) {
  return createEnergyComputer(_energyProperty, _energyProperty);
}

//////////////////////////////////////////////////
// REGISTRATION WITH PYTHON
//////////////////////////////////////////////////
namespace espresso {
  namespace potential {
    class PythonPotential 
      : public espresso::python::wrapper< Potential >,
	public PotentialBase< PythonPotential > {
    public:
      real _getCutoffSqr() const {
	return get_override("getCutoffSqr")();
      }
      
      real _computeEnergy(const Real3D dist) const {
	return get_override("computeEnergy")(dist);
      }
      
      Real3D _computeForce(const Real3D dist) const {
	return get_override("computeForce")(dist);
      }
    };
  }
}

void
Potential::registerPython() {
  using namespace espresso::python;

  ForceComputerBase::SelfPtr (Potential::*createForceComputer1)
    (Property< Real3D >::SelfPtr, Property< Real3D >::SelfPtr, bool) 
     = &Potential::createForceComputer;
  ForceComputerBase::SelfPtr (Potential::*createForceComputer2)
    (Property< Real3D >::SelfPtr, bool) 
     = &Potential::createForceComputer;

  EnergyComputerBase::SelfPtr (Potential::*createEnergyComputer1)
    (Property< real >::SelfPtr, Property< real >::SelfPtr) 
     = &Potential::createEnergyComputer;
  EnergyComputerBase::SelfPtr (Potential::*createEnergyComputer2)
    (Property< real >::SelfPtr) 
     = &Potential::createEnergyComputer;

  class_< espresso::potential::PythonPotential, boost::noncopyable >
    ("potential_PythonPotential")
    .def("getCutoffSqr", pure_virtual(&Potential::getCutoffSqr))
    .def("computeEnergy", pure_virtual(&Potential::computeEnergy))
    .def("computeForce", pure_virtual(&Potential::computeForce))
    .def("createForceComputer", pure_virtual(createForceComputer1))
    .def("createForceComputer", createForceComputer2)
    .def("createEnergyComputer", pure_virtual(createEnergyComputer1))
    .def("createEnergyComputer", createEnergyComputer2)
    ;
}

