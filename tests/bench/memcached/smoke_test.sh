#!/usr/bin/env bash
#
# Smoke test for the persistent-memory memcached (memcached_mtm).
#
# Boots the server, exercises the persistent transactional hashtable with a
# set/get round-trip over the text protocol, then shuts it down. Verifies the
# build links and the mnemosyne runtime initialises and serves requests.
#
# Usage: smoke_test.sh <path-to-build-dir>   (defaults to ./build)
set -euo pipefail

SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../src" && pwd)"
# Build dir: arg 1, else $SRC_DIR/build. Resolved to an absolute path.
BUILD_DIR="$(cd "${1:-${SRC_DIR}/build}" && pwd)"
MC="${BUILD_DIR}/tests/bench/memcached/memcached_mtm"
KEY=smoke
VAL=barbaz

if [[ ! -x "${MC}" ]]; then
    echo "smoke_test: ${MC} not found" >&2
    exit 1
fi

# Pick a free ephemeral port so the test never collides with a squatter.
PORT="$(python3 -c 'import socket;s=socket.socket();s.bind(("127.0.0.1",0));print(s.getsockname()[1]);s.close()')"

mkdir -p /dev/shm/psegments
rm -rf /dev/shm/psegments/*

cleanup() { [[ -n "${MCPID:-}" ]] && kill "${MCPID}" 2>/dev/null || true; }
trap cleanup EXIT

# mnemosyne reads mnemosyne.ini from the working dir; keep the heap region small.
cd "${SRC_DIR}"
MNEMOSYNE_PHEAP_SIZE_MB=32 LD_LIBRARY_PATH="${BUILD_DIR}:${LD_LIBRARY_PATH:-}" \
    "${MC}" -p "${PORT}" -t 1 -m 16 -u "$(id -un)" \
    >/tmp/memcached_smoke.log 2>&1 &
MCPID=$!

# Wait for the server to accept connections (max ~10s).
ready=0
for _ in $(seq 1 50); do
    if python3 -c "import socket;socket.create_connection(('127.0.0.1',${PORT}),timeout=0.5).close()" 2>/dev/null; then
        ready=1
        break
    fi
    if ! kill -0 "${MCPID}" 2>/dev/null; then
        echo "smoke_test: server exited early (failed to start/bind)" >&2
        tail -20 /tmp/memcached_smoke.log >&2
        exit 1
    fi
    sleep 0.2
done
if [[ "${ready}" -ne 1 ]]; then
    echo "smoke_test: server never became ready on port ${PORT}" >&2
    tail -20 /tmp/memcached_smoke.log >&2
    exit 1
fi

# Confirm it is *our* server (still alive) immediately before issuing requests.
if ! kill -0 "${MCPID}" 2>/dev/null; then
    echo "smoke_test: server (pid ${MCPID}) not alive before set/get" >&2
    exit 1
fi

python3 - "${PORT}" "${KEY}" "${VAL}" <<'PY'
import socket, sys, time
port, key, val = int(sys.argv[1]), sys.argv[2], sys.argv[3]
s = socket.create_connection(("127.0.0.1", port), timeout=5)
def rpc(b):
    s.sendall(b); time.sleep(0.1); return s.recv(4096)
stored = rpc(f"set {key} 0 0 {len(val)}\r\n{val}\r\n".encode())
assert stored.strip() == b"STORED", f"set failed: {stored!r}"
got = rpc(f"get {key}\r\n".encode())
assert val.encode() in got, f"get returned wrong value: {got!r}"
print(f"smoke_test: set/get round-trip OK ({key}={val})")
s.close()
PY

echo "smoke_test: PASS"
