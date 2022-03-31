#include "tests.h"

#include "../librcl.h"

#include <stdio.h>

START_TEST(test_rcl_decode_header) {
  char in[4096];
  FILE *f = fopen("message.bin", "rb");
  size_t r = fread(in, 1, 4096, f);
  fclose(f);
  
  ck_assert(r == 680);

  // seek past the first 8 bytes (cmf header)
  char *i = in + 8;
  
  RCLDecoded *rcl = g_new0(RCLDecoded, 1);
  gboolean success = librcl_decode(i, 680 - 8, rcl);

  ck_assert(success);

  ck_assert(rcl->uuid == 1);
  ck_assert(rcl->response == TRUE);
  ck_assert(rcl->size == 656);
  
  g_free(rcl);
}

Suite *rcl_suite(void) {
  Suite *s = suite_create("RCL Suite");
  TCase *tc = NULL;

  tc = tcase_create("Decode");
  tcase_add_test(tc, test_rcl_decode_header);
  suite_add_tcase(s, tc);

  return s;
}
