#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
// If you have any custom containers, you might need more includes.

#include "orbitMath.h"
// e.g. #include "main.h" or whatever includes the struct OrbitDataAll, parse_TLE_file, etc.

namespace py = pybind11;

// We'll wrap OrbitDataAll in a Python-friendly way

PYBIND11_MODULE(my_sgp4_module, m) {
    m.doc() = "SGP4 Orbit Propagation Module (C++) exposed to Python via pybind11";

    // Expose the GeoData struct
    py::class_<GeoData>(m, "GeoData")
        .def(py::init<double,double,double,double>())   // or the relevant constructor
        .def_readwrite("satID", &GeoData::satID)
        .def_readwrite("time",  &GeoData::time)
        .def_readwrite("lat",   &GeoData::lat)
        .def_readwrite("lon",   &GeoData::lon)
        .def_readwrite("alt",   &GeoData::alt);

    // Expose the OrbitDataAll struct
    py::class_<OrbitResults>(m, "OrbitResults")
        .def(py::init<>())
        .def_readwrite("names",      &OrbitResults::names)
        .def_readwrite("shortOrbit", &OrbitResults::shortOrbits)
        .def_readwrite("longOrbit",  &OrbitResults::longOrbits);

    // Expose the function runOrbitPropagation
    m.def("runOrbitPropagation", &runPropagation,
          "Reads TLE from file, propagates orbits, returns names/shortOrbit/longOrbit");
}
