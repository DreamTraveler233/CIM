#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONF="${CONF:-$DIR/nginx.dev.conf}"
PID="${PID:-$DIR/nginx.pid}"
NGINX_BIN="${NGINX_BIN:-nginx}"
ACCESS_LOG="${ACCESS_LOG:-$DIR/nginx.access.log}"
ERROR_LOG="${ERROR_LOG:-$DIR/nginx.error.log}"

is_running() {
  [[ -f "$PID" ]] || return 1
  local p
  p=$(cat "$PID" 2>/dev/null || true)
  [[ -n "${p:-}" ]] || return 1
  kill -0 "$p" 2>/dev/null
}

start() {
  if is_running; then
    echo "nginx(dev) already running (pid $(cat "$PID"))"
    exit 0
  fi
  "$NGINX_BIN" -c "$CONF"
  sleep 0.2
  if is_running; then
    echo "nginx(dev) started (pid $(cat "$PID"))"
  else
    echo "Failed to start. Check error log: $ERROR_LOG" >&2
    exit 1
  fi
}

reload() {
  if is_running; then
    kill -HUP "$(cat "$PID")"
    echo "nginx(dev) reloaded"
  else
    echo "nginx(dev) not running. Starting..."
    start
  fi
}

stop() {
  if is_running; then
    kill -QUIT "$(cat "$PID")" || true
    for i in {1..50}; do
      if ! is_running; then break; fi
      sleep 0.1
    done
    if is_running; then
      echo "Graceful stop timed out. Sending TERM..."
      kill -TERM "$(cat "$PID")" || true
    fi
    echo "nginx(dev) stopped"
  else
    echo "nginx(dev) not running"
  fi
}

status() {
  if is_running; then
    echo "nginx(dev) running (pid $(cat "$PID"))"
  else
    echo "nginx(dev) stopped"
  fi
}

testconf() {
  "$NGINX_BIN" -t -c "$CONF" -g "pid $PID;"
}

logs() {
  echo "Tailing logs (Ctrl-C to exit)..."
  touch "$ERROR_LOG" "$ACCESS_LOG" 2>/dev/null || true
  tail -n 100 -F "$ERROR_LOG" "$ACCESS_LOG"
}

usage() {
  cat <<EOF
Usage: $(basename "$0") <command>

Commands:
  start     Start the dev nginx instance
  reload    Reload config (HUP)
  stop      Graceful stop (QUIT), fallback TERM
  status    Show running status
  test      Test config syntax
  logs      Tail error and access logs

Environment overrides:
  NGINX_BIN   Path to nginx binary (default: nginx)
  CONF        Path to config (default: $CONF)
  PID         PID file path (default: $PID)
  ACCESS_LOG  Access log path (default: $ACCESS_LOG)
  ERROR_LOG   Error log path (default: $ERROR_LOG)
EOF
}

cmd=${1:-}
case "$cmd" in
  start)    start;;
  reload)   reload;;
  stop)     stop;;
  status)   status;;
  test|testconf) testconf;;
  logs)     logs;;
  *)        usage; exit 1;;
 esac
