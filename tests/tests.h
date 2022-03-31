#ifndef TESTS_H_
#define TESTS_H_

#include <glib.h>
#include <check.h>
#include <util.h>

Suite *rcl_suite(void);
Suite *cmf_suite(void);
Suite *ballyhoo_message_suite(void);
Suite *ballyhoo_deflate_suite(void);
Suite *ballyhoo_xml_suite(void);

/* helper macros */
#define assert_int_equal(expected, actual) { \
	ck_assert_msg(expected == actual, "Expected '%d' but got '%d'", expected, actual); \
}

#define assert_string_equal(expected, actual) { \
	const gchar *a = actual; \
	ck_assert_msg(purple_strequal(expected, a), "Expected '%s' but got '%s'", expected, a); \
}

#define assert_string_equal_free(expected, actual) { \
	gchar *b = actual; \
	assert_string_equal(expected, b); \
	g_free(b); \
}

#endif
