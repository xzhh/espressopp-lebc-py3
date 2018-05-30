/*
  Copyright (C) 2016
      Jakub Krajniak (jkrajniak at gmail.com)
  Copyright (C) 2012,2013
      Max Planck Institute for Polymer Research
  Copyright (C) 2008,2009,2010,2011
      Max-Planck-Institute for Polymer Research & Fraunhofer SCAI

  This file is part of ESPResSo++.

  ESPResSo++ is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo++ is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "python.hpp"
#include "TabulatedSubEnsAngular.hpp"
#include "InterpolationLinear.hpp"
#include "InterpolationAkima.hpp"
#include "InterpolationCubic.hpp"
#include "FixedTripleListInteractionTemplate.hpp"
#include "FixedTripleListTypesInteractionTemplate.hpp"

namespace espressopp {
    namespace interaction {

        void TabulatedSubEnsAngular::setFilenames(int dim,
            int itype, boost::python::list _filenames) {
            boost::mpi::communicator world;
            filenames.resize(dim);
            colVarRef.setDimension(dim);
            numInteractions = dim;
            for (int i=0; i<dim; ++i) {
              filenames[i] = boost::python::extract<std::string>(_filenames[i]);
              colVarRef[i].setDimension(4);
              if (itype == 1) { // create a new InterpolationLinear
                  tables[i] = make_shared <InterpolationLinear> ();
                  tables[i]->read(world, filenames[i].c_str());
              }

              else if (itype == 2) { // create a new InterpolationAkima
                  tables[i] = make_shared <InterpolationAkima> ();
                  tables[i]->read(world, filenames[i].c_str());
              }

              else if (itype == 3) { // create a new InterpolationCubic
                  tables[i] = make_shared <InterpolationCubic> ();
                  tables[i]->read(world, filenames[i].c_str());
              }
          }
        }

        void TabulatedSubEnsAngular::addInteraction(int itype,
            boost::python::str fname, const RealND& _cvref) {
            boost::mpi::communicator world;
            int i = numInteractions;
            numInteractions += 1;
            colVarRef.setDimension(numInteractions);
            // Dimension 6: angle, bond, dihed, sd_angle, sd_bond, sd_dihed
            colVarRef[i].setDimension(6);
            colVarRef[i] = _cvref;
            filenames.push_back(boost::python::extract<std::string>(fname));
            weights.push_back(0.);
            if (itype == 1) { // create a new InterpolationLinear
                  tables.push_back(make_shared <InterpolationLinear> ());
                  tables[i]->read(world, filenames[i].c_str());
              }
              else if (itype == 2) { // create a new InterpolationAkima
                  tables.push_back(make_shared <InterpolationAkima> ());
                  tables[i]->read(world, filenames[i].c_str());
              }
              else if (itype == 3) { // create a new InterpolationCubic
                  tables.push_back(make_shared <InterpolationCubic> ());
                  tables[i]->read(world, filenames[i].c_str());
              }
        }

        void TabulatedSubEnsAngular::setColVarRef(
            const RealNDs& cvRefs){
            // Set the reference values of the collective variables
            // aka cluster centers
            for (int i=0; i<numInteractions; ++i)
                colVarRef[i] = cvRefs[i];
        }

        void TabulatedSubEnsAngular::computeColVarWeights(
            const Real3D& dist12, const Real3D& dist32, const bc::BC& bc){
            // Compute the weights for each force field
            // given the reference and instantaneous values of ColVars
            setColVar(dist12, dist32, bc);
            // Compute weights up to next to last FF
            real sumWeights = 0.;
            for (int i=0; i<numInteractions-1; ++i) {
                weights[i] = 1.;
                for (int j=0; j<colVar.getDimension(); ++j) {
                    int k = 0;
                    // Choose between angle, bond, and dihed
                    if (j == 0) k = 0;
                    else if (j>0 && j<1+colVarBondList->size()) k = 1;
                    else k = 2;
                    // Lengthscale of the cluster i
                    real length_ci = colVarRef[i][3+k];
                    // Distance between inst and ref_i
                    real d_i = abs((colVar[j] - colVarRef[i][k]) / colVarSd[k]);
                    if (d_i > length_ci)
                        weights[i] *= exp(- (d_i - length_ci) / alpha);
                }
                sumWeights += weights[i];
            }
            if (sumWeights >= 1.) {
              for (int i=0; i<numInteractions-1; ++i)
                weights[i] /= sumWeights;
            }
            weights[numInteractions-1] = 1. - sumWeights;
        }

        // Collective variables
        void TabulatedSubEnsAngular::setColVar(const Real3D& dist12,
              const Real3D& dist32, const bc::BC& bc) {
            colVar.setDimension(1+colVarBondList->size());
            real dist12_sqr = dist12 * dist12;
            real dist32_sqr = dist32 * dist32;
            real dist1232 = sqrt(dist12_sqr) * sqrt(dist32_sqr);
            real cos_theta = dist12 * dist32 / dist1232;
            colVar[0] = acos(cos_theta);
            // Now all bonds in colVarBondList
            int i=1;
            for (FixedPairList::PairList::Iterator it(*colVarBondList); it.isValid(); ++it) {
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              Real3D dist12;
              bc.getMinimumImageVectorBox(dist12, p1.position(), p2.position());
              colVar[i] = sqrt(dist12 * dist12);
              i+=1;
            }
        }

        typedef class FixedTripleListInteractionTemplate <TabulatedSubEnsAngular>
                FixedTripleListTabulatedSubEnsAngular;
        typedef class FixedTripleListTypesInteractionTemplate<TabulatedSubEnsAngular>
            FixedTripleListTypesTabulatedSubEnsAngular;

        //////////////////////////////////////////////////
        // REGISTRATION WITH PYTHON
        //////////////////////////////////////////////////
        void TabulatedSubEnsAngular::registerPython() {
            using namespace espressopp::python;

            class_ <TabulatedSubEnsAngular, bases <AngularPotential> >
                ("interaction_TabulatedSubEnsAngular", init <>())
                .def("dimension_get", &TabulatedSubEnsAngular::getDimension)
                .def("filenames_get", &TabulatedSubEnsAngular::getFilenames)
                .def("filename_get", &TabulatedSubEnsAngular::getFilename)
                .def("filename_set", &TabulatedSubEnsAngular::setFilename)
                .def("colVarMu_get", &TabulatedSubEnsAngular::getColVarMus)
                .def("colVarMu_set", &TabulatedSubEnsAngular::setColVarMu)
                .def("colVarSd_get", &TabulatedSubEnsAngular::getColVarSds)
                .def("colVarSd_set", &TabulatedSubEnsAngular::setColVarSd)
                .def("weight_get", &TabulatedSubEnsAngular::getWeights)
                .def("weight_set", &TabulatedSubEnsAngular::setWeight)
                .def("alpha_get", &TabulatedSubEnsAngular::getAlpha)
                .def("alpha_set", &TabulatedSubEnsAngular::setAlpha)
                .def("addInteraction", &TabulatedSubEnsAngular::addInteraction)
                .def("colVarRefs_get", &TabulatedSubEnsAngular::getColVarRefs)
                .def("colVarRef_get", &TabulatedSubEnsAngular::getColVarRef)
                .def_pickle(TabulatedSubEnsAngular_pickle())
                ;

            class_ <FixedTripleListTabulatedSubEnsAngular, bases <Interaction> >
                ("interaction_FixedTripleListTabulatedSubEnsAngular",
                init <shared_ptr<System>,
                      shared_ptr <FixedTripleList>,
                      shared_ptr <TabulatedSubEnsAngular> >())
                .def("setPotential", &FixedTripleListTabulatedSubEnsAngular::setPotential)
                .def("getFixedTripleList", &FixedTripleListTabulatedSubEnsAngular::getFixedTripleList);

            class_< FixedTripleListTypesTabulatedSubEnsAngular, bases< Interaction > >
                ("interaction_FixedTripleListTypesTabulatedSubEnsAngular",
                 init< shared_ptr<System>, shared_ptr<FixedTripleList> >())
                .def("setPotential", &FixedTripleListTypesTabulatedSubEnsAngular::setPotential)
                .def("getPotential", &FixedTripleListTypesTabulatedSubEnsAngular::getPotentialPtr)
                .def("setFixedTripleList", &FixedTripleListTypesTabulatedSubEnsAngular::setFixedTripleList)
                .def("getFixedTripleList", &FixedTripleListTypesTabulatedSubEnsAngular::getFixedTripleList);
        }

    } // ns interaction
} // ns espressopp
