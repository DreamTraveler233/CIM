#!/usr/bin/env bash
set -euo pipefail

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-9503}"
BASE="http://${HOST}:${PORT}"

function req() {
  local path="$1"
  echo "==> GET ${BASE}${path}"
  http_code=$(curl -s -o /tmp/_resp.txt -w "%{http_code}" "${BASE}${path}" || true)
  echo "<== ${http_code} $(cat /tmp/_resp.txt)"
  [[ "${http_code}" == "200" ]] || exit 1
}

req /healthz
req /readyz
req /api/v1/ping

echo "Smoke OK"
