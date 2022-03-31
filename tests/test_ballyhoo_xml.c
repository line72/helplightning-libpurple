#include "tests.h"

#include "../libballyhoo_deflate.h"
#include "../libballyhoo_xml.h"

#include <stdio.h>

START_TEST(test_ballyhoo_xml_decode) {
  char in[4096];
  FILE *f = fopen("message.bin", "rb");
  size_t r = fread(in, 1, 4096, f);
  fclose(f);
  
  ck_assert(r == 680);

  // seek past the first 8 + 16 bytes (cmf + rcl header)
  // + the ballyhoo message headers (65)
  char *i = in + 8 + 16 + 65;
  int size = 680 - 8 - 16 - 65; // 591 bytes left

  // first decompress it
  size_t output_length;
  gpointer xmlrpc = libballyhoo_deflate_decompress(i, size, &output_length);
  ck_assert(xmlrpc != NULL);

  // decode the xmlrpc
  BallyhooXMLRPC *b = libballyhoo_xml_decode(xmlrpc, output_length);
  g_free(xmlrpc);

  xmlrpc_env env;
  xmlrpc_env_init(&env);
  
  ck_assert(b->type == BXMLRPC_RESPONSE);
  ck_assert(xmlrpc_value_type(b->response) == XMLRPC_TYPE_ARRAY);
  ck_assert(xmlrpc_array_size(&env, b->response) == 2);

  xmlrpc_value *arr;
  const char *v;
  xmlrpc_array_read_item(&env, b->response, 0, &arr);
  ck_assert(env.fault_occurred == FALSE);

  ck_assert(xmlrpc_value_type(arr) == XMLRPC_TYPE_STRING);
  xmlrpc_read_string(&env, arr, &v);
  assert_string_equal("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2NDkzNDY1NDUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiZDFmOTQ3YTAtMmM2OC00MWQyLWE5N2MtZjQ1MTVjYzFlZjI0IiwibWV0YSI6e30sIm5iZiI6MTY0OTM0NjQ4NCwicGVtIjp7InVzZXIiOjE1ODUyNjcyMTIxODU1Mzk4NTB9LCJzdWIiOiJVc2VyOjM5MDk0IiwidHlwIjoiYWNjZXNzIiwidmVyIjoiMTAwIn0.XqLRkFgOTCdcJA1pT0WDtwXzgCiwMQKKOKeY8x3Qb88", v);
  xmlrpc_DECREF(arr);

  xmlrpc_array_read_item(&env, b->response, 1, &arr);
  ck_assert(env.fault_occurred == FALSE);

  ck_assert(xmlrpc_value_type(arr) == XMLRPC_TYPE_STRING);
  xmlrpc_read_string(&env, arr, &v);
  assert_string_equal("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2ODA4ODI0ODUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiNWM5OTljMWMtZmQwZi00YjljLWEwOTUtY2U2OWE3YTViODNkIiwibmJmIjoxNjQ5MzQ2NDg0LCJwZW0iOnsidXNlcl9yZWZyZXNoIjoxNTg1MjY3MjEyMTg1NTM5ODUwfSwic3ViIjoiVXNlclJlZnJlc2g6MzkwOTQiLCJ0eXAiOiJhY2Nlc3MiLCJ2ZXIiOiIxMDAifQ.4Wq_XQP9HQkK4MD8p4ylPthyGFZ_Fvtih0yHukWjADY", v);
  xmlrpc_DECREF(arr);
}

Suite *ballyhoo_xml_suite(void) {
  Suite *s = suite_create("BALLYHOO_xml Suite");
  TCase *tc = NULL;

  tc = tcase_create("Decode");
  tcase_add_test(tc, test_ballyhoo_xml_decode);
  suite_add_tcase(s, tc);

  return s;
}
