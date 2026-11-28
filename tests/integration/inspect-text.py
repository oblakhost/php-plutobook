#!/usr/bin/env python3
"""Check that pdftotext returns the characters the text extraction fixture was written with.

A PDF stores glyphs. Copy, search and pdftotext need each glyph's characters as well, and for
a glyph that OpenType substitution chose (tabular figures, ligatures, localized forms) only
the renderer knows them.
"""

import subprocess
import sys
from html.parser import HTMLParser
from pathlib import Path


class Paragraphs(HTMLParser):
    """Collect the expected text of every <p>: its data-expect, or else its content."""

    def __init__(self):
        super().__init__()
        self.expected = []
        self.override = None
        self.content = None

    def handle_starttag(self, tag, attrs):
        if tag == "p":
            self.override = dict(attrs).get("data-expect")
            self.content = []

    def handle_data(self, data):
        if self.content is not None:
            self.content.append(data)

    def handle_endtag(self, tag):
        if tag == "p" and self.content is not None:
            self.expected.append(self.override or "".join(self.content))
            self.content = None


# Poppler wraps right-to-left text in directional marks.
MARKS = dict.fromkeys(map(ord, "‎‏‪‫‬‭‮⁦⁧⁨⁩"))


def words(text):
    return " ".join(text.translate(MARKS).split())


pdf = Path(sys.argv[1])
fixture = Path(__file__).resolve().parent.parent / "fixtures/text/extraction.html"
parser = Paragraphs()
parser.feed(fixture.read_text(encoding="utf-8"))
if len(parser.expected) < 10:
    sys.exit(f"{fixture}: expected at least ten paragraphs")

text_file = pdf.with_suffix(".txt")
subprocess.run(["pdftotext", "-enc", "UTF-8", pdf, text_file], check=True)
text = words(text_file.read_text(encoding="utf-8"))
failures = [line for line in map(words, parser.expected) if line not in text]
if failures:
    print(f"{pdf}: pdftotext did not return {len(failures)} of {len(parser.expected)} paragraphs:",
          file=sys.stderr)
    for line in failures:
        print(f"  expected {line!r}", file=sys.stderr)
    print(f"  extracted {text!r}", file=sys.stderr)
    sys.exit(1)
print(f"{pdf}: pdftotext returned all {len(parser.expected)} paragraphs unchanged")
