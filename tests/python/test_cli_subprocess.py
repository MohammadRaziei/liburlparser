from __future__ import annotations

import json
import subprocess
import sys

import pytest


def run_cli_command(command):
    """Run the CLI command and return the output"""
    result = subprocess.run(
        command,
        capture_output = True,
        text=True,
        check=False
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
