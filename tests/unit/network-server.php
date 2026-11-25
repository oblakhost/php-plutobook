<?php

// HTTP/1.1 fixture server for the network PHPTs, over plain TCP or TLS.
// Usage: php -n network-server.php tcp|tls FONT_FILE [CERT_AND_KEY_PEM]
// Prints the listening port, then answers one request per connection until terminated.

declare(strict_types=1);

$transport = $argv[1] ?? 'tcp';
$font = $argv[2] ?? '';
$options = $transport === 'tls' ? ['ssl' => ['local_cert' => $argv[3] ?? '']] : [];
$server = stream_socket_server(
    "$transport://127.0.0.1:0",
    $errno,
    $errstr,
    STREAM_SERVER_BIND | STREAM_SERVER_LISTEN,
    stream_context_create($options),
);
if ($server === false) {
    fwrite(STDERR, "$errstr\n");
    exit(1);
}
$name = (string) stream_socket_get_name($server, false);
$port = (int) substr($name, (int) strrpos($name, ':') + 1);
echo $port, "\n";

$png = (string) base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1t'
    . 'AVBkfyF3QAAAABJRU5ErkJggg==');
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
    . '<rect width="10" height="10" fill="#00ff00"/></svg>';
$css = 'p { color: #0000ff } h1 { font-family: Remote; font-weight: normal }'
    . ' @font-face { font-family: Remote; src: url(fonts/face.ttf) }';
$away = "http://localhost:$port/img/dot.png";

/** @return array{string, array<string, string>, string} status line, headers, body */
$route = static fn (string $path): array => match ($path) {
    '/style.css' => ['200 OK', ['Content-Type' => 'text/css; charset=utf-8'], $css],
    '/fonts/face.ttf' => ['200 OK', ['Content-Type' => 'font/ttf'], (string) file_get_contents($font)],
    '/img/dot.png' => ['200 OK', ['Content-Type' => 'image/png'], $png],
    '/img/logo.svg' => ['200 OK', ['Content-Type' => 'image/svg+xml'], $svg],
    '/empty.png' => ['200 OK', ['Content-Type' => 'image/png'], ''],
    '/big.png' => ['200 OK', ['Content-Type' => 'image/png'], str_repeat('x', 65536)],
    '/redirect/dot.png' => ['302 Found', ['Location' => '/img/dot.png'], ''],
    '/redirect/away' => ['302 Found', ['Location' => $away], ''],
    '/redirect/loop' => ['302 Found', ['Location' => '/redirect/loop'], ''],
    '/redirect/ftp' => ['302 Found', ['Location' => 'ftp://127.0.0.1/dot.png'], ''],
    '/redirect/downgrade' => ['302 Found', ['Location' => 'http://127.0.0.1:1/img/dot.png'], ''],
    default => ['404 Not Found', ['Content-Type' => 'text/plain'], 'not found'],
};

// Each test makes far fewer requests. The bound only ends an orphaned server.
for ($served = 0; $served < 1000; $served++) {
    // A client that rejects the TLS certificate makes accept fail; keep serving.
    $client = @stream_socket_accept($server, -1);
    if ($client === false) {
        continue;
    }
    $request = '';
    while (!str_contains($request, "\r\n\r\n") && ($line = fgets($client)) !== false) {
        $request .= $line;
    }
    // Route on the path alone, so "?n" can make otherwise identical URLs distinct.
    $path = strtok(explode(' ', $request)[1] ?? '/', '?') ?: '/';
    if ($path === '/slow.png') {
        sleep(3);
    }
    if ($path === '/chunked.png') {
        fwrite($client, "HTTP/1.1 200 OK\r\nConnection: close\r\nTransfer-Encoding: chunked\r\n\r\n");
        for ($i = 0; $i < 16; $i++) {
            fwrite($client, "1000\r\n" . str_repeat('x', 4096) . "\r\n");
        }
        fwrite($client, "0\r\n\r\n");
        fclose($client);
        continue;
    }
    [$status, $headers, $body] = $route($path);
    $head = "HTTP/1.1 $status\r\nConnection: close\r\nContent-Length: " . strlen($body) . "\r\n";
    foreach ($headers as $header => $value) {
        $head .= "$header: $value\r\n";
    }
    fwrite($client, "$head\r\n$body");
    fclose($client);
}
