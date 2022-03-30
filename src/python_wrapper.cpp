/*
MIT License

Copyright (c) 2022 Quandela

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "permanent.h"
#include "sub_permanents.h"
#include "fockstate.h"
#include "fs_array.h"
#include "fs_map.h"
#include "fs_mask.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

long long permanent_in(const py::array_t<long long, py::array::c_style | py::array::forcecast> &M, int n_threads=0)
{
  // check input dimensions
  if ( M.ndim()     != 2 )
    throw std::runtime_error("Input should be 2-D NumPy array");
  if ( M.shape()[0] != M.shape()[1] )
    throw std::runtime_error("Input should have size [N,N]");

  return permanent<long long>(M.data(), M.shape()[0], n_threads);
}

double permanent_fl(const py::array_t<double, py::array::c_style | py::array::forcecast> &M, int n_threads=2)
{
    // check input dimensions
  if ( M.ndim()     != 2 )
    throw std::runtime_error("Input should be 2-D NumPy array");
  if ( M.shape()[0] != M.shape()[1] )
    throw std::runtime_error("Input should have size [N,N]");

  return permanent<double>(M.data(), M.shape()[0], n_threads);
}

std::complex<double> permanent_cx(const py::array_t<std::complex<double>, py::array::c_style | py::array::forcecast> &M,
                                  int n_threads=2)
{
  // check input dimensions
  if ( M.ndim()     != 2 )
    throw std::runtime_error("Input should be 2-D NumPy array");
  if ( M.shape()[0] != M.shape()[1] )
    throw std::runtime_error("Input should have size [N,N]");

  return permanent<std::complex<double>>(M.data(), M.shape()[0], n_threads);
}

py::array_t<double> sub_permanents_fl(const py::array_t<double, py::array::c_style | py::array::forcecast> &M)
{
  // check input dimensions
  if ( M.ndim()     != 2 )
    throw std::runtime_error("Input should be 2-D NumPy array");
  if ( M.shape()[0] != M.shape()[1]+1 )
    throw std::runtime_error("Input should have size [N+1,N]");
  py::array_t<double> output(M.shape()[0]);
  sub_permanents<double>(M.data(), M.shape()[1], (double *)output.data());
  return output;
}

py::array_t<std::complex<double>> sub_permanents_cx(const py::array_t<std::complex<double>, py::array::c_style | py::array::forcecast> &M)
{
  // check input dimensions
  if ( M.ndim()     != 2 )
    throw std::runtime_error("Input should be 2-D NumPy array");
  if ( M.shape()[0] != M.shape()[1]+1 )
    throw std::runtime_error("Input should have size [N+1,N]");
  py::array_t<std::complex<double>> output(M.shape()[0]);
  sub_permanents<std::complex<double>>(M.data(), M.shape()[1], (std::complex<double>*)output.data());
  return output;
}

fockstate get_slice(const fockstate &fs, const py::slice &slice) {
    size_t start, end, step, slice_length;
    if (!slice.compute(fs.get_m(), &start, &end, &step, &slice_length))
        throw py::error_already_set();
    return fs.slice(start, end, step);
}

fockstate set_slice(const fockstate &fs1, const py::slice &slice, const fockstate &fs2) {
    size_t start, end, step, slice_length;
    if (!slice.compute(fs1.get_m(), &start, &end, &step, &slice_length))
        throw py::error_already_set();
    if (step != 1)
        throw std::runtime_error("set_slice does not support step!=1");
    return fs1.set_slice(fs2, start, end);
}

PYBIND11_MODULE(quandelibc, m) {
    m.doc() = "Optimized c-functions";

    m.def("permanent_in", &permanent_in,
          "Permanent of int number (n,n) array",
          py::arg("M"), py::arg("n_threads")=0);
    m.def("permanent_fl", &permanent_fl,
          "Permanent of float number (n,n) array",
          py::arg("M"), py::arg("n_threads")=2);
    m.def("permanent_cx", &permanent_cx,
          "Permanent of complex number (n,n) array",
          py::arg("M"), py::arg("n_threads")=2);
    m.def("sub_permanents_fl", &sub_permanents_fl,
          "Permanent of n+1 (n,n) float number sub-array",
          py::arg("M"));
    m.def("sub_permanents_cx", &sub_permanents_cx,
          "Permanent of n+1 (n,n) complex number sub-array",
          py::arg("M"));

    m.attr("npos") = py::int_(fs_npos);

    py::class_<fockstate>(m, "FockState")
        .def(py::init<fockstate>(),
             "constructor from existing fockstate", py::arg("fs"))
        .def(py::init<int>(),
             "vacuum state constructor, 0 photons")
        .def(py::init<const char *>(),
             "constructor from string representation", py::arg("s"))
        .def(py::init<std::vector<int>>(),
             "constructor from int vector", py::arg("fs_vec"))
        .def("__getitem__", &fockstate::operator[], py::arg("mk"))
        .def("__getitem__", &get_slice)
        .def("set_slice", &set_slice)
        .def("__iter__",
            [](const fockstate &s) { return py::make_iterator(s.begin(), s.end()); },
            py::keep_alive<0, 1>())
        .def("__str__", &fockstate::to_str)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__hash__", &fockstate::hash)
        .def("__add__", (fockstate(fockstate::*)(const fockstate &) const) &fockstate::operator+)
        .def("__add__", (fockstate(fockstate::*)(int) const) &fockstate::operator+)
        .def("__iadd__", (fockstate&(fockstate::*)(const fockstate &)) &fockstate::operator+=)
        .def("__iadd__", (fockstate&(fockstate::*)(int)) &fockstate::operator+=)
        .def("__mul__", &fockstate::operator*)
        .def("__rmul__", &fockstate::operator*)
        .def("slice", &fockstate::slice)
        .def("photon2mode", &fockstate::photon2mode)
        .def("mode2photon", &fockstate::mode2photon)
        .def("prodnfact", &fockstate::prodnfact)
        .def("__copy__", &fockstate::copy)
        .def_property("m", &fockstate::get_m, nullptr)
        .def_property("n", &fockstate::get_n, nullptr);

    py::class_<fs_mask>(m, "FSMask")
        .def(py::init<int, int>())
        .def(py::init<int, int, std::list<std::string>>(), py::arg("m"), py::arg("n"), py::arg("conditions"))
        .def("match", &fs_mask::match, py::arg("fs"), py::arg("allow_missing")=true);

    py::class_<fs_array>(m, "FSArray")
        .def(py::init<int, int>(), py::arg("m"), py::arg("n"))
        .def(py::init<int, int, fs_mask>(), py::arg("m"), py::arg("n"), py::arg("mask"))
        .def(py::init<const char *, int, int>(), py::arg("fdname"), py::arg("m")=-1, py::arg("n")=-1)
        .def("save", &fs_array::save, py::arg("fdname"))
        .def("__getitem__", &fs_array::operator[], py::arg("idx"))
        .def("__iter__",
            [](const fs_array &fsa) { return py::make_iterator(fsa.begin(), fsa.end()); },
            py::keep_alive<0, 1>())
        .def("find", &fs_array::find_idx, py::arg("fs"))
        .def("count", &fs_array::count)
        .def("generate", &fs_array::generate)
        .def("size", &fs_array::size)
        .def_property("m", &fs_array::get_m, nullptr)
        .def_property("n", &fs_array::get_n, nullptr);

    py::class_<fs_map>(m, "FSMap")
        .def(py::init<const fs_array &, const fs_array &, bool>(),
                py::arg("fsa_current"),
                py::arg("fsa_parent"),
                py::arg("generate")=false)
        .def(py::init<const char *, int, int>(), py::arg("fdname"), py::arg("m")=-1, py::arg("n")=-1)
        .def("save", &fs_map::save, py::arg("fdname"))
        .def("get", &fs_map::get, py::arg("idx"), py::arg("mk"))
        .def("count", &fs_map::count)
        .def("size", &fs_map::size)
        .def_property("m", &fs_map::get_m, nullptr)
        .def_property("n", &fs_map::get_n, nullptr);


#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
