--TEST--
HTTPS assets require a trusted certificate, and redirects never downgrade to http
--SKIPIF--
<?php
require __DIR__ . '/network.inc';
if (!is_readable(plutobook_test_font())) {
    die('skip FONTCONFIG_FILE must point at the pinned DejaVu fonts');
}
if (!extension_loaded('openssl')) {
    die('skip the fixture server needs ext/openssl');
}
?>
--INI--
plutobook.allow_private_addresses=1
plutobook.http_timeout=5
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require __DIR__ . '/network.inc';
use PlutoBook\Document;
// A self-signed certificate for 127.0.0.1, written next to the test with its key.
$pem = __DIR__ . '/network-tls-' . uniqid() . '.pem';
$config = "$pem.cnf";
file_put_contents($config, "[req]\ndistinguished_name = dn\n[dn]\n[ext]\n"
    . "subjectAltName = IP:127.0.0.1\nbasicConstraints = critical,CA:TRUE\n");
$settings = ['config' => $config, 'digest_alg' => 'sha256', 'x509_extensions' => 'ext'];
$key = openssl_pkey_new(['private_key_type' => OPENSSL_KEYTYPE_EC, 'curve_name' => 'prime256v1']);
$csr = openssl_csr_new(['commonName' => '127.0.0.1'], $key, $settings);
$certificate = openssl_csr_sign($csr, null, $key, 1, $settings);
openssl_x509_export($certificate, $certificateText);
openssl_pkey_export($key, $keyText, null, ['config' => $config]);
file_put_contents($pem, $certificateText . $keyText);

$port = plutobook_test_server('tls', $pem);
$origin = "https://127.0.0.1:$port";
$doc = new Document();
$hosts = ['127.0.0.1'];
echo str_pad('untrusted certificate', 26), plutobook_test_image($doc, "$origin/img/dot.png", $hosts, detail: false), "\n";
ini_set('plutobook.http_ca_file', $pem);
echo str_pad('trusted via http_ca_file', 26), plutobook_test_image($doc, "$origin/img/dot.png", $hosts), "\n";
echo str_pad('redirect to http', 26), plutobook_test_image($doc, "$origin/redirect/downgrade", $hosts), "\n";
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/network-tls-*.pem*') as $file) {
    unlink($file);
}
?>
--EXPECT--
untrusted certificate     tls-error
trusted via http_ca_file  loaded
redirect to http          redirect-error (redirect from https to http)
