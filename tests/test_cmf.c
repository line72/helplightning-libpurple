#include "tests.h"

#include "../libcmf.h"

#include <stdio.h>

START_TEST(test_cmf_decode_header) {
  char in[4096];
  FILE *f = fopen("message.bin", "rb");
  size_t r = fread(in, 1, 4096, f);
  fclose(f);
  
  ck_assert(r == 680);

  CMFDecodedChunk *cmf = g_new0(CMFDecodedChunk, 1);
  int success = libcmf_decode(in, 680, cmf);

  ck_assert(success == 1);

  ck_assert(cmf->size == 672);
  ck_assert(cmf->begin == TRUE);
  ck_assert(cmf->end == TRUE);
  ck_assert(cmf->id == 1);
  
  g_free(cmf);
}

Suite *cmf_suite(void) {
  Suite *s = suite_create("CMF Suite");
  TCase *tc = NULL;

  tc = tcase_create("Decode");
  tcase_add_test(tc, test_cmf_decode_header);
  suite_add_tcase(s, tc);

  return s;
}
