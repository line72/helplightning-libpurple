#include "tests.h"

#include "../libballyhoo_message.h"
#include "../libballyhoo_deflate.h"

#include <stdio.h>

START_TEST(test_ballyhoo_message_decode) {
  char in[4096];
  FILE *f = fopen("message.bin", "rb");
  size_t r = fread(in, 1, 4096, f);
  fclose(f);
  
  ck_assert(r == 680);

  // seek past the first 8 + 16 bytes (cmf + rcl header)
  char *i = in + 8 + 16;
  int size = 680 - 8 - 16;
  
  BallyhooMessage *bm = libballyhoo_message_decode(i, size);
  ck_assert(bm != NULL);

  ck_assert(bm->length == 591);

  // verify some headers
  gpointer content_type = g_hash_table_lookup(bm->headers, "encoding");
  assert_string_equal_free("deflate", content_type);
  gpointer message_length = g_hash_table_lookup(bm->headers, "message-length");
  assert_string_equal_free("591", message_length);
  gpointer message_type = g_hash_table_lookup(bm->headers, "content-type");
  assert_string_equal_free("message", message_type);

  // verify the binary data in xmlrpc
  const char expected[591] = {
    0x78, 0x9c, 0xb5, 0x93, 0x4d, 0x73, 0x9b,
    0x30, 0x10, 0x86, 0xff, 0x4a, 0xc6, 0xf7, 0xda, 0x02, 0x7f, 0xd4, 0x9e, 0x71, 0x9c, 0xa1, 0x60,
    0x6c, 0xe1, 0x48, 0x0a, 0x8e, 0x8c, 0x80, 0x4b, 0x86, 0x00, 0x01, 0x89, 0x0f, 0xbb, 0x86, 0x04,
    0xc3, 0xaf, 0xaf, 0x70, 0xdd, 0x1c, 0xd2, 0x5b, 0x67, 0x7a, 0x92, 0xe6, 0x9d, 0x5d, 0xed, 0xbb,
    0xfb, 0x68, 0x97, 0x0f, 0x97, 0x22, 0xbf, 0xfb, 0x88, 0xcf, 0x15, 0x3f, 0x96, 0xf7, 0x03, 0x65,
    0x08, 0x06, 0x77, 0x71, 0x19, 0x1e, 0x23, 0x5e, 0x26, 0xf7, 0x83, 0x03, 0x35, 0xbf, 0xcd, 0x07,
    0x0f, 0xab, 0x65, 0x11, 0xd7, 0xe9, 0x31, 0xda, 0xc7, 0xd5, 0xe9, 0x58, 0x56, 0xf1, 0x6a, 0x79,
    0x0a, 0xce, 0x41, 0x51, 0xdd, 0xce, 0xd5, 0xf2, 0x23, 0xc8, 0xdf, 0xa5, 0x1a, 0x9c, 0xcf, 0x41,
    0xbb, 0x5a, 0x46, 0x41, 0x1d, 0x7c, 0x6a, 0x55, 0x7d, 0x96, 0x4f, 0xad, 0xe2, 0xd6, 0x4a, 0x5f,
    0x37, 0x21, 0x27, 0xdc, 0x82, 0x87, 0x0e, 0x2a, 0x98, 0xc3, 0x0a, 0x96, 0xfb, 0x69, 0xa8, 0xc3,
    0x19, 0xcc, 0x4e, 0xae, 0xa3, 0x5b, 0x8b, 0x61, 0x1f, 0x14, 0x31, 0xbb, 0x0f, 0xda, 0x06, 0x1b,
    0x73, 0xe6, 0xb1, 0x86, 0x3f, 0xea, 0x56, 0x1e, 0x6f, 0x35, 0x4e, 0xc4, 0x5a, 0xc5, 0x46, 0xd6,
    0x61, 0xc3, 0x53, 0xb0, 0x71, 0xa8, 0x60, 0x91, 0xa7, 0x91, 0x4c, 0x46, 0xd4, 0x03, 0x84, 0x22,
    0x80, 0x85, 0x3d, 0xc1, 0xcf, 0x0d, 0x0f, 0x5c, 0xdc, 0x41, 0x71, 0xe4, 0x7b, 0x35, 0x4d, 0xe3,
    0xc2, 0xac, 0x20, 0x97, 0x5a, 0xb9, 0x3f, 0xf5, 0x9a, 0x6f, 0x98, 0x05, 0xa1, 0xf6, 0xd8, 0xa3,
    0x5a, 0x8d, 0x0a, 0xa4, 0x12, 0x1d, 0x00, 0xc4, 0xec, 0xf6, 0x91, 0xad, 0xa7, 0x58, 0x45, 0xb5,
    0x2f, 0x6c, 0x05, 0x51, 0x47, 0x78, 0x9d, 0x99, 0xfb, 0x02, 0x82, 0x3e, 0xf7, 0x95, 0x39, 0xc0,
    0x7b, 0x86, 0xb3, 0x78, 0x0c, 0x64, 0xcd, 0x29, 0xf7, 0xf9, 0x97, 0x9a, 0x7a, 0xc3, 0xc3, 0x8d,
    0x53, 0x43, 0x71, 0xfa, 0x0e, 0x4b, 0xa7, 0xf3, 0x5d, 0xd8, 0x7b, 0x55, 0x88, 0x71, 0x68, 0xb1,
    0x08, 0x5b, 0x44, 0xe1, 0x45, 0xde, 0x15, 0xd4, 0x65, 0x13, 0x4c, 0x7f, 0x2c, 0x64, 0x3f, 0x5d,
    0xc4, 0x60, 0xdf, 0xa3, 0x13, 0xaa, 0x4e, 0x4b, 0x04, 0x9a, 0x22, 0x23, 0xbb, 0xd6, 0x8a, 0xb6,
    0x79, 0xd3, 0xfb, 0xf4, 0x18, 0x16, 0x7e, 0xdf, 0x47, 0xaf, 0x15, 0x4e, 0xdb, 0x6b, 0x88, 0x6a,
    0x0d, 0x2c, 0xc1, 0xd0, 0xfd, 0xf9, 0xb8, 0xcf, 0xcc, 0x84, 0x50, 0x3d, 0x0a, 0x2d, 0x4d, 0x39,
    0x51, 0xc0, 0x8c, 0xba, 0x71, 0xbb, 0x44, 0xe7, 0x0d, 0xb2, 0x77, 0x3b, 0xb2, 0x8b, 0xbd, 0xf9,
    0x65, 0x6c, 0xbf, 0xce, 0xe7, 0xcb, 0xd1, 0x6d, 0xf8, 0xcb, 0xd1, 0x0d, 0xc6, 0x7f, 0x60, 0x42,
    0x0c, 0x6d, 0x42, 0x0c, 0x08, 0xc8, 0x3f, 0x32, 0xc1, 0x0c, 0x4d, 0x09, 0xcd, 0x05, 0x62, 0x72,
    0xfe, 0x85, 0xdd, 0xf8, 0x1c, 0x00, 0x4f, 0xe4, 0x42, 0x32, 0x69, 0x08, 0x3d, 0xd4, 0x9e, 0x7a,
    0x50, 0x09, 0x5b, 0x4b, 0x66, 0x0e, 0x27, 0x06, 0xce, 0xae, 0x4c, 0x0a, 0xab, 0x90, 0xb9, 0x17,
    0xf9, 0xfe, 0x14, 0x75, 0xb6, 0xfc, 0x17, 0x09, 0x90, 0x9e, 0x1a, 0x9f, 0x01, 0x4e, 0xca, 0x8a,
    0x47, 0x2e, 0xce, 0xc3, 0x7c, 0xd1, 0xfa, 0xcc, 0x6f, 0xe5, 0x1c, 0x8f, 0xd7, 0x58, 0x9a, 0x28,
    0x48, 0x78, 0x63, 0x24, 0xd6, 0x92, 0x49, 0xa2, 0x60, 0x2a, 0xeb, 0x1a, 0x87, 0xe6, 0x4d, 0xfa,
    0x0b, 0xc7, 0x0e, 0xef, 0xbd, 0x38, 0xd7, 0x3c, 0x2b, 0xf7, 0x4b, 0x2b, 0x0f, 0xd5, 0x64, 0x26,
    0x99, 0x49, 0x0f, 0x76, 0xdf, 0x2f, 0x88, 0x5d, 0xad, 0x9f, 0x41, 0xea, 0xa9, 0x32, 0x66, 0x8c,
    0x7a, 0x4d, 0xbd, 0xb2, 0xe6, 0xf0, 0x82, 0x0c, 0x8d, 0xbf, 0xd9, 0xc3, 0x09, 0xfb, 0xf9, 0xe2,
    0xda, 0x4f, 0x8b, 0xad, 0x9d, 0xed, 0x26, 0xc8, 0x98, 0x9f, 0x26, 0x6d, 0xfe, 0x54, 0xa7, 0xed,
    0xc6, 0xf4, 0x5f, 0xcc, 0x8f, 0x9a, 0xa7, 0xa0, 0xdd, 0xbe, 0x67, 0x4c, 0x68, 0x86, 0xf7, 0x37,
    0x9a, 0xd1, 0xef, 0xad, 0x19, 0xdd, 0x76, 0xe8, 0x53, 0xbe, 0x6d, 0xd8, 0xe8, 0xcf, 0xc6, 0x8d,
    0xbe, 0xac, 0xe2, 0x2f, 0xba, 0xd5, 0x48, 0x53
  };

  for (int i = 0; i < 591; i++) {
    ck_assert_msg(expected[i] == ((char*)(bm->xmlrpc))[i], "Byte %d doesn't match %x vs %x", i, expected[i], ((char*)(bm->xmlrpc))[i]);
  }
  
  /* size_t output_leng */
  /* gpointer xmlrpc = libballyhoo_deflate_decompress(bm->xmlrpc, bm->length, &output_length); */

  /* ck_assert(xmlrpc != NULL); */

  /* assert_string_equal_free("<?xml version=\"1.0\" encoding=\"UTF-8\"?><methodResponse><params><param><value><array><data><value><string>eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2NDkzNDY1NDUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiZDFmOTQ3YTAtMmM2OC00MWQyLWE5N2MtZjQ1MTVjYzFlZjI0IiwibWV0YSI6e30sIm5iZiI6MTY0OTM0NjQ4NCwicGVtIjp7InVzZXIiOjE1ODUyNjcyMTIxODU1Mzk4NTB9LCJzdWIiOiJVc2VyOjM5MDk0IiwidHlwIjoiYWNjZXNzIiwidmVyIjoiMTAwIn0.XqLRkFgOTCdcJA1pT0WDtwXzgCiwMQKKOKeY8x3Qb88</string></value><value><string>eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJHaGF6YWwiLCJleHAiOjE2ODA4ODI0ODUsImlhdCI6MTY0OTM0NjQ4NSwiaXNzIjoiR2hhemFsIiwianRpIjoiNWM5OTljMWMtZmQwZi00YjljLWEwOTUtY2U2OWE3YTViODNkIiwibmJmIjoxNjQ5MzQ2NDg0LCJwZW0iOnsidXNlcl9yZWZyZXNoIjoxNTg1MjY3MjEyMTg1NTM5ODUwfSwic3ViIjoiVXNlclJlZnJlc2g6MzkwOTQiLCJ0eXAiOiJhY2Nlc3MiLCJ2ZXIiOiIxMDAifQ.4Wq_XQP9HQkK4MD8p4ylPthyGFZ_Fvtih0yHukWjADY</string></value></data></array></value></param></params></methodResponse>", xmlrpc); */
  
  /* ck_assert(output_length == 961); */
  
  g_free(bm);
}

Suite *ballyhoo_message_suite(void) {
  Suite *s = suite_create("BALLYHOO_messageSuite");
  TCase *tc = NULL;

  tc = tcase_create("Decode");
  tcase_add_test(tc, test_ballyhoo_message_decode);
  suite_add_tcase(s, tc);

  return s;
}