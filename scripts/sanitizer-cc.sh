# Source in Bash scripts that build the asan flavor. PHP and the extension must use the same
# compiler: GCC when its libasan runtime is installed, otherwise Clang's bundled runtime.
asan_compiler() {
  local cc
  for cc in "${CC:-gcc}" clang; do
    if printf 'int main(void) { return 0; }\n' |
      "$cc" -x c -fsanitize=address -o /dev/null - >/dev/null 2>&1; then
      echo "$cc"
      return
    fi
  done
  echo 'No compiler can link AddressSanitizer programs (install libasan or clang)' >&2
  return 1
}
