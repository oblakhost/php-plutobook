# Run one private worker. Additional arguments set PHP options for that worker.
start_private_fpm() {
  local root=$1 prefix=$2 run=$3 archive socket_dir
  shift 3
  client="$run/fcgi.inc"
  archive=$("$root/scripts/fetch-source" php-8.2.36.tar.xz)
  tar -xOf "$archive" php-8.2.36/sapi/fpm/tests/fcgi.inc > "$client"

  # A deeply nested checkout can exceed the Unix socket pathname limit.
  socket_dir=$(mktemp -d /tmp/php-plutobook-fpm.XXXXXXXX)
  socket="$socket_dir/fpm.sock"
  cat > "$run/fpm.conf" <<CONF
[global]
pid = "$run/fpm.pid"
error_log = "$run/fpm.log"
[private]
listen = "$socket"
listen.mode = 0600
pm = static
pm.max_children = 1
CONF
  if [[ -n ${FONTCONFIG_FILE:-} ]]; then
    printf 'env[FONTCONFIG_FILE] = "%s"\n' "$FONTCONFIG_FILE" >> "$run/fpm.conf"
  fi
  "$prefix/sbin/php-fpm" "$@" --nodaemonize --force-stderr --fpm-config "$run/fpm.conf" >"$run/process.log" 2>&1 &
  fpm_pid=$!
  trap 'kill "$fpm_pid" 2>/dev/null || true; wait "$fpm_pid" 2>/dev/null || true; rm -f "$socket"; rmdir "${socket%/*}"' EXIT
  for ((i=0; i<100; i++)); do
    [[ -S "$socket" ]] && return
    kill -0 "$fpm_pid" 2>/dev/null || { cat "$run/process.log" >&2; return 1; }
    sleep 0.05
  done
  echo 'Private FPM socket did not become ready' >&2
  cat "$run/process.log" >&2
  return 1
}
