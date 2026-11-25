#!/usr/bin/env python3
"""Inspect the synthetic invoice and retain its page rasters and Poppler reports."""

import hashlib
import json
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


pdf = Path(sys.argv[1])
require(pdf.read_bytes().startswith(b"%PDF-"), f"{pdf} is not a PDF")


def report(program, suffix, *args):
    output = subprocess.check_output([program, *args, pdf])
    pdf.with_suffix(suffix).write_bytes(output)
    return output.decode("utf-8")


info = report("pdfinfo", ".info.txt")
fonts = report("pdffonts", ".fonts.txt")
for face in ("DejaVuSans", "DejaVuSans-Bold"):
    require(re.search(rf"^(?:[A-Z]{{6}}\+)?{face}\s+.*\byes\s+yes\s+yes\b", fonts, re.M),
            f"{pdf}: expected embedded, subsetted {face} with a Unicode map")

text_file = pdf.with_suffix(".txt")
subprocess.run(["pdftotext", "-enc", "UTF-8", "-layout", pdf, text_file], check=True)
pages = text_file.read_text(encoding="utf-8").rstrip("\f\n").split("\f")
require(len(pages) == 2, f"{pdf}: expected two pages, found {len(pages)}")
require(re.search(r"^Pages:\s+2$", info, re.M), f"{pdf}: unexpected PDF page count")
text = " ".join(" ".join(pages).split())
for phrase in (
    "Račun INV-2042", "Đorđe Živković", "Čačak", "Радионица Љиљана", "Његошева",
    "Osnovica 40.000,00 RSD",
    "PDV 20% 8.000,00 RSD", "Za uplatu 48.000,00 RSD", "EUR (€) · USD ($)",
):
    require(phrase in text, f"{pdf}: missing text {phrase!r}")
description = ("Produženo održavanje računarske opreme sa proverom rezervnih kopija, "
               "beleženjem rezultata i završnim izveštajem.")
# The amount column interrupts the first line in Poppler's layout-preserving text.
require(description in " ".join(text.replace("1.250,00 RSD", "").split()), f"{pdf}: clipped long description")
require(re.findall(r"SR-\d{3}", text) == [f"SR-{i:03d}" for i in range(1, 33)],
        f"{pdf}: missing, duplicated or disordered line items")
require(text.count("1.250,00 RSD") == 32, f"{pdf}: expected 32 priced items")
require("SR-001" in pages[0] and "SR-032" in pages[1] and "48.000,00" in pages[1],
        f"{pdf}: unexpected page breaks")

bbox_file = pdf.with_suffix(".bbox.html")
subprocess.run(["pdftotext", "-bbox", pdf, bbox_file], check=True)
for page in ET.parse(bbox_file).iterfind(".//{*}page"):
    width, height = (float(page.attrib[k]) for k in ("width", "height"))
    require(abs(width - 595.28) < 1 and abs(height - 841.89) < 1, f"{pdf}: expected A4 portrait")
    for word in page.iterfind(".//{*}word"):
        x1, y1, x2, y2 = (float(word.attrib[k]) for k in ("xMin", "yMin", "xMax", "yMax"))
        # The default margin is 15 mm (42.5 pt). Allow font metric overhang.
        require(40 <= x1 < x2 <= width - 40 and 40 <= y1 < y2 <= height - 40,
                f"{pdf}: word outside margins: {word.text!r} at {(x1, y1, x2, y2)}")

# At 72 dpi the first 100 x 100 pt includes the header logo. Its teal is unique to the SVG.
logo = subprocess.check_output(["pdftoppm", "-singlefile", "-r", "72", "-W", "100", "-H", "100", pdf])
require(logo.count(bytes.fromhex("147d74")) > 100, f"{pdf}: relative SVG logo did not paint")
subprocess.run(["pdftoppm", "-png", "-r", "100", pdf, pdf.with_suffix("")], check=True)
digests = [hashlib.sha256(pdf.with_name(f"{pdf.stem}-{i}.png").read_bytes()).hexdigest() for i in (1, 2)]
summary = {"pages": len(pages), "line_items": 32, "page_png_sha256": digests}
pdf.with_suffix(".inspection.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(f"{pdf}: two A4 pages, 32 items, totals, fonts, margins and logo verified")
