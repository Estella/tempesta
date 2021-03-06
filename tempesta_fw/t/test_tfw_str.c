/**
 *		Tempesta FW
 *
 * Copyright (C) 2012-2014 NatSys Lab. (info@natsys-lab.com).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <linux/kernel.h>

#include "str.h"
#include "test.h"

static TfwPool *str_pool;

static void
create_str_pool(void)
{
	BUG_ON(str_pool);
	str_pool = __tfw_pool_new(1);
	BUG_ON(!str_pool);
}

static void
free_all_str(void)
{
	tfw_pool_free(str_pool);
	str_pool = NULL;
}

static TfwStr *
alloc_str(void)
{
	TfwStr *s;

	s = tfw_pool_alloc(str_pool, sizeof(*s));
	BUG_ON(!s);
	memset(s, 0, sizeof(*s));

	return s;
}

static TfwStr *
make_plain_str(const char *data)
{
	TfwStr *s = alloc_str();

	s->len =  strlen(data);
	s->ptr = (void *)data;

	return s;
}

static TfwStr *
make_compound_str(const char *data)
{
	TfwStr *str, *chunk;
	size_t chunk_len = 0;
	size_t total_len = strlen(data);

	str = alloc_str();

	do {
		chunk = tfw_str_add_compound(str_pool, str);
		chunk->len = min(total_len, ++chunk_len % 4);
		chunk->ptr = (void *)data;
		data += chunk->len;
		total_len -= chunk->len;
	} while (total_len > 0);

	return str;
}

#define TFW_STR(name, literal) const TfwStr *name = make_compound_str(literal)


TEST(tfw_str_eq_cstr, returns_true_only_for_equal_strings)
{
	const char *cstr = "foo123 barbaz";
	int len = strlen(cstr);

	TFW_STR(match, "foo123 barbaz");
	TFW_STR(diff1, "aoo123 barbaz");
	TFW_STR(diff2, "foo123 barbaa");
	TFW_STR(diff3, "Foo123 barbaz");
	TFW_STR(crop,  "foo123 barba");
	TFW_STR(extra, "foo123 barbazz");

	EXPECT_TRUE(tfw_str_eq_cstr(match, cstr, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(diff1, cstr, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(diff2, cstr, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(diff3, cstr, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(crop,  cstr, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(extra, cstr, len, TFW_STR_EQ_DEFAULT));
}

TEST(tfw_str_eq_cstr, handles_plain_str)
{
	const char *cstr1 = "foo";
	const char *cstr2 = "bar baz";
	size_t len1 = strlen(cstr1);
	size_t len2 = strlen(cstr2);
	TfwStr *s1 = make_plain_str(cstr1);
	TfwStr *s2 = make_plain_str(cstr2);

	EXPECT_TRUE(tfw_str_eq_cstr(s1, cstr1, len1, TFW_STR_EQ_DEFAULT));
	EXPECT_TRUE(tfw_str_eq_cstr(s2, cstr2, len2, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(s1, cstr2, len2, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(s2, cstr1, len1, TFW_STR_EQ_DEFAULT));
}

TEST(tfw_str_eq_cstr, handles_unterminated_strs)
{
	const char *cstr = "foobarbaz [SOME GARBAGE]";
	int cstr_len = 9;
	TfwStr s = {
		.len = cstr_len,
		.ptr = (void *)"foobarbaz [ANOTHER GARBAGE]"
	};
	EXPECT_TRUE(tfw_str_eq_cstr(&s, cstr, cstr_len, TFW_STR_EQ_DEFAULT));
}

TEST(tfw_str_eq_cstr, handles_empty_strs)
{
	TfwStr s1 = {
		.len = 0,
		.ptr = (void *)"garbage"
	};
	TfwStr s2 = {
		.len = 0,
		.ptr = NULL
	};
	TfwStr chunks[] = { s1, s2 };
	TfwStr s3 = {
		.flags = TFW_STR_COMPOUND,
		.len = 2,
		.ptr = &chunks
	};
	TfwStr s_ne = {
		.len = 3,
		.ptr = (void *)"foo"
	};
	const char *cstr = "";
	const char *cstr_ne = "bar";
	size_t len = strlen(cstr_ne);

	EXPECT_TRUE(tfw_str_eq_cstr(&s1, cstr, 0, TFW_STR_EQ_DEFAULT));
	EXPECT_TRUE(tfw_str_eq_cstr(&s2, cstr, 0, TFW_STR_EQ_DEFAULT));
	EXPECT_TRUE(tfw_str_eq_cstr(&s3, cstr, 0, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(&s_ne, cstr, 0, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(&s1, cstr_ne, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(&s2, cstr_ne, len, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(&s3, cstr_ne, len, TFW_STR_EQ_DEFAULT));
}


TEST(tfw_str_eq_cstr, supports_casei)
{
	TFW_STR(s, "FooBarBaz 123");
	const char *cstr1 = "FooBarBaz 123";
	const char *cstr2 = "fooBarBaz 123";
	const char *cstr3 = "FooBarBaZ 123";
	size_t len1 = strlen(cstr1);
	size_t len2 = strlen(cstr2);
	size_t len3 = strlen(cstr3);

	EXPECT_TRUE(tfw_str_eq_cstr(s, cstr1, len1, TFW_STR_EQ_CASEI));
	EXPECT_TRUE(tfw_str_eq_cstr(s, cstr2, len2, TFW_STR_EQ_CASEI));
	EXPECT_TRUE(tfw_str_eq_cstr(s, cstr3, len3, TFW_STR_EQ_CASEI));
	EXPECT_FALSE(tfw_str_eq_cstr(s, cstr2, len2, TFW_STR_EQ_DEFAULT));
	EXPECT_FALSE(tfw_str_eq_cstr(s, cstr3, len3, TFW_STR_EQ_DEFAULT));
}

TEST(tfw_str_eq_cstr, supports_prefix)
{
	TFW_STR(s, "/foo/bar/baz.test");
	const char *p1 = "/foo/bar/baz.test";
	const char *p2 = "/foo/bar/baz.tes";
	const char *p3 = "/foo/bar/baz";
	const char *p4 = "/foo/bar/";
	const char *p5 = "/foo";
	const char *p6 = "/";
	const char *p7 = "";
	const char *extra = "/foo/bar/baz.test1";
	const char *p1_ci = "/foo/bar/baz.tesT";
	const char *p5_ci = "/Foo";

	EXPECT_TRUE(tfw_str_eq_cstr(s, p1, strlen(p1), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p2, strlen(p2), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p3, strlen(p3), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p4, strlen(p4), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p5, strlen(p5), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p6, strlen(p6), TFW_STR_EQ_PREFIX));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p7, strlen(p7), TFW_STR_EQ_PREFIX));

	EXPECT_FALSE(tfw_str_eq_cstr(s, extra, strlen(extra), TFW_STR_EQ_PREFIX));
	EXPECT_FALSE(tfw_str_eq_cstr(s, p1_ci, strlen(p1_ci), TFW_STR_EQ_PREFIX));
	EXPECT_FALSE(tfw_str_eq_cstr(s, p5_ci, strlen(p5_ci), TFW_STR_EQ_PREFIX));

	EXPECT_TRUE(tfw_str_eq_cstr(s, p1_ci, strlen(p1_ci), TFW_STR_EQ_PREFIX_CASEI));
	EXPECT_TRUE(tfw_str_eq_cstr(s, p5_ci, strlen(p5_ci), TFW_STR_EQ_PREFIX_CASEI));
}

TEST(tfw_str_eq_kv, reutrns_true_only_for_equal_kv)
{
	const char *key = "Cache-Control";
	const char *val = "max-age=3600, public";
	size_t klen = strlen(key);
	size_t vlen = strlen(val);
	char sep = ':';
	tfw_str_eq_flags_t flags = TFW_STR_EQ_DEFAULT;

	TFW_STR(match,    "Cache-Control: max-age=3600, public");
	TFW_STR(keydiff,  "Cache-Cootrol: max-age=3600, public");
	TFW_STR(keylong,  "Cache-Controll: max-age=3600, public");
	TFW_STR(keyshort, "Cache-Contro: max-age=3600, public");
	TFW_STR(sepdiff,  "Cache-Control= max-age=3600, public");
	TFW_STR(sepdup,   "Cache-Control:: max-age=3600, public");
	TFW_STR(sepempty, "Cache-Control max-age=3600, public");
	TFW_STR(valshort, "Cache-Control: max-age=3600, publi");
	TFW_STR(valdiff,  "Cache-Control: max-age=3601, public");

	EXPECT_TRUE(tfw_str_eq_kv(match, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(keydiff, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(keylong, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(keyshort, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(sepdiff, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(sepdup, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(sepempty, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(valshort, key, klen, sep, val, vlen, flags));
	EXPECT_FALSE(tfw_str_eq_kv(valdiff, key, klen, sep, val, vlen, flags));
}

TEST(tfw_str_eq_kv, handles_plain_str)
{
	TfwStr *s1 = make_plain_str("Cache-Control: max-age=3600, public");
	TfwStr *s2 = make_plain_str("Cache-Control: no-cache");
	const char *k = "Cache-Control";
	const char *v1 = "max-age=3600, public";
	const char *v2 = "no-cache";
	size_t klen = strlen(k);
	size_t v1_len = strlen(v1);
	size_t v2_len = strlen(v2);
	tfw_str_eq_flags_t flags = TFW_STR_EQ_DEFAULT;
	char sep = ':';

	EXPECT_TRUE(tfw_str_eq_kv(s1, k, klen, sep, v1, v1_len, flags));
	EXPECT_TRUE(tfw_str_eq_kv(s2, k, klen, sep, v2, v2_len, flags));
	EXPECT_FALSE(tfw_str_eq_kv(s1, k, klen, sep, v2, v2_len, flags));
	EXPECT_FALSE(tfw_str_eq_kv(s2, k, klen, sep, v1, v1_len, flags));
}

TEST(tfw_str_eq_kv, handles_unterminated_strs)
{
	TfwStr s = {
		.len = 35,
		.ptr = (void *)"Cache-Control: max-age=3600, public [GARBAGE1]"
	};
	const char *k = "Cache-Control [GARBAGE2]";
	const char *v = "max-age=3600, public [GARBAGE3]";
	size_t klen = 13;
	size_t vlen = 20;
	tfw_str_eq_flags_t flags = TFW_STR_EQ_DEFAULT;
	char sep = ':';

	EXPECT_TRUE(tfw_str_eq_kv(&s, k, klen, sep, v, vlen, flags));
}

TEST(tfw_str_eq_kv, handles_empty_val)
{
	TFW_STR(str_e1, "Cache-Control:");
	TFW_STR(str_e2, "Cache-Control:    ");
	TFW_STR(str_ne1, "Cache-Control: max-age=3600, public");
	TFW_STR(str_ne2, "Cache-Control=");
	TFW_STR(str_ne3, "Cache-Control= ");
	TFW_STR(str_ne4, "Cache-Control=     ");
	TFW_STR(str_ne5, "Cache-Contro:");
	TFW_STR(str_ne6, "Cache-Contr:");
	TFW_STR(str_ne7, "Cache-Cont:");
	TFW_STR(str_ne8, "");
	TFW_STR(str_ne9, "Other-Key:");
	TFW_STR(str_ne10, "Other-Key: ");
	TFW_STR(str_ne11, "Other-Key:    ");
	const char *k = "Cache-Control";
	size_t klen = strlen(k);
	tfw_str_eq_flags_t flags = TFW_STR_EQ_DEFAULT;

	EXPECT_TRUE(tfw_str_eq_kv(str_e1, k, klen, ':', "", 0, flags));
	EXPECT_TRUE(tfw_str_eq_kv(str_e2, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne1, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne2, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne3, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne4, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne5, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne6, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne7, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne8, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne9, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne10, k, klen, ':', "", 0, flags));
	EXPECT_FALSE(tfw_str_eq_kv(str_ne11, k, klen, ':', "", 0, flags));
}

TEST(tfw_str_eq_kv, supports_casei_val)
{
	TFW_STR(keycase, "cache-control: max-age=3600, public");
	TFW_STR(valcase1, "Cache-Control: Max-age=3600, public");
	TFW_STR(valcase2, "Cache-Control: Max-age=3600, publiC");
	const char *k = "Cache-Control";
	const char *v = "max-age=3600, public";
	size_t klen = strlen(k);
	size_t vlen = strlen(v);
	char sep = ':';
	tfw_str_eq_flags_t flags_ci = TFW_STR_EQ_CASEI;
	tfw_str_eq_flags_t flags_cs = TFW_STR_EQ_DEFAULT;

	/* The key is always case-sensitive, regardless of the flag. */
	EXPECT_TRUE(tfw_str_eq_kv(keycase, k, klen, sep, v, vlen, flags_ci));
	EXPECT_TRUE(tfw_str_eq_kv(keycase, k, klen, sep, v, vlen, flags_cs));

	EXPECT_TRUE(tfw_str_eq_kv(valcase1, k, klen, sep, v, vlen, flags_ci));
	EXPECT_TRUE(tfw_str_eq_kv(valcase2, k, klen, sep, v, vlen, flags_ci));
	EXPECT_FALSE(tfw_str_eq_kv(valcase1, k, klen, sep, v, vlen, flags_cs));
	EXPECT_FALSE(tfw_str_eq_kv(valcase2, k, klen, sep, v, vlen, flags_cs));
}

TEST(tfw_str_eq_kv, supports_prefix_val)
{
	TFW_STR(s, "Cache-Control: max-age=3600, public");
	const char *k = "Cache-Control";
	const char *p1 = "max-age=3600, public";
	const char *p2 = "max-age=3600";
	const char *p3 = "m";
	const char *p4 = "";
	const char *extra = "max-age=3600, publicc";
	const char *diff = "max-age=3601, public";
	const char *p2_ci = "Max-Age=3600";
	size_t k_len = strlen(k);
	size_t p1_len = strlen(p1);
	size_t p2_len = strlen(p2);
	size_t p3_len = strlen(p3);
	size_t p4_len = strlen(p4);
	size_t extra_len = strlen(extra);
	size_t diff_len = strlen(diff);
	size_t p2_ci_len = strlen(p2_ci);
	char sep = ':';
	tfw_str_eq_flags_t flags = TFW_STR_EQ_PREFIX;
	tfw_str_eq_flags_t flags_ci = TFW_STR_EQ_PREFIX_CASEI;

	EXPECT_TRUE(tfw_str_eq_kv(s, k, k_len, sep, p1, p1_len, flags));
	EXPECT_TRUE(tfw_str_eq_kv(s, k, k_len, sep, p2, p2_len, flags));
	EXPECT_TRUE(tfw_str_eq_kv(s, k, k_len, sep, p3, p3_len, flags));
	EXPECT_TRUE(tfw_str_eq_kv(s, k, k_len, sep, p4, p4_len, flags));
	EXPECT_FALSE(tfw_str_eq_kv(s, k, k_len, sep, extra, extra_len, flags));
	EXPECT_FALSE(tfw_str_eq_kv(s, k, k_len, sep, diff, diff_len, flags));
	EXPECT_FALSE(tfw_str_eq_kv(s, k, k_len, sep, p2_ci, p2_ci_len, flags));
	EXPECT_TRUE(tfw_str_eq_kv(s, k, k_len, sep, p2_ci, p2_ci_len, flags_ci));

}

TEST(tfw_str_eq_kv, allows_space_sep)
{
	TFW_STR(s, "key v a l u e");
	const char *k = "key";
	const char *v = "v a l u e";
	size_t klen = strlen(k);
	size_t vlen = strlen(v);
	char sep = ' ';

	EXPECT_TRUE(tfw_str_eq_kv(s, k, klen, sep, v, vlen, TFW_STR_EQ_DEFAULT));
}


TEST_SUITE(tfw_str)
{
	TEST_SETUP(create_str_pool);
	TEST_TEARDOWN(free_all_str);

	TEST_RUN(tfw_str_eq_cstr, returns_true_only_for_equal_strings);
	TEST_RUN(tfw_str_eq_cstr, handles_plain_str);
	TEST_RUN(tfw_str_eq_cstr, handles_unterminated_strs);
	TEST_RUN(tfw_str_eq_cstr, handles_empty_strs);
	TEST_RUN(tfw_str_eq_cstr, supports_casei);
	TEST_RUN(tfw_str_eq_cstr, supports_prefix);

	TEST_RUN(tfw_str_eq_kv, reutrns_true_only_for_equal_kv);
	TEST_RUN(tfw_str_eq_kv, handles_plain_str);
	TEST_RUN(tfw_str_eq_kv, handles_unterminated_strs);
	TEST_RUN(tfw_str_eq_kv, handles_empty_val);
	TEST_RUN(tfw_str_eq_kv, supports_casei_val);
	TEST_RUN(tfw_str_eq_kv, supports_prefix_val);
	TEST_RUN(tfw_str_eq_kv, allows_space_sep);
}



