#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <variant>
#include "urlparser.h"

namespace nb = nanobind;
using namespace nb::literals;

inline nb::dict hostname_to_dict(const urlparser::hostname& host) {
    nb::dict dict;
    dict["type"] = "hostname";
    dict["str"] = host.str();
    dict["subdomain"] = host.subdomain();
    dict["domain"] = host.domain();
    dict["domain_name"] = host.domain_name();
    dict["suffix"] = host.suffix();
    return dict;
}

// Handles whichever of hostname/ipv4/ipv6 a url's host actually is, giving
// each its own natural set of dict keys rather than forcing IP addresses
// through domain-shaped fields (subdomain/suffix/etc.) that don't apply.
inline nb::dict host_to_dict(const urlparser::host& host) {
    nb::dict dict;
    std::visit(
        [&dict](const auto& h) {
            using T = std::decay_t<decltype(h)>;
            dict["str"] = h.str();
            if constexpr (std::is_same_v<T, urlparser::hostname>) {
                dict["type"] = "hostname";
                dict["subdomain"] = h.subdomain();
                dict["domain"] = h.domain();
                dict["domain_name"] = h.domain_name();
                dict["suffix"] = h.suffix();
            } else if constexpr (std::is_same_v<T, urlparser::ipv4>) {
                dict["type"] = "ipv4";
                dict["as_int"] = h.to_uint32();
            } else {
                static_assert(std::is_same_v<T, urlparser::ipv6>);
                dict["type"] = "ipv6";
                dict["high64"] = h.high64();
                dict["low64"] = h.low64();
            }
        },
        host.variant());
    return dict;
}

inline std::string host_to_json(const urlparser::host& host) {
    return std::visit(
        [](const auto& h) -> std::string {
            using T = std::decay_t<decltype(h)>;
            if constexpr (std::is_same_v<T, urlparser::hostname>) {
                return "{\"type\": \"hostname\", \"str\": \"" + h.str() + "\""
                    + ", \"subdomain\": \"" + h.subdomain() + "\""
                    + ", \"domain\": \"" + h.domain() + "\""
                    + ", \"domain_name\": \"" + h.domain_name() + "\""
                    + ", \"suffix\": \"" + h.suffix() + "\"}";
            } else if constexpr (std::is_same_v<T, urlparser::ipv4>) {
                return "{\"type\": \"ipv4\", \"str\": \"" + h.str() + "\""
                    + ", \"as_int\": " + std::to_string(h.to_uint32()) + "}";
            } else {
                static_assert(std::is_same_v<T, urlparser::ipv6>);
                return "{\"type\": \"ipv6\", \"str\": \"" + h.str() + "\""
                    + ", \"high64\": " + std::to_string(h.high64())
                    + ", \"low64\": " + std::to_string(h.low64()) + "}";
            }
        },
        host.variant());
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

NB_MODULE(_urlparser_py, m) {
    m.attr("__version__") = URLPARSER_VERSION_STRING;
    m.doc() = R"pbdoc(
        liburlparser
        ------------

        .. currentmodule:: liburlparser

        .. autosummary::
           :toctree: _generate

           Url
           Host
           Hostname
           IPv4
           IPv6
    )pbdoc";

    nb::class_<urlparser::hostname> hostname_cls(m, "Hostname");
    nb::class_<urlparser::ipv4> ipv4_cls(m, "IPv4");
    nb::class_<urlparser::ipv6> ipv6_cls(m, "IPv6");
    nb::class_<urlparser::host> host_cls(m, "Host");
    nb::class_<urlparser::url> url_cls(m, "Url");

    // --- Hostname: a domain name (subdomain/domain/suffix via PSL) --------
    hostname_cls
        .def(nb::init<const std::string&, const bool>(), nb::arg("hoststr"), nb::arg("ignore_www") = false)
        .def_static("from_url",
                    static_cast<urlparser::hostname (*)(std::string_view, bool)>(
                        &urlparser::hostname::from_url),
                    nb::arg("urlstr"), nb::arg("ignore_www") = false)
        .def_static("remove_www", &urlparser::hostname::remove_www, nb::arg("hoststr"))
        .def_prop_ro("subdomain", &urlparser::hostname::subdomain)
        .def_prop_ro("domain", &urlparser::hostname::domain)
        .def_prop_ro("domain_name", &urlparser::hostname::domain_name)
        .def_prop_ro("full_domain", &urlparser::hostname::full_domain)
        .def_prop_ro("suffix", &urlparser::hostname::suffix)
        .def("__eq__", [](const urlparser::hostname& self, const urlparser::hostname& other) {
            return self == other;
        })
        .def("__eq__", [](const urlparser::hostname& self, const std::string& other) {
            return self == other;
        })
        .def("to_dict", hostname_to_dict)
        .def("to_json", [](const urlparser::hostname& h) {
            return "{\"type\": \"hostname\", \"str\": \"" + h.str() + "\""
                + ", \"subdomain\": \"" + h.subdomain() + "\""
                + ", \"domain\": \"" + h.domain() + "\""
                + ", \"domain_name\": \"" + h.domain_name() + "\""
                + ", \"suffix\": \"" + h.suffix() + "\"}";
        })
        .def("__str__", &urlparser::hostname::str)
        .def("__repr__", [](const urlparser::hostname& host) {
            return "<Hostname '" + host.str() + "'>";
        });

    // --- IPv4: a 32-bit address with integer-like arithmetic ---------------
    ipv4_cls
        .def(nb::init<>())
        .def(nb::init<std::string_view>(), nb::arg("text"))
        .def_static("is_valid", &urlparser::ipv4::is_valid, nb::arg("text"))
        .def_static("from_int", &urlparser::ipv4::from_uint32, nb::arg("address"))
        .def_static("from_url", &urlparser::ipv4::from_url, nb::arg("urlstr"))
        .def_prop_ro("as_int", &urlparser::ipv4::to_uint32)
        .def("__int__", &urlparser::ipv4::to_uint32)
        .def("__str__", &urlparser::ipv4::str)
        .def("__repr__", [](const urlparser::ipv4& v) { return "<IPv4 '" + v.str() + "'>"; })
        .def("__eq__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a == b; })
        .def("__lt__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a < b; })
        .def("__le__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a <= b; })
        .def("__gt__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a > b; })
        .def("__ge__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a >= b; })
        .def("__hash__", [](const urlparser::ipv4& v) { return static_cast<size_t>(v.to_uint32()); })
        .def("__add__", [](const urlparser::ipv4& a, int64_t delta) { return a + delta; })
        // ipv4 - int -> ipv4 (step back); ipv4 - ipv4 -> int (distance).
        // nanobind tries __sub__ overloads in registration order and picks
        // the first whose argument type matches.
        .def("__sub__", [](const urlparser::ipv4& a, int64_t delta) { return a - delta; })
        .def("__sub__", [](const urlparser::ipv4& a, const urlparser::ipv4& b) { return a - b; })
        .def("__iadd__", [](urlparser::ipv4& a, int64_t delta) -> urlparser::ipv4& { a += delta; return a; })
        .def("__isub__", [](urlparser::ipv4& a, int64_t delta) -> urlparser::ipv4& { a -= delta; return a; })
        .def("to_dict", [](const urlparser::ipv4& v) {
            nb::dict d;
            d["type"] = "ipv4";
            d["str"] = v.str();
            d["as_int"] = v.to_uint32();
            return d;
        })
        .def("to_json", [](const urlparser::ipv4& v) {
            return "{\"type\": \"ipv4\", \"str\": \"" + v.str() + "\""
                + ", \"as_int\": " + std::to_string(v.to_uint32()) + "}";
        });

    // --- IPv6: a 128-bit address, exposed as (high64, low64) ---------------
    ipv6_cls
        .def(nb::init<>())
        .def(nb::init<std::string_view>(), nb::arg("text"))
        .def_static("is_valid", &urlparser::ipv6::is_valid, nb::arg("text"))
        .def_static("from_uint64_pair", &urlparser::ipv6::from_uint64_pair, nb::arg("high"), nb::arg("low"))
        .def_static("from_url", &urlparser::ipv6::from_url, nb::arg("urlstr"))
        .def_prop_ro("high64", &urlparser::ipv6::high64)
        .def_prop_ro("low64", &urlparser::ipv6::low64)
        .def("__str__", &urlparser::ipv6::str)
        .def("__repr__", [](const urlparser::ipv6& v) { return "<IPv6 '" + v.str() + "'>"; })
        .def("__eq__", [](const urlparser::ipv6& a, const urlparser::ipv6& b) { return a == b; })
        .def("__lt__", [](const urlparser::ipv6& a, const urlparser::ipv6& b) { return a < b; })
        .def("__le__", [](const urlparser::ipv6& a, const urlparser::ipv6& b) { return a <= b; })
        .def("__gt__", [](const urlparser::ipv6& a, const urlparser::ipv6& b) { return a > b; })
        .def("__ge__", [](const urlparser::ipv6& a, const urlparser::ipv6& b) { return a >= b; })
        .def("__hash__", [](const urlparser::ipv6& v) { return std::hash<uint64_t>{}(v.high64()) ^ std::hash<uint64_t>{}(v.low64()); })
        .def("__add__", [](const urlparser::ipv6& a, int64_t delta) { return a + delta; })
        .def("__sub__", [](const urlparser::ipv6& a, int64_t delta) { return a - delta; })
        .def("__iadd__", [](urlparser::ipv6& a, int64_t delta) -> urlparser::ipv6& { a += delta; return a; })
        .def("__isub__", [](urlparser::ipv6& a, int64_t delta) -> urlparser::ipv6& { a -= delta; return a; })
        .def("to_dict", [](const urlparser::ipv6& v) {
            nb::dict d;
            d["type"] = "ipv6";
            d["str"] = v.str();
            d["high64"] = v.high64();
            d["low64"] = v.low64();
            return d;
        })
        .def("to_json", [](const urlparser::ipv6& v) {
            return "{\"type\": \"ipv6\", \"str\": \"" + v.str() + "\""
                + ", \"high64\": " + std::to_string(v.high64())
                + ", \"low64\": " + std::to_string(v.low64()) + "}";
        });

    // --- Host: hostname/ipv4/ipv6, wrapped in one uniform, ergonomic type --
    host_cls
        .def(nb::init<std::string_view, bool>(), nb::arg("host_text"), nb::arg("ignore_www") = false)
        .def_static("from_url",
                    static_cast<urlparser::host (*)(std::string_view, bool)>(&urlparser::host::from_url),
                    nb::arg("urlstr"), nb::arg("ignore_www") = false)
        .def("is_hostname", &urlparser::host::is_hostname)
        .def("is_ipv4", &urlparser::host::is_ipv4)
        .def("is_ipv6", &urlparser::host::is_ipv6)
        .def("is_ip", &urlparser::host::is_ip)
        .def("get_hostname", &urlparser::host::get_hostname)
        .def("get_ipv4", &urlparser::host::get_ipv4)
        .def("get_ipv6", &urlparser::host::get_ipv6)
        // try_*() returns None instead of a null pointer in Python.
        .def("try_hostname", [](const urlparser::host& h) -> nb::object {
            if (auto* p = h.try_hostname()) return nb::cast(*p);
            return nb::none();
        })
        .def("try_ipv4", [](const urlparser::host& h) -> nb::object {
            if (auto* p = h.try_ipv4()) return nb::cast(*p);
            return nb::none();
        })
        .def("try_ipv6", [](const urlparser::host& h) -> nb::object {
            if (auto* p = h.try_ipv6()) return nb::cast(*p);
            return nb::none();
        })
        .def("__eq__", [](const urlparser::host& a, const urlparser::host& b) { return a == b; })
        .def("to_dict", host_to_dict)
        .def("to_json", host_to_json)
        .def("__str__", &urlparser::host::str)
        .def("__repr__", [](const urlparser::host& h) { return "<Host '" + h.str() + "'>"; });

    // --- Url ----------------------------------------------------------------
    url_cls
        .def(nb::init<const std::string&, const bool>(), nb::arg("urlstr"), nb::arg("ignore_www") = false)
        .def_static("extract_host",
                    static_cast<std::string (*)(std::string_view)>(
                        &urlparser::url::extract_host),
                    nb::arg("urlstr"))
        .def_prop_ro("protocol", &urlparser::url::protocol)
        .def_prop_ro("userinfo", &urlparser::url::userinfo)
        // Returns a Host (hostname/ipv4/ipv6, wrapped uniformly) - see
        // is_hostname()/is_ipv4()/is_ipv6()/get_*()/try_*() on Host.
        .def_prop_ro("host", &urlparser::url::host, nb::rv_policy::copy)
        .def_prop_ro("host_text", &urlparser::url::host_text)
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
            return "<Url '" + url.str() + "'>";
        });

    nb::class_<urlparser::psl> psl(m, "Psl", nb::dynamic_attr());

    psl.def_static("instance", &urlparser::psl::instance, nb::rv_policy::reference,
                   "the one, process-wide shared PSL instance")
       .def_prop_ro("url", &urlparser::psl::source_url)
       .def("is_loaded", &urlparser::psl::is_loaded, "check whether psl is loaded or not")
       .def("load_from_path", &urlparser::psl::load_from_path, nb::arg("filepath"), "load PSL from path")
       .def("load_from_string", &urlparser::psl::load_from_string, nb::arg("string"), "load PSL from string")
       .def("is_suffix", &urlparser::psl::is_suffix, nb::arg("text"),
            "check whether text is itself a recognized public suffix (e.g. \"co.uk\"), "
            "not the suffix *of* some hostname")
       .def("__repr__", [](const urlparser::psl& p) -> std::string {
            return std::string("<PSL : ") + (p.is_loaded() ? "loaded" : "not loaded") + ">";
        });
}
