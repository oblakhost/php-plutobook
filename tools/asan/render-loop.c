// Render one HTML file repeatedly through the PlutoBook C API, without PHP, and report the
// resident memory growth per render after a warm-up.
//
//   engine=.local/plutobook/0.20.0
//   export PKG_CONFIG_PATH=$engine/lib/pkgconfig FONTCONFIG_FILE=$engine/etc/fonts/fonts.conf
//   cc -O1 -g -o render-loop tools/asan/render-loop.c $(pkg-config --cflags --libs plutobook) \
//     -Wl,-rpath,$engine/lib
//   ./render-loop invoice.html 300
//
// Build with -fsanitize=address and run a few iterations to list unreachable allocations.
#include <plutobook.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static plutobook_stream_status_t count_bytes(void *closure, const char *data, unsigned int length)
{
	*(size_t *)closure += length;
	return PLUTOBOOK_STREAM_STATUS_SUCCESS;
}

static long resident_kib(void)
{
	FILE *status = fopen("/proc/self/status", "r");
	char line[256];
	long kib = 0;
	while (status && fgets(line, sizeof(line), status)) {
		if (!strncmp(line, "VmRSS:", 6)) {
			kib = strtol(line + 6, NULL, 10);
		}
	}
	if (status) {
		fclose(status);
	}
	return kib;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s FILE.html ITERATIONS\n", argv[0]);
		return 2;
	}
	FILE *file = fopen(argv[1], "rb");
	if (!file) {
		perror(argv[1]);
		return 1;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);
	char *html = malloc((size_t)size);
	if (!html || fread(html, 1, (size_t)size, file) != (size_t)size) {
		return 1;
	}
	fclose(file);
	const int iterations = atoi(argv[2]);
	const int warm_up = iterations / 5;
	long baseline = 0;
	for (int i = 0; i < iterations; i++) {
		plutobook_t *book = plutobook_create(PLUTOBOOK_PAGE_SIZE_A4, PLUTOBOOK_PAGE_MARGINS_NORMAL,
		                                     PLUTOBOOK_MEDIA_TYPE_PRINT);
		size_t written = 0;
		plutobook_load_html(book, html, (int)size, "", "", "file:///");
		plutobook_write_to_pdf_stream(book, count_bytes, &written);
		plutobook_destroy(book);
		if (i == warm_up) {
			baseline = resident_kib();
		}
	}
	const long final = resident_kib();
	printf("%d renders: RSS %ld -> %ld KiB after warm-up, %.1f KiB per render\n", iterations,
	       baseline, final, (double)(final - baseline) / (iterations - warm_up - 1));
	free(html);
	return 0;
}
