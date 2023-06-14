#include <pybind11/pybind11.h>
#include <string>
#include "urlparser.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)


namespace py = pybind11;
using namespace pybind11::literals;

 py::dict host_to_dict(const TLD::Host &host){
     return py::dict(
       "subdomain"_a = host.subdomain(),
       "domain"_a = host.domain(),
       "domain_name"_a = host.domainName(),
       "suffix"_a = host.suffix()
       );
 }

 py::dict url_to_dict(const TLD::Url &url){
     return py::dict(
       "protocol"_a = url.protocol(),
       "userinfo"_a = url.userinfo(),
       "host"_a = host_to_dict(url.host()),
       "port"_a = url.port(),
       "query"_a = url.query(),
       "fragment"_a = url.fragment()
       );
 }



PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: liburlparser

        .. autosummary::
           :toctree: _generate

           Url
           Host
    )pbdoc";

    py::class_<TLD::Host> Host(m, "Host");
    py::class_<TLD::Url> Url(m, "Url");


    Host.def(py::init<const std::string&>())
       .def_static("from_url", &TLD::Host::fromUrl)
       .def_static("load_psl_from_path", &TLD::Host::loadPslFromPath, py::arg("filepath"))
       .def_static("load_psl_from_string", &TLD::Host::loadPslFromString, py::arg("filestr"))
       .def_static("is_psl_loaded", &TLD::Host::isPslLoaded)
       .def_property_readonly("subdomain", &TLD::Host::subdomain)
       .def_property_readonly("domain", &TLD::Host::domain)
       .def_property_readonly("domain_name", &TLD::Host::domainName)
       .def_property_readonly("suffix", &TLD::Host::suffix)
       .def("to_dict", host_to_dict)
       .def("__str__", &TLD::Host::str)
       .def("__repr__",
            [](const TLD::Host &host) {
                return "<Host :'" + host.str() + "'>";
            });

    Url.def(py::init<const std::string&>())
       .def_property_readonly("protocol", &TLD::Url::protocol)
       .def_property_readonly("userinfo", &TLD::Url::userinfo)
       .def_property_readonly("subdomain", &TLD::Url::subdomain)
       .def_property_readonly("domain", &TLD::Url::domain)
       .def_property_readonly("domain_name", &TLD::Url::domainName)
       .def_property_readonly("suffix", &TLD::Url::suffix)
       .def_property_readonly("port", &TLD::Url::port)
       .def_property_readonly("params", &TLD::Url::params)
       .def_property_readonly("query", &TLD::Url::query)
       .def_property_readonly("host", &TLD::Url::host)
       .def("to_dict", url_to_dict)
       .def("__str__", &TLD::Url::str)
       .def("__repr__",
             [](const TLD::Url &url) -> std::string {
                 return "<Url :'" + url.str() + "'>";
             });

    m.def("load_psl_from_path", &TLD::Host::loadPslFromPath, R"pbdoc(
        load PSL from path
        Some other explanation about the load_psl_from_path function.
    )pbdoc");

    m.def("load_psl_from_string", &TLD::Host::loadPslFromString, R"pbdoc(
        load PSL from string
        Some other explanation about the load_psl_from_string function.
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

