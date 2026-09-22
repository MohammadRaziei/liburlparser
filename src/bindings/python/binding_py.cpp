#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <variant>
#include "urlparser.h"
#include "percent_codec.h"

namespace nb = nanobind;
using namespace nb::literals;

// Interned dict keys, built once (first use) and reused for the process's
// lifetime, instead of a fresh PyUnicode object per key on every call -
// mirrors the technique PyDomainExtractor's pyo3 binding uses
// (`pyo3::intern!`) for its hot, dict-returning extract().
namespace interned {
inline PyObject* type_() { static PyObject* k = PyUnicode_InternFromString("type"); return k; }
inline PyObject* str_() { static PyObject* k = PyUnicode_InternFromString("str"); return k; }
inline PyObject* subdomain_() { static PyObject* k = PyUnicode_InternFromString("subdomain"); return k; }
inline PyObject* domain_() { static PyObject* k = PyUnicode_InternFromString("domain"); return k; }
inline PyObject* domain_name_() { static PyObject* k = PyUnicode_InternFromString("domain_name"); return k; }
inline PyObject* suffix_() { static PyObject* k = PyUnicode_InternFromString("suffix"); return k; }
inline PyObject* hostname_literal() { static PyObject* v = PyUnicode_InternFromString("hostname"); return v; }
}  // namespace interned

// Production dict-builder for Hostname.extract_dict_from_host/_url: raw
// CPython C-API + interned keys (mirrors PyDomainExtractor's pyo3
// `intern!` technique) + direct std::string_view slices of the
// already-owned host buffer, computed by mirroring
// urlparser::hostname::ensure_parsed()'s logic (including its ignore_www
// edge cases) instead of going through hostname's own mutable-field
// caching, which would materialize each of domain_/subdomain_/fulldomain_
// as a separately-*owned* std::string first (one substr() allocation
// each) only for this function to allocate a *second* time
// (PyUnicode_FromStringAndSize) to copy them into Python objects.
// domain_name still needs one concatenation (domain + "." + suffix) -
// there's no getting around that allocation if the field is to exist at
// all. See benchmarks/python/binding_fast_path_ab.py for how this was
// arrived at and measured (~2x over the original nb::dict-based version).
inline nb::object hostname_to_dict_6field_direct(const std::string& host_owned, bool ignore_www) {
    std::string suffix = urlparser::psl::instance().suffix_of(host_owned);
    const size_t suffix_pos =
        (suffix.size() < host_owned.size()) ? host_owned.size() - suffix.size() - 1 : std::string::npos;

    std::string_view full = host_owned;
    std::string_view domain_view, subdomain_view;

    if (suffix_pos != std::string::npos && suffix_pos >= 1) {
        std::string_view domain_part(host_owned.data(), suffix_pos);
        const size_t domain_pos = domain_part.find_last_of('.');
        if (domain_pos != std::string_view::npos) {
            size_t subdomain_start = 0;
            bool bail = false;
            if (ignore_www) {
                const size_t www_pos = domain_part.find("www.");
                if (www_pos != 0) {
                    if (www_pos != std::string_view::npos) bail = true;  // matches ensure_parsed()'s early return
                } else {
                    subdomain_start = 4;
                    full = full.substr(4);  // fulldomain_ drops the "www." prefix too
                }
            }
            if (!bail) {
                if (subdomain_start < domain_pos) subdomain_view = domain_part.substr(subdomain_start, domain_pos - subdomain_start);
                domain_view = domain_part.substr(domain_pos + 1);
            } else {
                domain_view = domain_part;  // unsplit, exactly like the early return in ensure_parsed()
            }
        } else {
            domain_view = domain_part;
        }
    }

    std::string domain_name = std::string(domain_view) + "." + suffix;  // one unavoidable allocation

    PyObject* dict = PyDict_New();
    auto set = [&](PyObject* key, std::string_view value) {
        PyObject* v = PyUnicode_FromStringAndSize(value.data(), static_cast<Py_ssize_t>(value.size()));
        PyDict_SetItem(dict, key, v);
        Py_DECREF(v);
    };
    PyDict_SetItem(dict, interned::type_(), interned::hostname_literal());
    set(interned::str_(), full);
    set(interned::subdomain_(), subdomain_view);
    set(interned::domain_(), domain_view);
    set(interned::domain_name_(), domain_name);
    set(interned::suffix_(), suffix);
    return nb::steal(dict);
}

// Isolation probes (temporary, benchmarking only) - narrow down exactly
// where the remaining cost sits between "just copy the string" (already
// measured, ~14.8M ops/s) and "full 3-field dict" (~3.2-3.4M ops/s).
namespace probe {
// Just the singleton access itself, called repeatedly - checks whether
// Meyer's-singleton's thread-safe-init guard (an atomic load on every call
// after the first) is contributing measurably at these throughput levels.
inline size_t singleton_only() { return urlparser::psl::instance().suffix_of("x").size(); }

// std::string copy + psl::instance().suffix_of() ONLY - no domain/subdomain
// slicing, no dict, no Python object at all. Isolates pure PSL-lookup cost
// (as actually exercised through this call path) from ensure_parsed()'s
// extra domain_/subdomain_ allocations and from dict-building.
inline size_t suffix_only(const std::string& host_owned) {
    return urlparser::psl::instance().suffix_of(host_owned).size();
}
}  // namespace probe

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

inline nb::dict ipv4_to_dict(const urlparser::ipv4& v) {
    nb::dict d;
    d["type"] = "ipv4";
    d["str"] = v.str();
    d["as_int"] = v.to_uint32();
    return d;
}

inline nb::dict ipv6_to_dict(const urlparser::ipv6& v) {
    nb::dict d;
    d["type"] = "ipv6";
    d["str"] = v.str();
    d["high64"] = v.high64();
    d["low64"] = v.low64();
    return d;
}

// Same fields as ipv4_to_dict/ipv6_to_dict, built with interned keys + raw
// CPython C-API instead of nb::dict - same technique proven out for
// Hostname.extract_dict_from_host/_url (see hostname_to_dict_6field_direct
// above). No "direct slice" opportunity here (there's no owned buffer to
// slice into - str()/to_uint32()/high64()/low64() each compute their own
// value from the raw bytes), so this is purely the interned-keys +
// raw-C-API win, not the two-allocations-per-field fix that mattered for
// Hostname.
inline nb::object ipv4_to_dict_fast(const urlparser::ipv4& v) {
    static PyObject* k_type = PyUnicode_InternFromString("type");
    static PyObject* k_str = PyUnicode_InternFromString("str");
    static PyObject* k_as_int = PyUnicode_InternFromString("as_int");
    static PyObject* v_ipv4_literal = PyUnicode_InternFromString("ipv4");

    const std::string s = v.str();
    PyObject* dict = PyDict_New();
    PyObject* pystr = PyUnicode_FromStringAndSize(s.data(), static_cast<Py_ssize_t>(s.size()));
    PyObject* pyint = PyLong_FromUnsignedLong(v.to_uint32());
    PyDict_SetItem(dict, k_type, v_ipv4_literal);
    PyDict_SetItem(dict, k_str, pystr);
    PyDict_SetItem(dict, k_as_int, pyint);
    Py_DECREF(pystr);
    Py_DECREF(pyint);
    return nb::steal(dict);
}

inline nb::object ipv6_to_dict_fast(const urlparser::ipv6& v) {
    static PyObject* k_type = PyUnicode_InternFromString("type");
    static PyObject* k_str = PyUnicode_InternFromString("str");
    static PyObject* k_high64 = PyUnicode_InternFromString("high64");
    static PyObject* k_low64 = PyUnicode_InternFromString("low64");
    static PyObject* v_ipv6_literal = PyUnicode_InternFromString("ipv6");

    const std::string s = v.str();
    PyObject* dict = PyDict_New();
    PyObject* pystr = PyUnicode_FromStringAndSize(s.data(), static_cast<Py_ssize_t>(s.size()));
    PyObject* pyhi = PyLong_FromUnsignedLongLong(v.high64());
    PyObject* pylo = PyLong_FromUnsignedLongLong(v.low64());
    PyDict_SetItem(dict, k_type, v_ipv6_literal);
    PyDict_SetItem(dict, k_str, pystr);
    PyDict_SetItem(dict, k_high64, pyhi);
    PyDict_SetItem(dict, k_low64, pylo);
    Py_DECREF(pystr);
    Py_DECREF(pyhi);
    Py_DECREF(pylo);
    return nb::steal(dict);
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

// Single-call, dict-returning constructor for Url.extract_dict() - see the
// comment at its .def_static() call site for why this exists.
//
// parse_host=True: u.host() classifies IPv4/IPv6/hostname and, for a
// hostname, runs the PSL lookup for subdomain/domain/suffix - the nested
// dict from host_to_dict(). parse_host=False: host_text() is the raw
// field, no classification or PSL lookup at all - a plain string. Skip
// the host-parsing work entirely when the caller only wants
// protocol/path/query/fragment.
inline nb::dict url_extract_dict(std::string_view urlstr, bool ignore_www, bool parse_host) {
    urlparser::url u(std::string(urlstr), ignore_www);
    nb::dict dict;
    dict["str"] = u.str();
    dict["protocol"] = u.protocol();
    dict["userinfo"] = u.userinfo();
    dict["host"] = parse_host ? nb::object(host_to_dict(u.host()))
                               : nb::object(nb::cast(std::string(u.host_text())));
    dict["port"] = u.port();
    dict["query"] = u.query();
    dict["fragment"] = u.fragment();
    return dict;
}

NB_MODULE(_urlparser_py, m) {
    m.def("unquote", [](std::string_view s) {
        return urlparser::percent_codec::decode(s);
    }, "s"_a, "Percent-decode a string (e.g. \"caf%C3%A9\" -> \"café\").");
    m.def("quote", [](std::string_view s, bool keep_slash) {
        return urlparser::percent_codec::encode(
            s, keep_slash ? urlparser::percent_codec::path_safe_set()
                          : urlparser::percent_codec::unreserved_set());
    }, "s"_a, "keep_slash"_a = false,
       "Percent-encode a string. keep_slash=True leaves '/' unescaped "
       "(handy for a whole path component).");
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
        .def_prop_ro("normalized_ascii", &urlparser::hostname::normalized_ascii)
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
        .def("to_dict", ipv4_to_dict)
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
        .def("to_dict", ipv6_to_dict)
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
        })
        .def_static(
            "extract_dict",
            url_extract_dict,
            nb::arg("urlstr"), nb::arg("ignore_www") = false, nb::arg("parse_host") = true,
            "Parse `urlstr` and return {str, protocol, userinfo, host, port, "
            "query, fragment} directly, without constructing a Url object - "
            "the single-call equivalent of Url(urlstr).to_dict(). "
            "With parse_host=True (default) `host` is itself a nested dict "
            "(hostname broken into subdomain/domain/suffix via the PSL, or "
            "an IPv4/IPv6 breakdown) - same shape as Url(...).to_dict(). "
            "With parse_host=False `host` is left as a plain string and the "
            "PSL lookup / host-type classification is skipped entirely, "
            "for callers who only need protocol/path/query/fragment.");

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

    // --- Single-call, dict-returning static constructors -------------------
    // Hostname(host).to_dict() (or any other "construct then call a method")
    // pattern crosses the Python/C++ boundary twice: once for nanobind to
    // build and refcount a persistent Python wrapper object around the C++
    // hostname, and once more for the method call on it. When the *only*
    // thing you want is the dict, that wrapper object is pure overhead -
    // it's built, used once, and immediately thrown away. Measured ~15-25%
    // faster than Hostname(host).to_dict() for that reason.
    //
    // extract_dict_from_host/extract_dict_from_url mirror what liburlparser
    // 1.6.1 had (Host.extract / Host.extract_from_url) and match the call
    // shape of PyDomainExtractor.extract(): the C++ object is constructed,
    // filled into a dict, and destroyed entirely on the C++ side. Python
    // only ever sees the dict, in one FFI call. The same pattern is applied
    // to IPv4/IPv6 below for consistency, even though their to_dict() is
    // cheap enough that the win there is smaller.
    hostname_cls
        .def_static(
            "extract_dict_from_host",
            [](std::string_view host, bool ignore_www) {
                return hostname_to_dict_6field_direct(std::string(host), ignore_www);
            },
            nb::arg("host"), nb::arg("ignore_www") = false,
            "Parse `host` as a hostname and return {str, subdomain, domain, "
            "domain_name, suffix} directly, without constructing a Hostname "
            "object. Prefer this over Hostname(host).to_dict() when you "
            "only need the dict.")
        .def_static(
            "extract_dict_from_url",
            [](std::string_view url, bool ignore_www) {
                return hostname_to_dict_6field_direct(urlparser::url::extract_host(url), ignore_www);
            },
            nb::arg("url"), nb::arg("ignore_www") = false,
            "Extract the host from `url`, parse it as a hostname, and "
            "return {str, subdomain, domain, domain_name, suffix} "
            "directly - the single-call equivalent of "
            "Hostname.from_url(url).to_dict().")
        .def_static("probe_singleton_only", []() { return probe::singleton_only(); })
        .def_static("probe_suffix_only", [](std::string_view host) { return probe::suffix_only(std::string(host)); })
        .def_static("probe_bare_call", [](std::string_view host) { return host.size(); })
        .def_static(
            "probe_construct_only",
            [](std::string_view host) {
                urlparser::hostname h(std::string(host), false);
                return h.str().empty();
            })
        .def_static(
            "probe_parsed_only",
            [](std::string_view host) {
                urlparser::hostname h(std::string(host), false);
                return h.suffix().size();
            });

    ipv4_cls
        .def_static(
            "extract_dict_from_host",
            [](std::string_view text) { return ipv4_to_dict_fast(urlparser::ipv4(text)); },
            nb::arg("host"),
            "Parse `host` as an IPv4 address and return {str, as_int} "
            "directly, without constructing an IPv4 object.")
        .def_static(
            "extract_dict_from_url",
            [](std::string_view url) { return ipv4_to_dict_fast(urlparser::ipv4::from_url(url)); },
            nb::arg("url"),
            "Extract the host from `url`, parse it as an IPv4 address, "
            "and return {str, as_int} directly.");

    ipv6_cls
        .def_static(
            "extract_dict_from_host",
            [](std::string_view text) { return ipv6_to_dict_fast(urlparser::ipv6(text)); },
            nb::arg("host"),
            "Parse `host` as an IPv6 address and return {str, high64, "
            "low64} directly, without constructing an IPv6 object.")
        .def_static(
            "extract_dict_from_url",
            [](std::string_view url) { return ipv6_to_dict_fast(urlparser::ipv6::from_url(url)); },
            nb::arg("url"),
            "Extract the host from `url`, parse it as an IPv6 address, "
            "and return {str, high64, low64} directly.");
}
