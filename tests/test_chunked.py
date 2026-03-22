#!/usr/bin/env python3
"""Integration tests for chunked transfer encoding + SIGPIPE fix."""

import os
import signal
import socket
import subprocess
import sys
import time

SERVER_BIN = "./webserv"
CONFIG = "tests/test_chunked.conf"
PORT = 19191
HOST = "127.0.0.1"

PASS = 0
FAIL = 0


def ok(name):
    global PASS
    PASS += 1
    print(f"  \033[1;32m[PASS]\033[0m {name}")


def fail(name, reason=""):
    global FAIL
    FAIL += 1
    msg = f"  \033[1;31m[FAIL]\033[0m {name}"
    if reason:
        msg += f": {reason}"
    print(msg)


def wait_for_server(timeout=5):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            s = socket.create_connection((HOST, PORT), timeout=0.5)
            s.close()
            return True
        except OSError:
            time.sleep(0.1)
    return False


def send_raw(data, timeout=5):
    """Send raw bytes to the server, return response bytes."""
    s = socket.create_connection((HOST, PORT), timeout=timeout)
    s.sendall(data)
    chunks = []
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            chunks.append(chunk)
    except socket.timeout:
        pass
    s.close()
    return b"".join(chunks)


def chunked_post(path, body, timeout=5):
    """Send a chunked POST request, return raw response."""
    body_bytes = body if isinstance(body, bytes) else body.encode()
    chunk_size = len(body_bytes)
    chunked_body = f"{chunk_size:x}\r\n".encode() + body_bytes + b"\r\n0\r\n\r\n"

    request = (
        f"POST {path} HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    ).encode() + chunked_body

    return send_raw(request, timeout)


def get(path, timeout=5):
    """Send a GET request, return raw response."""
    request = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    ).encode()
    return send_raw(request, timeout)


def status_code(response):
    try:
        first_line = response.split(b"\r\n")[0].decode()
        return int(first_line.split()[1])
    except Exception:
        return -1


def run_tests(proc):
    # Test 1: simple GET works
    resp = get("/")
    code = status_code(resp)
    if code == 200:
        ok("GET / returns 200")
    else:
        fail("GET / returns 200", f"got {code}")

    # Test 2: chunked POST to non-CGI endpoint — server must survive
    resp = chunked_post("/", "hello world")
    code = status_code(resp)
    if code in (200, 201, 204):
        ok("chunked POST to non-CGI endpoint returns 2xx")
    else:
        fail("chunked POST to non-CGI endpoint returns 2xx", f"got {code}")

    # Test 3: server still alive after chunked POST (SIGPIPE would have killed it)
    if proc.poll() is None:
        ok("server still running after chunked POST (no SIGPIPE crash)")
    else:
        fail("server still running after chunked POST (no SIGPIPE crash)", "process died")
        return  # no point continuing

    # Test 4: GET still works after chunked POST
    resp = get("/")
    code = status_code(resp)
    if code == 200:
        ok("GET / still works after chunked POST")
    else:
        fail("GET / still works after chunked POST", f"got {code}")

    # Test 5: multiple chunked POSTs in sequence
    all_ok = True
    for i in range(3):
        resp = chunked_post("/", f"request {i}")
        code = status_code(resp)
        if code not in (200, 201, 204):
            all_ok = False
            fail(f"chunked POST #{i} returns 2xx", f"got {code}")
    if all_ok:
        ok("3 sequential chunked POSTs all return 2xx")

    # Test 6: server alive after multiple chunked POSTs
    if proc.poll() is None:
        ok("server alive after 3 sequential chunked POSTs")
    else:
        fail("server alive after 3 sequential chunked POSTs", "process died")
        return

    # Test 7: chunked POST with large body (> BUFFER_SIZE = 1024)
    large_body = "x" * 8192
    resp = chunked_post("/", large_body)
    code = status_code(resp)
    if code in (200, 201, 204):
        ok("chunked POST with 8KB body returns 2xx")
    else:
        fail("chunked POST with 8KB body returns 2xx", f"got {code}")

    # Test 8: server alive after large chunked POST
    if proc.poll() is None:
        ok("server alive after large chunked POST")
    else:
        fail("server alive after large chunked POST", "process died")
        return

    # Test 9: chunked POST to CGI endpoint
    resp = chunked_post("/cgi/echo.cgi", "ping")
    code = status_code(resp)
    if proc.poll() is None:
        ok("server alive after chunked POST to CGI endpoint")
    else:
        fail("server alive after chunked POST to CGI endpoint", "process died")
        return

    if code > 0:  # any HTTP response means server is alive
        ok("chunked POST to CGI returns a valid HTTP response")
    else:
        fail("chunked POST to CGI returns a valid HTTP response", f"got {code}")


def main():
    # Verify binary exists
    if not os.path.isfile(SERVER_BIN):
        print(f"ERROR: {SERVER_BIN} not found, run 'make' first")
        sys.exit(1)

    proc = subprocess.Popen(
        [SERVER_BIN, CONFIG],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        if not wait_for_server():
            print("ERROR: server did not start in time")
            proc.terminate()
            sys.exit(1)

        print(f"\nRunning chunked transfer tests on {HOST}:{PORT}...\n")
        run_tests(proc)

    finally:
        if proc.poll() is None:
            proc.terminate()
            proc.wait()

    print(f"\n  Passed: {PASS}  Failed: {FAIL}\n")
    sys.exit(0 if FAIL == 0 else 1)


if __name__ == "__main__":
    main()
