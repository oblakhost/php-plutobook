--TEST--
phpinfo() says whether the download timeouts also bound DNS lookups
--FILE--
<?php
// libcurl can only interrupt a DNS lookup with an asynchronous resolver. The binding
// sets CURLOPT_NOSIGNAL, so a synchronous getaddrinfo() call runs to completion.
ob_start();
(new ReflectionExtension('plutobook'))->info();
$info = ob_get_clean();
if (!preg_match('/^DNS lookups => (.*)$/m', $info, $row)) {
    exit("row missing\n");
}
echo in_array($row[1], [
    'bounded by the download timeouts',
    'not bounded by the download timeouts (libcurl resolves host names synchronously)',
    'not used (libcurl is older than 7.68.0)',
], true) ? 'row present' : "unexpected: $row[1]", "\n";
?>
--EXPECT--
row present
