from __future__ import annotations

import json
import subprocess
import sys

import pytest


def run_cli_command(command, timeout=30):
    """Run the CLI command and return the output.

    A timeout is set deliberately: if the subprocess ever hangs (e.g. due to
    environment-specific process-creation issues), this test should fail
    with a clear TimeoutExpired error instead of hanging the whole pytest
    run indefinitely, forcing a manual Ctrl+C.
    """
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as e:
        pytest.fail(
            f"CLI command timed out after {timeout}s: {command!r}\n"
            f"partial stdout: {e.stdout!r}\npartial stderr: {e.stderr!r}"
        )
    return result.stdout, result.stderr, result.returncode


@pytest.mark.parametrize(("host","expected"), [
    ("www.example.com", {"domain": "example", "suffix": "com", "subdomain": "www"}),
    ("example.co.uk", {"domain": "example", "suffix": "co.uk", "subdomain": ""}),
    ("sub.domain.example.org", {"domain": "example", "suffix": "org", "subdomain": "sub.domain"}),
])
def test_host_parsing(host, expected):
    """Test parsing different hosts"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--host", host
    ])
    assert returncode == 0
    assert stderr == ""
    parsed = json.loads(stdout)
    for key, value in expected.items():
        assert parsed[key] == value


@pytest.mark.parametrize(("url","expected"), [
    (
        "https://www.example.com/path?query=value#fragment",
        {
            "protocol": "https",
            "host": {"domain": "example", "suffix": "com", "subdomain": "www"}
        }
    ),
    (
        "http://example.co.uk/page",
        {
            "protocol": "http",
            "host": {"domain": "example", "suffix": "co.uk", "subdomain": ""}
        }
    ),
])
def test_url_parsing(url, expected):
    """Test parsing different URLs"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--url", url
    ])
    assert returncode == 0
    assert stderr == ""
    parsed = json.loads(stdout)
    assert parsed["protocol"] == expected["protocol"]
    for key, value in expected["host"].items():
        assert parsed["host"][key] == value


@pytest.mark.parametrize(("args","expected_output"), [
    (["--host", "www.example.com", "--parts", "domain", "suffix"], "example com"),
    (["--url", "https://www.example.com/path", "--parts", "protocol", "domain"], "https example"),
    (["--url", "https://sub.example.org", "--parts", "subdomain", "suffix"], "sub org"),
])
def test_specific_parts(args, expected_output):
    """Test extracting specific parts from hosts and URLs"""
    command = [sys.executable, "-m", "liburlparser", *args]
    stdout, stderr, returncode = run_cli_command(command)
    assert returncode == 0
    assert stderr == ""
    assert stdout.strip() == expected_output


@pytest.mark.parametrize(("host","expected"), [
    (
        "192.168.1.1",
        {"type": "ipv4", "str": "192.168.1.1", "as_int": 3232235777},
    ),
    (
        "2001:db8::1",
        {"type": "ipv6", "str": "2001:db8::1"},
    ),
])
def test_host_parsing_ip_addresses(host, expected):
    """--host must classify IP addresses as ipv4/ipv6 (via parse_host), not
    run them through domain-name/PSL logic the way it used to - see
    ARCHITECTURE.md section 10."""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--host", host
    ])
    assert returncode == 0
    assert stderr == ""
    parsed = json.loads(stdout)
    for key, value in expected.items():
        assert parsed[key] == value


def test_url_parsing_ipv6_with_port():
    """--url with a bracketed IPv6 host + explicit port must classify the
    host as ipv6 without the port digits leaking into it."""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--url", "http://[2001:db8::1]:8080/x"
    ])
    assert returncode == 0
    assert stderr == ""
    parsed = json.loads(stdout)
    assert parsed["host"]["type"] == "ipv6"
    assert parsed["host"]["str"] == "2001:db8::1"


def test_invalid_part():
    """Test error handling for invalid part"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--url", "https://www.example.com", "--parts", "invalid_part"
    ])
    assert returncode == 1
    assert "Error: Invalid part" in stderr


def test_no_args():
    """Test behavior with no arguments"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser"
    ])
    # This should show help and exit with code 0
    assert "usage:" in stdout


def test_version():
    """Test version flag"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "-v"
    ])
    assert returncode == 0
    assert stderr == ""
    # Version should be in the format x.y.z
    assert stdout.strip().count(".") >= 1


def test_doc():
    """Test doc flag"""
    stdout, stderr, returncode = run_cli_command([
        sys.executable, "-m", "liburlparser", "--doc"
    ])
    assert returncode == 0
    assert stderr == ""
    # Doc should contain some text
    assert len(stdout.strip()) > 0


if __name__ == "__main__":
    pytest.main(["-xvs", __file__])
