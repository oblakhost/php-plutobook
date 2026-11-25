<?php

declare(strict_types=1);

// All parties, prices and identifiers are synthetic. Keep the body below one FastCGI client record.
echo <<<'HTML'
<!DOCTYPE html>
<html lang="sr-Latn"><head><meta charset="utf-8"><title>Račun INV-2042</title>
<link rel="stylesheet" href="invoice.css"></head><body>
<header><img src="logo.svg" alt="">Radionica Dunav</header>
<h1>Račun INV-2042</h1>
<p>Datum izdavanja 14.03.2026. · Valuta RSD</p>
<h2>Primalac</h2>
<p>Đorđe Živković · Čačak<br><span lang="sr-Cyrl">Радионица Љиљана · Његошева</span></p>
<table><thead><tr><th class="sku">Stavka</th><th>Opis usluge</th><th class="amount">Iznos</th></tr></thead><tbody>
HTML;
for ($item = 1; $item <= 32; ++$item) {
    $description = $item === 16
        ? 'Produženo održavanje računarske opreme sa proverom rezervnih kopija, '
        . 'beleženjem rezultata i završnim izveštajem.'
        : 'Mesečna podrška · Paket održavanja';
    printf(
        '<tr><td>SR-%03d</td><td>%s</td><td class="amount">1.250,00 RSD</td></tr>',
        $item,
        $description
    );
}
echo <<<'HTML'
</tbody></table>
<table class="totals"><tbody>
<tr><td>Osnovica</td><td class="amount">40.000,00 RSD</td></tr>
<tr><td>PDV 20%</td><td class="amount">8.000,00 RSD</td></tr>
<tr><td><b>Za uplatu</b></td><td class="amount"><b>48.000,00 RSD</b></td></tr>
</tbody></table>
<p>Oznake valuta u katalogu · EUR (€) · USD ($)</p>
</body></html>
HTML;
