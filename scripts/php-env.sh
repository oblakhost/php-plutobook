# Source in Bash scripts after setting root, version and flavor.
[[ $version =~ ^8\.[0-9]+\.[0-9]+$ && $flavor =~ ^(nts|debug|ubsan)$ ]] || {
  echo 'Expected an exact PHP release and nts, debug or ubsan' >&2; exit 2;
}
prefix="$root/.local/php/$version/$flavor"
php="$prefix/bin/php"
if [[ ! -x "$php" || $(realpath "$php") != "$php" ]]; then
  echo "Build the private PHP first: scripts/build-php $version $flavor" >&2
  exit 1
fi
export PHPRC="$prefix/etc/php.ini" PHP_INI_SCAN_DIR="$prefix/etc/conf.d"
export PATH="$prefix/bin:$prefix/sbin:$PATH"
export TEST_PHP_EXECUTABLE="$php"
export COMPOSER_HOME="$root/.cache/composer/home" COMPOSER_CACHE_DIR="$root/.cache/composer/cache"
export TMPDIR="$root/build/php/$version/$flavor/tmp"
