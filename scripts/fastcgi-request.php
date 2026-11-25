<?php

declare(strict_types=1);

use Adoy\FastCGI\Client;

// The pinned php-src test client is loaded by the CLI via auto_prepend_file.
$options = getopt('', ['socket:', 'file:', 'html:']);
$socket = $options['socket'] ?? null;
$filename = $options['file'] ?? null;
if (!is_string($socket) || !is_string($filename)) {
    throw new InvalidArgumentException('Expected --socket PATH and --file PATH');
}
$params = [
    'REQUEST_METHOD' => 'GET',
    'SCRIPT_FILENAME' => $filename,
    'SCRIPT_NAME' => '/' . basename($filename),
    'SERVER_PROTOCOL' => 'HTTP/1.1',
];
$body = '';
if (isset($options['html'])) {
    if (!is_string($options['html'])) {
        throw new InvalidArgumentException('Expected --html PATH');
    }
    $body = file_get_contents($options['html']);
    // The pinned php-src client encodes STDIN in a single FastCGI record.
    if ($body === false || $body === '' || strlen($body) > 65535) {
        throw new RuntimeException('Expected 1 to 65535 bytes of HTML');
    }
    $params['REQUEST_METHOD'] = 'POST';
    $params['CONTENT_TYPE'] = 'text/html; charset=utf-8';
    $params['CONTENT_LENGTH'] = (string) strlen($body);
}
$client = new Client('unix://' . $socket, -1);
$response = $client->request($params, $body);
$parts = explode("\r\n\r\n", $response, 2);
if (count($parts) !== 2 || preg_match('/^Status: (?!200\b)/mi', $parts[0])) {
    throw new RuntimeException('Unexpected FastCGI response: ' . substr($response, 0, 512));
}
if ($body !== '' && preg_match('/^Content-Type:[ \t]*application\/pdf\r?$/mi', $parts[0]) !== 1) {
    throw new RuntimeException('Expected PDF response headers: ' . $parts[0]);
}
echo $parts[1];
