--TEST--
Internal network addresses are refused by default, checked on the resolved address of each connection
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require __DIR__ . '/network.inc';
use PlutoBook\Document;
$doc = new Document();
// Every request stops before connecting, so no traffic leaves the process.
$urls = [
    'http://127.0.0.1:9/a.png',
    'http://[::1]:9/a.png',
    'http://169.254.169.254/latest/meta-data/',
    'http://10.1.2.3/a.png',
    'http://192.168.0.10/a.png',
    'http://[::ffff:172.16.0.1]/a.png',
    'http://[fd00::1]/a.png',
    'http://0.0.0.0:9/a.png',
];
foreach ($urls as $url) {
    echo str_pad($url, 42), plutobook_test_image($doc, $url, ['*']), "\n";
}
// A name that resolves to loopback is refused on its resolved address.
echo str_pad('http://localhost:9/a.png', 42), plutobook_test_image($doc, 'http://localhost:9/a.png', ['*'], detail: false), "\n";
// Only php.ini, .user.ini or server configuration can allow internal addresses.
var_dump(ini_set('plutobook.allow_private_addresses', '1'));
?>
--EXPECT--
http://127.0.0.1:9/a.png                  blocked-address (connection to internal address 127.0.0.1 refused)
http://[::1]:9/a.png                      blocked-address (connection to internal address ::1 refused)
http://169.254.169.254/latest/meta-data/  blocked-address (connection to internal address 169.254.169.254 refused)
http://10.1.2.3/a.png                     blocked-address (connection to internal address 10.1.2.3 refused)
http://192.168.0.10/a.png                 blocked-address (connection to internal address 192.168.0.10 refused)
http://[::ffff:172.16.0.1]/a.png          blocked-address (connection to internal address ::ffff:172.16.0.1 refused)
http://[fd00::1]/a.png                    blocked-address (connection to internal address fd00::1 refused)
http://0.0.0.0:9/a.png                    blocked-address (connection to internal address 0.0.0.0 refused)
http://localhost:9/a.png                  blocked-address
bool(false)
