// Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>
// Copyright (C) 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor

/** @file exception.cpp
 *
 *  This file reflects the Boost.MPI @c mpi_error class into
 *  Python.
 */
#include <boost/python.hpp>
#include <boost/mpi/exception.hpp>
#include <string>
#include <boost/lexical_cast.hpp>

using namespace boost::python;
using namespace boost::mpi;

namespace boost { namespace mpi { namespace python {

extern const char* exception_docstring;
extern const char* exception_what_docstring;
extern const char* exception_routine_docstring;
extern const char* exception_result_code_docstring;

handle<> mpi_exception;

void translate_bmpi_exception(const exception& e)
{
  PyErr_SetObject(mpi_exception.get(), object(e).ptr());
}

str exception_str(const exception& e)
{
  return str(std::string(e.what()) + 
             " (code " + lexical_cast<std::string>(e.result_code())+")");
}

void export_exception()
{
  using boost::python::arg;
  using boost::python::object;

  mpi_exception = handle<>(
      PyErr_NewException("boostmpi.Error", 0, 0));
  scope().attr("Error") = mpi_exception;
  
  register_exception_translator<exception>(&translate_bmpi_exception);
  
  class_<exception>
    ("Exception", exception_docstring, no_init)
    .add_property("what", &exception::what, exception_what_docstring)
    .add_property("routine", &exception::what, exception_routine_docstring)
    .add_property("result_code", &exception::result_code, 
                  exception_result_code_docstring)
    .def("__str__", &exception_str)
  ;
}

} } } // end namespace boost::mpi::python