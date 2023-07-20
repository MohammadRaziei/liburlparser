#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include "urlparser.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace nb = nanobind;

class Psl{
public:
    Psl() {};
    std::string url() const {return PUBLIC_SUFFIX_LIST_URL;}
    std::string filename() const {return PUBLIC_SUFFIX_LIST_DAT;}
    void loadFromPath(const std::string& filename) {TLD::Host::loadPslFromPath(filename);}
    void loadFromString(const std::string& str) {TLD::Host::loadPslFromString(str);}
    bool isLoaded() const {return TLD::Host::isPslLoaded();}
};

inline nb::dict host_to_dict(const TLD::Host& host) {
    nb::dict dict;
    dict["str"] = host.str();
    dict["subdomain"] = host.subdomain();
    dict["domain"] = host.domain();
    dict["domain_name"] = host.domainName();
    dict["suffix"] = host.suffix();
    return dict;
}
//
inline nb::dict url_to_dict(const TLD::Url& url) {
    nb::dict dict;
    dict["str"] = url.str();
    dict["protocol"] = url.protocol();
    dict["userinfo"] = url.userinfo();
    dict["host"] = host_to_dict(url.host());
    dict["port"] = url.port();
    dict["query"] = url.query();
    dict["fragment"] = url.fragment();
    return dict;
}

inline std::string host_to_json(const TLD::Host& host) {
    return "{\"str\": \"" + host.str() + "\""
        + ", \"subdomain\": \"" + host.subdomain() + "\""
        + ", \"domain\": \"" + host.domain() + "\""
        + ", \"domain_name\": \"" + host.domainName() + "\""
        + ", \"suffix\": \"" + host.suffix() + "\"}";
}

inline std::string url_to_json(const TLD::Url& url) {
    return "{\"str\": \"" + url.str() + "\""
        + ", \"protocol\": \"" + url.protocol() + "\""
        + ", \"userinfo\": \"" + url.userinfo() + "\""
        + ", \"host\": " + host_to_json(url.host())
        + ", \"port\": " + std::to_string(url.port())
        + ", \"query\": \"" + url.query() + "\""
        + ", \"fragment\": \"" + url.fragment() + "\"}";
}

inline nb::dict host_to_dict_minimal(const TLD::Host& host) {
    nb::dict dict;
    dict["suffix"] = host.suffix();
    dict["domain"] = host.domain();
    dict["subdomain"] = host.subdomain();
    return dict;
}

inline nb::dict extract_from_url(const std::string& url){
    return host_to_dict_minimal(TLD::Host::fromUrl(url));
}
inline nb::dict extract(const std::string& host){
    return host_to_dict_minimal(TLD::Host(host));
}

NB_MODULE(_core, m) {
//    try{
//        if (not TLD::Host::isPslLoaded())
//            TLD::Host::loadPslFromPath(PUBLIC_SUFFIX_LIST_DAT);
//    }
//    catch(const std::invalid_argument&) {
//
//    }

    m.doc() = R"pbdoc(
        nanobind example plugin
        -----------------------

        .. currentmodule:: liburlparser

        .. autosummary::
           :toctree: _generate

           Url
           Host
    )pbdoc";
    //
    nb::class_<TLD::Host> Host(m, "Host");
    nb::class_<TLD::Url> Url(m, "Url");

    Host.def(nb::init<const std::string&>())
        .def_static("from_url", &TLD::Host::fromUrl)
        .def_static("extract_from_url", extract_from_url)
        .def_static("extract", extract)
        .def_static("load_psl_from_path", &TLD::Host::loadPslFromPath,
                    nb::arg("filepath"))
        .def_static("load_psl_from_string", &TLD::Host::loadPslFromString,
                    nb::arg("filestr"))
        .def_static("is_psl_loaded", &TLD::Host::isPslLoaded)
        .def_prop_ro("subdomain", &TLD::Host::subdomain)
        .def_prop_ro("domain", &TLD::Host::domain)
        .def_prop_ro("domain_name", &TLD::Host::domainName)
        .def_prop_ro("suffix", &TLD::Host::suffix)
        .def("to_dict", host_to_dict)
        .def("to_json", host_to_json)
        .def("__str__", &TLD::Host::str)
        .def("__repr__", [](const TLD::Host& host) {
            return "<Host :'" + host.str() + "'>";
        });

    Url.def(nb::init<const std::string&>())
        .def_static("extract_host", &TLD::Url::extractHost)
        .def_prop_ro("protocol", &TLD::Url::protocol)
        .def_prop_ro("userinfo", &TLD::Url::userinfo)
        .def_prop_ro("host", &TLD::Url::host)
        .def_prop_ro("subdomain", &TLD::Url::subdomain)
        .def_prop_ro("domain", &TLD::Url::domain)
        .def_prop_ro("domain_name", &TLD::Url::domainName)
        .def_prop_ro("suffix", &TLD::Url::suffix)
        .def_prop_ro("port", &TLD::Url::port)
        .def_prop_ro("params", &TLD::Url::params)
        .def_prop_ro("query", &TLD::Url::query)
        .def_prop_ro("fragment", &TLD::Url::fragment)
        .def("to_dict", url_to_dict)
        .def("to_json", url_to_json)
        .def("__str__", &TLD::Url::str)
        .def("__repr__", [](const TLD::Url& url) -> std::string {
            return "<Url :'" + url.str() + "'>";
        });

    nb::class_<Psl> psl(m, "Psl", nb::dynamic_attr());

    psl.def(nb::init<>())
       .def_prop_ro("url", &Psl::url)
       .def_prop_ro("filename", &Psl::filename)
       .def("is_loaded", &Psl::isLoaded, "check whether psl is loaded or not")
       .def("load_from_path", &Psl::loadFromPath, "load PSL from path")
       .def("load_from_string", &Psl::loadFromString, "load PSL from string")
       .def("__repr__", [](const Psl& p) -> std::string {
            return std::string("<PSL : ") + (p.isLoaded() ? "loaded" : "not loaded") + ">";
        });

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
