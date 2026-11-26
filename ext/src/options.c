// SPDX-License-Identifier: Apache-2.0
#include "binding.h"
#include <float.h>
#include "Zend/zend_strtod.h"

static bool php_plutobook_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

static bool php_plutobook_margin_length(const char **cursor, float *points)
{
	static const struct {
		const char *name;
		float points;
	} units[] = {{"pt", 1},
	             {"mm", PLUTOBOOK_UNITS_MM},
	             {"cm", PLUTOBOOK_UNITS_CM},
	             {"in", PLUTOBOOK_UNITS_IN},
	             {"px", PLUTOBOOK_UNITS_PX},
	             {"pc", 12}};
	const char *end;
	double n = zend_strtod(*cursor, &end);
	if (end == *cursor || !zend_finite(n) || n < 0) {
		return false;
	}
	bool has_unit = false;
	for (size_t i = 0; i < sizeof(units) / sizeof(units[0]); i++) {
		if (!strncasecmp(end, units[i].name, 2)) {
			n *= units[i].points;
			end += 2;
			has_unit = true;
			break;
		}
	}
	if ((!has_unit && n != 0) || (*end && !php_plutobook_space(*end)) || n > FLT_MAX) {
		return false;
	}
	*points = (float)n;
	*cursor = end;
	return true;
}

static bool php_plutobook_parse_margins(zend_string *value, plutobook_page_margins_t *result)
{
	float lengths[4];
	size_t count = 0;
	const char *cursor = ZSTR_VAL(value);
	while (*cursor) {
		if (php_plutobook_space(*cursor)) {
			cursor++;
			continue;
		}
		if (count == 4 || !php_plutobook_margin_length(&cursor, &lengths[count])) {
			return false;
		}
		count++;
	}
	if (!count || cursor != ZSTR_VAL(value) + ZSTR_LEN(value)) {
		return false;
	}
	result->top = lengths[0];
	result->right = count > 1 ? lengths[1] : result->top;
	result->bottom = count > 2 ? lengths[2] : result->top;
	result->left = count > 3 ? lengths[3] : result->right;
	return true;
}

bool php_plutobook_page_options(zend_string *paper, zend_string *orientation, zend_string *margins,
                                plutobook_page_size_t *size, plutobook_page_margins_t *insets)
{
	const struct {
		const char *name;
		plutobook_page_size_t size;
	} papers[] = {{"A3", PLUTOBOOK_PAGE_SIZE_A3},       {"A4", PLUTOBOOK_PAGE_SIZE_A4},
	              {"A5", PLUTOBOOK_PAGE_SIZE_A5},       {"B4", PLUTOBOOK_PAGE_SIZE_B4},
	              {"B5", PLUTOBOOK_PAGE_SIZE_B5},       {"Letter", PLUTOBOOK_PAGE_SIZE_LETTER},
	              {"Legal", PLUTOBOOK_PAGE_SIZE_LEGAL}, {"Ledger", PLUTOBOOK_PAGE_SIZE_LEDGER}};
	*size = PLUTOBOOK_PAGE_SIZE_A4;
	if (paper) {
		size_t i = 0;
		while (i < sizeof(papers) / sizeof(papers[0]) &&
		       zend_binary_strcasecmp(ZSTR_VAL(paper), ZSTR_LEN(paper), papers[i].name,
		                              strlen(papers[i].name)) != 0) {
			i++;
		}
		if (i == sizeof(papers) / sizeof(papers[0])) {
			zend_argument_value_error(1, "must be a supported paper size");
			return false;
		}
		*size = papers[i].size;
	}
	if (orientation && zend_string_equals_literal_ci(orientation, "landscape")) {
		float width = size->width;
		size->width = size->height;
		size->height = width;
	} else if (orientation && !zend_string_equals_literal_ci(orientation, "portrait")) {
		zend_argument_value_error(2, "must be portrait or landscape");
		return false;
	}
	float normal = 15 * PLUTOBOOK_UNITS_MM;
	*insets = PLUTOBOOK_MAKE_PAGE_MARGINS(normal, normal, normal, normal);
	if ((margins && !php_plutobook_parse_margins(margins, insets)) ||
	    insets->top + insets->bottom >= size->height ||
	    insets->left + insets->right >= size->width) {
		zend_argument_value_error(
		    3, "must specify one to four nonnegative CSS lengths leaving a printable area");
		return false;
	}
	return true;
}
