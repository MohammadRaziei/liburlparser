#include "utest.h"
#include <string>

#include "urlparser.h"

using urlparser::scp;

// Gap: liburlparser had no support at all for the "scp-like" SSH address
// shape ([user@]host:path) that scp, rsync, sshfs, and git all accept -
// e.g. "git@github.com:user/repo.git". It's not RFC 3986 URI syntax (that
// colon is a path separator, not a port), so plain url() misparses it.
//
// Rule verified against git's own documentation (`git help clone`, "GIT
// URLS"): scp-like syntax is recognized only when there's no "://" and no
// '/' before the first ':'.

UTEST(scp, is_scp_like_true_for_basic_case) {
    ASSERT_TRUE(scp::is_scp_like("git@github.com:user/repo.git"));
}

UTEST(scp, is_scp_like_true_without_user) {
    ASSERT_TRUE(scp::is_scp_like("github.com:user/repo.git"));
}

UTEST(scp, is_scp_like_false_for_normal_uri) {
    ASSERT_FALSE(scp::is_scp_like("ssh://git@github.com/user/repo.git"));
    ASSERT_FALSE(scp::is_scp_like("https://github.com/user/repo.git"));
}

UTEST(scp, is_scp_like_false_when_slash_precedes_colon) {
    // Git's own documented disambiguation example: a local path like
    // "./foo:bar" must NOT be mistaken for an scp-like address.
    ASSERT_FALSE(scp::is_scp_like("./foo:bar"));
    ASSERT_FALSE(scp::is_scp_like("/abs/path:something"));
}

UTEST(scp, is_scp_like_false_with_no_colon_at_all) {
    ASSERT_FALSE(scp::is_scp_like("just/a/path"));
    ASSERT_FALSE(scp::is_scp_like("justahost"));
}

UTEST(scp, is_scp_like_true_even_with_digits_after_colon) {
    // Per git's rule there is NO port disambiguation in this syntax - the
    // whole point of "no slash before the first colon" is that it's the
    // only test git applies. "host:22/repo" is scp-like with path
    // "22/repo", not "host:22" + path "/repo".
    ASSERT_TRUE(scp::is_scp_like("host:22/repo"));
}

UTEST(scp, normalize_basic_case) {
    ASSERT_STREQ(scp::normalize("git@github.com:user/repo.git").c_str(),
                 "ssh://git@github.com/user/repo.git");
}

UTEST(scp, normalize_without_user) {
    ASSERT_STREQ(scp::normalize("github.com:user/repo.git").c_str(),
                 "ssh://github.com/user/repo.git");
}

UTEST(scp, normalize_leaves_non_scp_like_input_unchanged) {
    ASSERT_STREQ(scp::normalize("https://github.com/user/repo.git").c_str(),
                 "https://github.com/user/repo.git");
}

UTEST(scp, constructor_parses_scp_like_input_as_ssh_url) {
    scp s("git@github.com:mohammadraziei/liburlparser.git");
    ASSERT_TRUE(s.was_scp_like());
    ASSERT_STREQ(std::string(s.as_url().protocol()).c_str(), "ssh");
    ASSERT_STREQ(std::string(s.as_url().host_text()).c_str(), "github.com");
    ASSERT_STREQ(std::string(s.as_url().userinfo()).c_str(), "git");
    ASSERT_STREQ(s.as_url().abspath().c_str(), "/mohammadraziei/liburlparser.git");
}

UTEST(scp, constructor_accepts_already_normal_url_unchanged) {
    scp s("https://github.com/mohammadraziei/liburlparser.git");
    ASSERT_FALSE(s.was_scp_like());
    ASSERT_STREQ(std::string(s.as_url().protocol()).c_str(), "https");
}

UTEST(scp, implicit_conversion_to_url_works) {
    scp s("git@github.com:user/repo.git");
    const urlparser::url& u = s;
    ASSERT_STREQ(std::string(u.host_text()).c_str(), "github.com");
}
