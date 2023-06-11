#include <pybind11/pybind11.h>
#include <string>
#include "urlparser.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
    return i + j;
}

std::string myfun(std::string url) {
    return  TLD::Url(url).domain();
}

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: scikit_build_example

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

    m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

    m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

    m.def("myfun", &myfun, R"pbdoc(
    Add two numbers

        Some other explanation about the add function.
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}



//#include <pybind11/pybind11.h>
//#include <pybind11/stl.h>
//#include "urlparser.h"
//
//#define STRINGIFY(x) #x
//#define MACRO_STRINGIFY(x) STRINGIFY(x)
//
//namespace py = pybind11;
//using namespace TLD;
//
//PYBIND11_MODULE(_core, m) {
//    py::class_<QueryParams>(m, "QueryParams")
//        .def(py::init<>())
//        .def("__str__", [](const QueryParams& self) {
//            return py::str(py::cast(self));
//        });
//
//    py::class_<Url::Host>(m, "Host")
//        .def(py::init<const std::string&>())
//        .def_property_readonly("suffix", &Url::Host::suffix)
//        .def_property_readonly("domain", &Url::Host::domain)
//        .def_property_readonly("domain_name", &Url::Host::domainName)
//        .def_property_readonly("subdomain", &Url::Host::subdomain)
//        .def_property_readonly("full_domain", &Url::Host::fulldomain)
//        .def("__str__", &Url::Host::str);
//
//    py::class_<Url>(m, "Url")
//        .def(py::init<const std::string&>())
//        .def_property_readonly("protocol", &Url::protocol)
//        .def_property_readonly("subdomain", &Url::subdomain)
//        .def_property_readonly("domain", &Url::domain)
//        .def_property_readonly("suffix", &Url::suffix)
//        .def_property_readonly("query", &Url::query)
//        .def_property_readonly("fragment", &Url::fragment)
//        .def_property_readonly("userinfo", &Url::userinfo)
//        .def_property_readonly("abspath", &Url::abspath)
//        .def_property_readonly("domain_name", &Url::domainName)
//        .def_property_readonly("full_domain", &Url::fulldomain)
//        .def_property_readonly("port", &Url::port)
//        .def_property_readonly("params", &Url::params)
//        .def_property_readonly("host", &Url::host)
//        .def("__str__", &Url::str);
//
//    m.def("load_psl_from_path", &Url::loadPslFromPath);
//    m.def("load_psl_from_string", &Url::loadPslFromString);
//
//    m.def("from_url", &Url::Host::from_url);
//
//    #ifdef VERSION_INFO
//        m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
//    #else
//        m.attr("__version__") = "dev";
//    #endif
//    }
//}

