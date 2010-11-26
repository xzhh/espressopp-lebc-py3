// ESPP_CLASS
#ifndef _INTERACTION_FIXEDQUADRUPLELISTINTERACTIONTEMPLATE_HPP
#define _INTERACTION_FIXEDQUADRUPLELISTINTERACTIONTEMPLATE_HPP

#include "mpi.hpp"
#include "types.hpp"
#include "Interaction.hpp"
#include "Real3D.hpp"
#include "Particle.hpp"
#include "FixedQuadrupleList.hpp"
#include "esutil/Array2D.hpp"
#include "bc/BC.hpp"
#include "SystemAccess.hpp"

namespace espresso {
  namespace interaction {
    template < typename _DihedralPotential >
    class FixedQuadrupleListInteractionTemplate : public Interaction, SystemAccess {
    protected:
      typedef _DihedralPotential Potential;
    public:
      FixedQuadrupleListInteractionTemplate
      (shared_ptr < System > _system,
       shared_ptr < FixedQuadrupleList > _fixedquadrupleList)
        : SystemAccess(_system), fixedquadrupleList(_fixedquadrupleList) 
      {
        potentialArray = esutil::Array2D<Potential, esutil::enlarge>(0, 0, Potential());
      }

      void
      setFixedQuadrupleList(shared_ptr < FixedQuadrupleList > _fixedquadrupleList) {
        fixedquadrupleList = _fixedquadrupleList;
      }

      shared_ptr < FixedQuadrupleList > getFixedQuadrupleList() {
        return fixedquadrupleList;
      }

      void
      setPotential(int type1, int type2, const Potential &potential) {
	potentialArray.at(type1, type2) = potential;
      }

      Potential &getPotential(int type1, int type2) {
        return potentialArray(type1, type2);
      }

      virtual void addForces();
      virtual real computeEnergy();
      virtual real computeVirial();
      virtual void computeVirialTensor(real* wij_);
      virtual real getMaxCutoff();

    protected:
      int ntypes;
      shared_ptr < FixedQuadrupleList > fixedquadrupleList;
      esutil::Array2D<Potential, esutil::enlarge> potentialArray;
    };

    //////////////////////////////////////////////////
    // INLINE IMPLEMENTATION
    //////////////////////////////////////////////////
    template < typename _DihedralPotential > inline void
    FixedQuadrupleListInteractionTemplate < _DihedralPotential >::addForces() {

      LOG4ESPP_INFO(theLogger, "add forces computed by FixedQuadrupleList");

      for (FixedQuadrupleList::Iterator it(*fixedquadrupleList); it.isValid(); ++it) {
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        Particle &p3 = *it->third;
        Particle &p4 = *it->fourth;
        const Potential &potential = getPotential(p1.type(), p2.type());

        Real3D dist21 = getSystemRef().bc->getMinimumImageVector(p2.position(), p1.position());
        Real3D dist32 = getSystemRef().bc->getMinimumImageVector(p3.position(), p2.position());
        Real3D dist43 = getSystemRef().bc->getMinimumImageVector(p4.position(), p3.position());

	Real3D force1, force2, force3, force4;  // result forces

	potential._computeForce(force1, force2, force3, force4,
                                dist21, dist32, dist43);
        p1.force() += force1;
        p2.force() -= force2;
        p3.force() += force3;
        p4.force() += force4;
      }
    }

    template < typename _DihedralPotential >
    inline real
    FixedQuadrupleListInteractionTemplate < _DihedralPotential >::
    computeEnergy() {
      LOG4ESPP_INFO(theLogger, "compute energy of the quadruples");

      espresso::bc::BC& bc = *getSystemRef().bc;
      real e = 0.0;
      for (FixedQuadrupleList::Iterator it(*fixedquadrupleList); it.isValid(); ++it) {
        const Particle &p1 = *it->first;
        const Particle &p2 = *it->second;
        const Particle &p3 = *it->third;
        const Particle &p4 = *it->fourth;
        const Potential &potential = getPotential(p1.type(), p2.type());
        Real3D dist21 = bc.getMinimumImageVector(p2.position(), p1.position());
        Real3D dist32 = bc.getMinimumImageVector(p3.position(), p2.position());
        Real3D dist43 = bc.getMinimumImageVector(p4.position(), p3.position());
        e += potential._computeEnergy(dist21, dist32, dist43);
      }
      real esum;
      boost::mpi::reduce(*mpiWorld, e, esum, std::plus<real>(), 0);
      return esum;
    }

    template < typename _DihedralPotential >
    inline real
    FixedQuadrupleListInteractionTemplate < _DihedralPotential >::
    computeVirial() {
      LOG4ESPP_INFO(theLogger, "compute scalar virial of the quadruples");

      real w = 0.0;
      for (FixedQuadrupleList::Iterator it(*fixedquadrupleList); it.isValid(); ++it) {
        const Particle &p1 = *it->first;
        const Particle &p2 = *it->second;
        const Particle &p3 = *it->third;
        const Particle &p4 = *it->fourth;
        const Potential &potential = getPotential(p1.type(), p2.type());
        Real3D force1;
        Real3D force2;
        Real3D force3;
        Real3D force4;
        espresso::bc::BC& bc = *getSystemRef().bc;
        Real3D dist21 = bc.getMinimumImageVector(p2.position(), p1.position());
        Real3D dist32 = bc.getMinimumImageVector(p3.position(), p2.position());
        Real3D dist43 = bc.getMinimumImageVector(p4.position(), p3.position());
        potential._computeForce(force1, force2, force3, force4,
                                dist21, dist32, dist43);
        w += dist21 * force1 + dist32 * force2;
      }
      real wsum;
      boost::mpi::reduce(*mpiWorld, w, wsum, std::plus<real>(), 0);
      return w;
    }

    template < typename _DihedralPotential >
    inline void
    FixedQuadrupleListInteractionTemplate < _DihedralPotential >::
    computeVirialTensor(real* wij_) {
      LOG4ESPP_INFO(theLogger, "compute the virial tensor of the quadruples");
    
      wij_[0] = 0.0;
      wij_[1] = 0.0;
      wij_[2] = 0.0;
      wij_[3] = 0.0;
      wij_[4] = 0.0;
      wij_[5] = 0.0;

      espresso::bc::BC& bc = *getSystemRef().bc;

      for (FixedQuadrupleList::Iterator it(*fixedquadrupleList); it.isValid(); ++it) {
        const Particle &p1 = *it->first;
        const Particle &p2 = *it->second;
        const Particle &p3 = *it->third;
        const Particle &p4 = *it->fourth;
        const Potential &potential = getPotential(p1.type(), p2.type());
        Real3D force1;
        Real3D force2;
        Real3D force3;
        Real3D force4;
        Real3D dist21 = bc.getMinimumImageVector(p2.position(), p1.position());
        Real3D dist32 = bc.getMinimumImageVector(p3.position(), p2.position());
        Real3D dist43 = bc.getMinimumImageVector(p4.position(), p3.position());
        potential._computeForce(force1, force2, force3, force4,
                                dist21, dist32, dist43);
        wij_[0] += dist21[0] * force1[0] - dist32[0] * force2[0];
        wij_[1] += dist21[1] * force1[1] - dist32[1] * force2[1];
        wij_[2] += dist21[2] * force1[2] - dist32[2] * force2[2];
        wij_[3] += dist21[0] * force1[1] - dist32[0] * force2[1];
        wij_[4] += dist21[0] * force1[2] - dist32[0] * force2[2];
        wij_[5] += dist21[1] * force1[2] - dist32[1] * force2[2];
      }
      real wij_sum[6];
      boost::mpi::reduce(*mpiWorld, wij_, 6, wij_sum, std::plus<real>(), 0);
      for (size_t k = 0; k < 6; k++)
        wij_[k] = wij_sum[k];
    }

    template < typename _DihedralPotential >
    inline real
    FixedQuadrupleListInteractionTemplate< _DihedralPotential >::
    getMaxCutoff() {
      real cutoff = 0.0;

      for (int i = 0; i < ntypes; i++) {
        for (int j = 0; j < ntypes; j++) {
          cutoff = std::max(cutoff, getPotential(i, j).getCutoff());
        }
      }
      return cutoff;
    }
  }
}
#endif
