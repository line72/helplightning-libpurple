#include "tests.h"

#include "../libballyhoo_deflate.h"

#include <stdio.h>

START_TEST(test_ballyhoo_deflate_decompress) {
  char in[4096];
  FILE *f = fopen("message.bin", "rb");
  size_t r = fread(in, 1, 4096, f);
  fclose(f);
  
  ck_assert(r == 680);

  // seek past the first 8 + 16 bytes (cmf + rcl header)
  // + the ballyhoo message headers (65)
  char *i = in + 8 + 16 + 65;
  int size = 680 - 8 - 16 - 65; // 591 bytes left

  size_t output_length;
  gpointer xmlrpc = libballyhoo_deflate_decompress(i, size, &output_length);
  ck_assert(xmlrpc != NULL);

  assert_string_equal_free("<?xml version=\"1.0\" encoding=\"UTF-8\"?><methodResponse><params><param><value><array><data><value><string>eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2NDkzNDY1NDUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiZDFmOTQ3YTAtMmM2OC00MWQyLWE5N2MtZjQ1MTVjYzFlZjI0IiwibWV0YSI6e30sIm5iZiI6MTY0OTM0NjQ4NCwicGVtIjp7InVzZXIiOjE1ODUyNjcyMTIxODU1Mzk4NTB9LCJzdWIiOiJVc2VyOjM5MDk0IiwidHlwIjoiYWNjZXNzIiwidmVyIjoiMTAwIn0.XqLRkFgOTCdcJA1pT0WDtwXzgCiwMQKKOKeY8x3Qb88</string></value><value><string>eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2ODA4ODI0ODUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiNWM5OTljMWMtZmQwZi00YjljLWEwOTUtY2U2OWE3YTViODNkIiwibmJmIjoxNjQ5MzQ2NDg0LCJwZW0iOnsidXNlcl9yZWZyZXNoIjoxNTg1MjY3MjEyMTg1NTM5ODUwfSwic3ViIjoiVXNlclJlZnJlc2g6MzkwOTQiLCJ0eXAiOiJhY2Nlc3MiLCJ2ZXIiOiIxMDAifQ.4Wq_XQP9HQkK4MD8p4ylPthyGFZ_Fvtih0yHukWjADY</string></value></data></array></value></param></params></methodResponse>", xmlrpc);
  
  ck_assert(output_length == 961);
}

Suite *ballyhoo_deflate_suite(void) {
  Suite *s = suite_create("BALLYHOO_deflate Suite");
  TCase *tc = NULL;

  tc = tcase_create("Decode");
  tcase_add_test(tc, test_ballyhoo_deflate_decompress);
  suite_add_tcase(s, tc);

  return s;
}
