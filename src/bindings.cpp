/* has to be included before any system headers according to Python API
   to avoid redefining _POSIX_C_SOURCE by Python.h */
#include <python.hpp>
#include <esutil/bindings.hpp>
#include <bc/bindings.hpp>
#include <storage/bindings.hpp>
#include <integrator/bindings.hpp>
#include <System.hpp>
#include <VerletList.hpp>
#include <Real3D.hpp>
#include <Int3D.hpp>
#include <esutil/PyLogger.hpp>

#include "bindings.hpp"

void espresso::registerPython() {

  espresso::System::registerPython();
  espresso::VerletList::registerPython();
  espresso::Real3D::registerPython();
  espresso::Int3D::registerPython();

  espresso::esutil::registerPython();
  espresso::bc::registerPython();
  espresso::storage::registerPython();
  espresso::integrator::registerPython();

  log4espp::PyLogger::registerPython();
}
