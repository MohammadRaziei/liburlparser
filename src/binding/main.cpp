#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

#include <string>
#include "urlparser.h"

namespace nb = nanobind;
using namespace nb::literals;

class Psl{
public:
    Psl() {};
    std::string url() const {return PUBLIC_SUFFIX_LIST_URL;}
    void loadFromPath(const std::string& filename) {urlparser::host::load_psl_from_path(filename);}
    void loadFromString(const std::string& str) {urlparser::host::load_psl_from_string(str);}
    bool isLoaded() const {return urlparser::host::is_psl_loaded();}
};

inline nb::dict host_to_dict(const urlparser::host& host) {
    nb::dict dict;
    dict["str"] = host.str();
    dict["subdomain"] = host.subdomain();
    dict["domain"] = host.domain();
    dict["domain_name"] = host.domain_name();
    dict["suffix"] = host.suffix();
    return dict;
}
//
inline nb::dict url_to_dict(const urlparser::url& url) {
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

inline std::string host_to_json(const urlparser::host& host) {
    return "{\"str\": \"" + host.str() + "\""
        + ", \"subdomain\": \"" + host.subdomain() + "\""
        + ", \"domain\": \"" + host.domain() + "\""
        + ", \"domain_name\": \"" + host.domain_name() + "\""
        + ", \"suffix\": \"" + host.suffix() + "\"}";
}

inline std::string url_to_json(const urlparser::url& url) {
    return "{\"str\": \"" + url.str() + "\""
        + ", \"protocol\": \"" + std::string(url.protocol()) + "\""
        + ", \"userinfo\": \"" + std::string(url.userinfo()) + "\""
        + ", \"host\": " + host_to_json(url.host())
        + ", \"port\": " + std::to_string(url.port())
        + ", \"query\": \"" + std::string(url.query()) + "\""
        + ", \"fragment\": \"" + std::string(url.fragment()) + "\"}";
}

inline nb::dict host_to_dict_minimal(const urlparser::host& host) {
    nb::dict dict;
    dict["suffix"] = host.suffix();
    dict["domain"] = host.domain();
    dict["subdomain"] = host.subdomain();
    return dict;
}

inline nb::dict extract_from_url(const std::string& url){
    return host_to_dict_minimal(urlparser::host::from_url(url));
}
inline nb::dict extract(const std::string& host){
    return host_to_dict_minimal(urlparser::host(host));
}

NB_MODULE(_urlparser_py, m) {
    m.attr("__version__") = URLPARSER_VERSION_STRING;
    m.doc() = R"pbdoc(
        nanobind example plugin
        -----------------------

        .. currentmodule:: liburlparser

        .. autosummary::
           :toctree: _generate

           url
           host
    )pbdoc";
    //
    nb::class_<urlparser::host> host(m, "Host");
    nb::class_<urlparser::url> url(m, "Url");

    host.def(nb::init<const std::string&, const bool>(), nb::arg("hoststr"), nb::arg("ignore_www") = false)
        .def_static("from_url",
                    static_cast<urlparser::host (*)(const std::string&, bool)>(
                        &urlparser::host::from_url),
                    nb::arg("urlstr"), nb::arg("ignore_www") = false)
        .def_static("extract_from_url", extract_from_url, nb::arg("urlstr"))
        .def_static("extract", extract, nb::arg("hoststr"))
        .def_static("load_psl_from_path", &urlparser::host::load_psl_from_path,
                    nb::arg("filepath"))
        .def_static("load_psl_from_string", &urlparser::host::load_psl_from_string,
                    nb::arg("string"))
        .def_static("is_psl_loaded", &urlparser::host::is_psl_loaded)
        .def_static("remove_www", &urlparser::host::remove_www, nb::arg("hoststr"))
        .def_prop_ro("subdomain", &urlparser::host::subdomain)
        .def_prop_ro("domain", &urlparser::host::domain)
        .def_prop_ro("domain_name", &urlparser::host::domain_name)
        .def_prop_ro("full_domain", &urlparser::host::full_domain)
        .def_prop_ro("suffix", &urlparser::host::suffix)
        .def("__eq__", [](const urlparser::host& self, const urlparser::host& other) {
            return self == other;
        })
        .def("__eq__", [](const urlparser::host& self, const std::string& other) {
            return self == other;
        })
        .def("to_dict", host_to_dict)
        .def("to_json", host_to_json)
        .def("__str__", &urlparser::host::str)
        .def("__repr__", [](const urlparser::host& host) {
            return "<host :'" + host.str() + "'>";
        });

    url.def(nb::init<const std::string&, const bool>(), nb::arg("urlstr"), nb::arg("ignore_www") = false)
        .def_static("extract_host",
                    static_cast<std::string (*)(std::string_view)>(
                        &urlparser::url::extract_host),
                    nb::arg("urlstr"))
        .def_prop_ro("protocol", &urlparser::url::protocol)
        .def_prop_ro("userinfo", &urlparser::url::userinfo)
        .def_prop_ro("host", &urlparser::url::host)
        .def_prop_ro("subdomain", &urlparser::url::subdomain)
        .def_prop_ro("domain", &urlparser::url::domain)
        .def_prop_ro("full_domain", &urlparser::url::full_domain)
        .def_prop_ro("domain_name", &urlparser::url::domain_name)
        .def_prop_ro("suffix", &urlparser::url::suffix)
        .def_prop_ro("port", &urlparser::url::port)
        .def_prop_ro("params", &urlparser::url::params)
        .def_prop_ro("query", &urlparser::url::query)
        .def_prop_ro("fragment", &urlparser::url::fragment)
        .def_prop_ro("abspath", &urlparser::url::abspath)
        .def("__eq__", &urlparser::url::operator==)
        .def("to_dict", url_to_dict)
        .def("to_json", url_to_json)
        .def("__str__", &urlparser::url::str)
        .def("__repr__", [](const urlparser::url& url) -> std::string {
            return "<url :'" + url.str() + "'>";
        });

    nb::class_<Psl> psl(m, "Psl", nb::dynamic_attr());

    psl.def(nb::init<>())
       .def_prop_ro("url", &Psl::url)
       .def("is_loaded", &Psl::isLoaded, "check whether psl is loaded or not")
       .def("load_from_path", &Psl::loadFromPath, nb::arg("filepath"), "load PSL from path")
       .def("load_from_string", &Psl::loadFromString, nb::arg("string"), "load PSL from string")
       .def("__repr__", [](const Psl& p) -> std::string {
            return std::string("<PSL : ") + (p.isLoaded() ? "loaded" : "not loaded") + ">";
        });


}
