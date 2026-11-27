# Shared by the cPanel packages' install and removal scripts: scripts/build-packages puts
# it at the top of each one. cPanel's own EasyApache 4 hooks refresh CageFS and restart
# PHP-FPM and Apache only after transactions with ea-* packages, so these packages run
# the same commands in the same order themselves. Nothing here may fail the package
# operation, and without cPanel or CloudLinux nothing here prints anything.
PLUTOBOOK_FLAG=/run/oblak-ea-plutobook.refresh
PLUTOBOOK_TRIGGER=oblak-ea-plutobook-refresh

plutobook_run() {
    "$@" > /dev/null 2>&1 || echo "PlutoBook: warning: $* failed; run it by hand" >&2
}

# CloudLinux copies php.d and the fonts listed in /etc/cagefs/conf.d into every cage.
# Same test as cPanel's hook: CageFS is in use when cagefs.mp lists a mount.
plutobook_cagefs_update() {
    [ -x /usr/sbin/cagefsctl ] || return 0
    grep -Eq '^[[:space:]]*[!@#]?/[[:alnum:]_]' /etc/cagefs/cagefs.mp 2> /dev/null || return 0
    echo "PlutoBook: updating CloudLinux CageFS"
    plutobook_run /usr/sbin/cagefsctl --update --force-update
}

# PHP-FPM pools reload php.d on restart; an Apache restart replaces mod_lsapi's lsphp
# processes, which otherwise keep the old configuration.
plutobook_restart_services() {
    for plutobook_service in apache_php_fpm:PHP-FPM httpd:Apache; do
        plutobook_script=/usr/local/cpanel/scripts/restartsrv_${plutobook_service%%:*}
        [ -x "$plutobook_script" ] || continue
        echo "PlutoBook: restarting ${plutobook_service#*:}"
        plutobook_run "$plutobook_script"
    done
}

# Installing several packages in one transaction refreshes once, after all of them:
# each package records that a refresh is due, and the first end-of-transaction hook
# does it. Without /run, refresh straight away.
plutobook_refresh_due() {
    if ! : > "$PLUTOBOOK_FLAG" 2> /dev/null; then
        plutobook_cagefs_update
        plutobook_restart_services
    fi
}

plutobook_refresh_if_due() {
    [ -e "$PLUTOBOOK_FLAG" ] || return 0
    rm -f "$PLUTOBOOK_FLAG"
    plutobook_cagefs_update
    plutobook_restart_services
}

# PIE writes its own ini (80-plutobook.ini), and PHP warns on every start when two
# files load the module.
plutobook_check_duplicate() {
    for plutobook_ini in "/opt/cpanel/ea-php$1/root/etc/php.d/"*.ini; do
        [ -f "$plutobook_ini" ] && [ "${plutobook_ini##*/}" != 20-plutobook.ini ] || continue
        if grep -Eq '^[[:space:]]*extension[[:space:]]*=[[:space:]]*"?plutobook(\.so)?"?[[:space:]]*$' "$plutobook_ini"; then
            echo "PlutoBook: warning: $plutobook_ini also loads plutobook; remove it" >&2
        fi
    done
}
