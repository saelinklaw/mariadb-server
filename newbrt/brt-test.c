/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007, 2008 Tokutek Inc.  All rights reserved."

#include "brt.h"
#include "key.h"
#include "pma.h"
#include "brt-internal.h"

#include "memory.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "test.h"

static TOKUTXN const null_txn = 0;
static DB * const null_db = 0;

static void test0 (void) {
    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    if (verbose) printf("%s:%d test0\n", __FILE__, __LINE__);
    toku_memory_check=1;
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    if (verbose) printf("%s:%d test0\n", __FILE__, __LINE__);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 1024, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);
    //printf("%s:%d test0\n", __FILE__, __LINE__);
    //printf("%s:%d n_items_malloced=%lld\n", __FILE__, __LINE__, n_items_malloced);
    r = toku_close_brt(t);     assert(r==0);
    //printf("%s:%d n_items_malloced=%lld\n", __FILE__, __LINE__, n_items_malloced);
    r = toku_cachetable_close(&ct);
    assert(r==0);
    toku_memory_check_all_free();
}

static void test1 (void) {
    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    DBT k,v;
    toku_memory_check=1;
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 1024, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);
    toku_brt_insert(t, toku_fill_dbt(&k, "hello", 6), toku_fill_dbt(&v, "there", 6), null_txn);
    {
	r = toku_brt_lookup(t, toku_fill_dbt(&k, "hello", 6), toku_init_dbt(&v));
	assert(r==0);
	assert(strcmp(v.data, "there")==0);
	assert(v.size==6);
    }
    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
    toku_memory_check_all_free();
    if (verbose) printf("test1 ok\n");
}

static void test2 (int memcheck) {
    BRT t;
    int r;
    int i;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    toku_memory_check=memcheck;
    if (verbose) printf("%s:%d checking\n", __FILE__, __LINE__);
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 1024, ct, null_txn, toku_default_compare_fun, null_db);
    if (verbose) printf("%s:%d did setup\n", __FILE__, __LINE__);
    assert(r==0);
    for (i=0; i<2048; i++) {
	DBT k,v;
	char key[100],val[100];
	snprintf(key,100,"hello%d",i);
	snprintf(val,100,"there%d",i);
	toku_brt_insert(t, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
	//printf("%s:%d did insert %d\n", __FILE__, __LINE__, i);
	if (0) {
	    brt_flush(t);
	    {
		int n = toku_get_n_items_malloced();
		if (verbose) printf("%s:%d i=%d n_items_malloced=%d\n", __FILE__, __LINE__, i, n);
		if (n!=3) toku_print_malloced_items();
		assert(n==3);
	    }
	}
    }
    if (verbose) printf("%s:%d inserted\n", __FILE__, __LINE__);
    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
    toku_memory_check_all_free();
    if (verbose) printf("test2 ok\n");
}

static void test3 (int nodesize, int count, int memcheck) {
    BRT t;
    int r;
    struct timeval t0,t1;
    int i;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    toku_memory_check=memcheck;
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    gettimeofday(&t0, 0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, nodesize, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);
    for (i=0; i<count; i++) {
	char key[100],val[100];
	DBT k,v;
	snprintf(key,100,"hello%d",i);
	snprintf(val,100,"there%d",i);
	toku_brt_insert(t, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
    }
    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
    toku_memory_check_all_free();
    gettimeofday(&t1, 0);
    {
	double tdiff = (t1.tv_sec-t0.tv_sec)+1e-6*(t1.tv_usec-t0.tv_usec);
	if (verbose) printf("serial insertions: blocksize=%d %d insertions in %.3f seconds, %.2f insertions/second\n", nodesize, count, tdiff, count/tdiff);
    }
}

static void test4 (int nodesize, int count, int memcheck) {
    BRT t;
    int r;
    struct timeval t0,t1;
    int i;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    gettimeofday(&t0, 0);
    unlink(fname);
    toku_memory_check=memcheck;
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);         assert(r==0);
    r = toku_open_brt(fname, 0, 1, &t, nodesize, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);
    for (i=0; i<count; i++) {
	char key[100],val[100];
	int rv = random();
	DBT k,v;
	snprintf(key,100,"hello%d",rv);
	snprintf(val,100,"there%d",i);
	toku_brt_insert(t, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
    }
    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
    toku_memory_check_all_free();
    gettimeofday(&t1, 0);
    {
	double tdiff = (t1.tv_sec-t0.tv_sec)+1e-6*(t1.tv_usec-t0.tv_usec);
	if (verbose) printf("random insertions: blocksize=%d %d insertions in %.3f seconds, %.2f insertions/second\n", nodesize, count, tdiff, count/tdiff);
    }
}

static void test5 (void) {
    int r;
    BRT t;
    int limit=100000;
    int *values;
    int i;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    toku_memory_check_all_free();
    MALLOC_N(limit,values);
    for (i=0; i<limit; i++) values[i]=-1;
    unlink(fname);
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);        assert(r==0);
    r = toku_open_brt(fname, 0, 1, &t, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);   assert(r==0);
    for (i=0; i<limit/2; i++) {
	char key[100],val[100];
	int rk = random()%limit;
	int rv = random();
	if (i%1000==0 && verbose) { printf("w"); fflush(stdout); }
	values[rk] = rv;
	snprintf(key, 100, "key%d", rk);
	snprintf(val, 100, "val%d", rv);
	DBT k,v;
	toku_brt_insert(t, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
    }
    if (verbose) printf("\n");
    for (i=0; i<limit/2; i++) {
	int rk = random()%limit;
	if (values[rk]>=0) {
	    char key[100], valexpected[100];
	    DBT k,v;
	    if (i%1000==0 && verbose) { printf("r"); fflush(stdout); }
	    snprintf(key, 100, "key%d", rk);
	    snprintf(valexpected, 100, "val%d", values[rk]);
	    r = toku_brt_lookup(t, toku_fill_dbt(&k, key, 1+strlen(key)), toku_init_dbt(&v));
	    assert(r==0);
	    assert(v.size==(1+strlen(valexpected)));
	    assert(memcmp(v.data,valexpected,v.size)==0);
	}
    }
    if (verbose) printf("\n");
    toku_free(values);
    r = toku_close_brt(t);          assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
}

static void test_dump_empty_db (void) {
    BRT t;
    CACHETABLE ct;
    int r;
    char fname[]="testbrt.brt";
    toku_memory_check=1;
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 1024, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);
    if (verbose) toku_dump_brt(t);
    r = toku_close_brt(t);          assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
}

/* Test running multiple trees in different files */
static void test_multiple_files_of_size (int size) {
    const char *n0 = "test0.brt";
    const char *n1 = "test1.brt";
    CACHETABLE ct;
    BRT t0,t1;
    int r,i;
    if (verbose) printf("test_multiple_files_of_size(%d)\n", size);
    unlink(n0);
    unlink(n1);
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);   assert(r==0);
    r = toku_open_brt(n0, 0, 1, &t0, size, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);
    r = toku_open_brt(n1, 0, 1, &t1, size, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);
    for (i=0; i<10000; i++) {
	char key[100],val[100];
	DBT k,v;
	snprintf(key, 100, "key%d", i);
	snprintf(val, 100, "val%d", i);
	toku_brt_insert(t0, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
	snprintf(val, 100, "Val%d", i);
	toku_brt_insert(t1, toku_fill_dbt(&k, key, 1+strlen(key)), toku_fill_dbt(&v, val, 1+strlen(val)), null_txn);
    }
    //toku_verify_brt(t0);
    //dump_brt(t0);
    //dump_brt(t1);
    toku_verify_brt(t0);
    toku_verify_brt(t1);

    r = toku_close_brt(t0); assert(r==0);
    r = toku_close_brt(t1); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();

    /* Now see if the data is all there. */
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);      assert(r==0);
    r = toku_open_brt(n0, 0, 0, &t0, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);
    if (verbose) printf("%s:%d r=%d\n", __FILE__, __LINE__,r);
    assert(r==0);
    r = toku_open_brt(n1, 0, 0, &t1, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    for (i=0; i<10000; i++) {
	char key[100],val[100];
	DBT k,actual;
	snprintf(key, 100, "key%d", i);
	snprintf(val, 100, "val%d", i);
	r=toku_brt_lookup(t0, toku_fill_dbt(&k, key, 1+strlen(key)), toku_init_dbt(&actual));
	assert(r==0);
	assert(strcmp(val,actual.data)==0);
	assert(actual.size==1+strlen(val));
	snprintf(val, 100, "Val%d", i);
	r=toku_brt_lookup(t1, toku_fill_dbt(&k, key, 1+strlen(key)), toku_init_dbt(&actual));
	assert(r==0);
	assert(strcmp(val,actual.data)==0);
	assert(actual.size==1+strlen(val));
    }

    r = toku_close_brt(t0); assert(r==0);
    r = toku_close_brt(t1); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
}

static void test_multiple_files (void) {
    test_multiple_files_of_size (1<<12);
    test_multiple_files_of_size (1<<20);
}

static void test_named_db (void) {
    const char *n0 = "test0.brt";
    const char *n1 = "test1.brt";
    CACHETABLE ct;
    BRT t0;
    int r;
    DBT k,v;

    if (verbose) printf("test_named_db\n");
    unlink(n0);
    unlink(n1);
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);           assert(r==0);
    r = toku_open_brt(n0, "db1", 1, &t0, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);


    toku_brt_insert(t0, toku_fill_dbt(&k, "good", 5), toku_fill_dbt(&v, "day", 4), null_txn); assert(r==0);

    r = toku_close_brt(t0); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();

    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);           assert(r==0);
    r = toku_open_brt(n0, "db1", 0, &t0, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    {
	r = toku_brt_lookup(t0, toku_fill_dbt(&k, "good", 5), toku_init_dbt(&v));
	assert(r==0);
	assert(v.size==4);
	assert(strcmp(v.data,"day")==0);
    }

    r = toku_close_brt(t0); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
}

static void test_multiple_dbs (void) {
    const char *n0 = "test0.brt";
    const char *n1 = "test1.brt";
    CACHETABLE ct;
    BRT t0,t1;
    int r;
    DBT k,v;
    if (verbose) printf("test_multiple_dbs: ");
    unlink(n0);
    unlink(n1);
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);        assert(r==0);
    r = toku_open_brt(n0, "db1", 1, &t0, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);
    r = toku_open_brt(n1, "db2", 1, &t1, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    toku_brt_insert(t0, toku_fill_dbt(&k, "good", 5), toku_fill_dbt(&v, "grief", 6), null_txn); assert(r==0);
    toku_brt_insert(t1, toku_fill_dbt(&k, "bad",  4), toku_fill_dbt(&v, "night", 6), null_txn); assert(r==0);

    r = toku_close_brt(t0); assert(r==0);
    r = toku_close_brt(t1); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);        assert(r==0);
    r = toku_open_brt(n0, "db1", 0, &t0, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);
    r = toku_open_brt(n1, "db2", 0, &t1, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    {
	r = toku_brt_lookup(t0, toku_fill_dbt(&k, "good", 5), toku_init_dbt(&v));
	assert(r==0);
	assert(v.size==6);
	assert(strcmp(v.data,"grief")==0);

	r = toku_brt_lookup(t1, toku_fill_dbt(&k, "good", 5), toku_init_dbt(&v));
	assert(r!=0);

	r = toku_brt_lookup(t0, toku_fill_dbt(&k, "bad", 4), toku_init_dbt(&v));
	assert(r!=0);

	r = toku_brt_lookup(t1, toku_fill_dbt(&k, "bad", 4), toku_init_dbt(&v));
	assert(r==0);
	assert(v.size==6);
	assert(strcmp(v.data,"night")==0);
    }

    r = toku_close_brt(t0); assert(r==0);
    r = toku_close_brt(t1); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();
    if (verbose) printf("ok\n");
}

/* Test to see a single file can contain many databases. */
static void test_multiple_dbs_many (void) {
    enum { MANYN = 16 };
    int i, r;
    const char *name = "test.brt";
    CACHETABLE ct;
    BRT trees[MANYN];
    if (verbose) printf("test_multiple_dbs_many:\n");
    toku_memory_check_all_free();
    unlink(name);
    r = toku_brt_create_cachetable(&ct, MANYN+4, ZERO_LSN, NULL_LOGGER);     assert(r==0);
    for (i=0; i<MANYN; i++) {
	char dbname[20];
	snprintf(dbname, 20, "db%d", i);
	r = toku_open_brt(name, dbname, 1, &trees[i], 1<<12, ct, null_txn, toku_default_compare_fun, null_db);
	assert(r==0);
    }
    for (i=0; i<MANYN; i++) {
	char k[20], v[20];
	DBT kdbt,vdbt;
	snprintf(k, 20, "key%d", i);
	snprintf(v, 20, "val%d", i);
	toku_brt_insert(trees[i], toku_fill_dbt(&kdbt, k, strlen(k)+1), toku_fill_dbt(&vdbt, v, strlen(v)+1), null_txn);
    }
    for (i=0; i<MANYN; i++) {
	r = toku_close_brt(trees[i]); assert(r==0);
    }
    r = toku_cachetable_close(&ct);    assert(r==0);
    toku_memory_check_all_free();
}

/* Test to see that a single db can be opened many times.  */
static void test_multiple_brts_one_db_one_file (void) {
    enum { MANYN = 2 };
    int i, r;
    const char *name = "test.brt";
    CACHETABLE ct;
    BRT trees[MANYN];
    if (verbose) printf("test_multiple_brts_one_db_one_file:");
    toku_memory_check_all_free();
    unlink(name);
    r = toku_brt_create_cachetable(&ct, 32, ZERO_LSN, NULL_LOGGER); assert(r==0);
    for (i=0; i<MANYN; i++) {
	r = toku_open_brt(name, 0, (i==0), &trees[i], 1<<12, ct, null_txn, toku_default_compare_fun, null_db);
	assert(r==0);
    }
    for (i=0; i<MANYN; i++) {
	char k[20], v[20];
	DBT kb, vb;
	snprintf(k, 20, "key%d", i);
	snprintf(v, 20, "val%d", i);
	toku_brt_insert(trees[i], toku_fill_dbt(&kb, k, strlen(k)+1), toku_fill_dbt(&vb, v, strlen(v)+1), null_txn);
    }
    for (i=0; i<MANYN; i++) {
	char k[20],vexpect[20];
	DBT kb, vb;
	snprintf(k, 20, "key%d", i);
	snprintf(vexpect, 20, "val%d", i);
	r=toku_brt_lookup(trees[0], toku_fill_dbt(&kb, k, strlen(k)+1), toku_init_dbt(&vb));
	assert(r==0);
	assert(vb.size==1+strlen(vexpect));
	assert(strcmp(vb.data, vexpect)==0);
    }
    for (i=0; i<MANYN; i++) {
	r=toku_close_brt(trees[i]); assert(r==0);
    }
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
    if (verbose) printf(" ok\n");
}


/* Check to see if data can be read that was written. */
static void  test_read_what_was_written (void) {
    const char *n="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    const int NVALS=10000;

    if (verbose) printf("test_read_what_was_written(): "); fflush(stdout);

    unlink(n);
    toku_memory_check_all_free();

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);       assert(r==0);
    r = toku_open_brt(n, 0, 1, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);  assert(r==0);
    r = toku_close_brt(brt); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();

    /* Now see if we can read an empty tree in. */
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);     assert(r==0);
    r = toku_open_brt(n, 0, 0, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);  assert(r==0);

    /* See if we can put something in it. */
    {
	DBT k,v;
	toku_brt_insert(brt, toku_fill_dbt(&k, "hello", 6), toku_fill_dbt(&v, "there", 6), null_txn);
    }

    r = toku_close_brt(brt); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();

    /* Now see if we can read it in and get the value. */
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);    assert(r==0);
    r = toku_open_brt(n, 0, 0, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    {
	DBT k,v;
	r = toku_brt_lookup(brt, toku_fill_dbt(&k, "hello", 6), toku_init_dbt(&v));
	assert(r==0);
	assert(v.size==6);
	assert(strcmp(v.data,"there")==0);
    }

    assert(toku_verify_brt(brt)==0);

    /* Now put a bunch (NVALS) of things in. */
    {
	int i;
	for (i=0; i<NVALS; i++) {
	    char key[100],val[100];
	    DBT k,v;
	    snprintf(key, 100, "key%d", i);
	    snprintf(val, 100, "val%d", i);
	    if (i<600) {
		int verify_result=toku_verify_brt(brt);;
		assert(verify_result==0);
	    }
	    toku_brt_insert(brt, toku_fill_dbt(&k, key, strlen(key)+1), toku_fill_dbt(&v, val, strlen(val)+1), null_txn);
	    if (i<600) {
		int verify_result=toku_verify_brt(brt);
		if (verify_result) {
		    toku_dump_brt(brt);
		    assert(0);
		}
		{
		    int j;
		    for (j=0; j<=i; j++) {
			char expectedval[100];
			snprintf(key, 100, "key%d", j);
			snprintf(expectedval, 100, "val%d", j);
			r=toku_brt_lookup(brt, toku_fill_dbt(&k, key, strlen(key)+1), toku_init_dbt(&v));
			if (r!=0) {
			    if (verbose) printf("%s:%d r=%d on lookup(key=%s) after i=%d\n", __FILE__, __LINE__, r, key, i);
			    toku_dump_brt(brt);
			}
			assert(r==0);
		    }
		}
	    }
	}
    }
    if (verbose) printf("Now read them out\n");

    //show_brt_blocknumbers(brt);
    toku_verify_brt(brt);
    //dump_brt(brt);

    /* See if we can read them all out again. */
    {
	int i;
	for (i=0; i<NVALS; i++) {
	    char key[100],expectedval[100];
	    DBT k,v;
	    snprintf(key, 100, "key%d", i);
	    snprintf(expectedval, 100, "val%d", i);
	    r=toku_brt_lookup(brt, toku_fill_dbt(&k, key, strlen(key)+1), toku_init_dbt(&v));
	    if (r!=0 && verbose) printf("%s:%d r=%d on key=%s\n", __FILE__, __LINE__, r, key);
	    assert(r==0);
	
	}
    }

    r = toku_close_brt(brt); assert(r==0);
    if (verbose) printf("%s:%d About to close %p\n", __FILE__, __LINE__, ct);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);    assert(r==0);
    r = toku_open_brt(n, 0, 0, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db); assert(r==0);

    {
	DBT k,v;
	r = toku_brt_lookup(brt, toku_fill_dbt(&k, "hello", 6), toku_init_dbt(&v));
	assert(r==0);
	assert(v.size==6);
	assert(strcmp(v.data,"there")==0);
    }
    {
	int i;
	for (i=0; i<NVALS; i++) {
	    char key[100],expectedval[100];
	    DBT k,v;
	    snprintf(key, 100, "key%d", i);
	    snprintf(expectedval, 100, "val%d", i);
	    r=toku_brt_lookup(brt, toku_fill_dbt(&k, key, strlen(key)+1), toku_init_dbt(&v));
	    if (r!=0 && verbose) printf("%s:%d r=%d on key=%s\n", __FILE__, __LINE__, r, key);
	    assert(r==0);
	
	}
    }

    r = toku_close_brt(brt); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);

    toku_memory_check_all_free();


    if (verbose) printf(" ok\n");
}

/* Test c_get(DB_LAST) on an empty tree */
static void test_cursor_last_empty(void) {
    const char *n="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;
    if (verbose) printf("%s", __FUNCTION__);
    unlink(n);
    toku_memory_check_all_free();
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);       assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_open_brt(n, 0, 1, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);  assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_brt_cursor(brt, &cursor);            assert(r==0);
    toku_init_dbt(&kbt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    toku_init_dbt(&vbt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_LAST, null_txn);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    assert(r==DB_NOTFOUND);
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_FIRST, null_txn);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    assert(r==DB_NOTFOUND);
    r = toku_close_brt(brt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_cachetable_close(&ct); assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    toku_memory_check_all_free();
}

static void test_cursor_next (void) {
    const char *n="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;

    unlink(n);
    toku_memory_check_all_free();
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);       assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_open_brt(n, 0, 1, &brt, 1<<12, ct, null_txn, toku_default_compare_fun, null_db);  assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_brt_insert(brt, toku_fill_dbt(&kbt, "hello", 6), toku_fill_dbt(&vbt, "there", 6), null_txn);
    r = toku_brt_insert(brt, toku_fill_dbt(&kbt, "byebye", 7), toku_fill_dbt(&vbt, "byenow", 7), null_txn);
    if (verbose) printf("%s:%d calling toku_brt_cursor(...)\n", __FILE__, __LINE__);
    r = toku_brt_cursor(brt, &cursor);            assert(r==0);
    toku_init_dbt(&kbt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    toku_init_dbt(&vbt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();

    if (verbose) printf("%s:%d calling toku_brt_cursor_get(...)\n", __FILE__, __LINE__);
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
    if (verbose) printf("%s:%d called toku_brt_cursor_get(...)\n", __FILE__, __LINE__);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    assert(r==0);
    assert(kbt.size==7);
    assert(memcmp(kbt.data, "byebye", 7)==0);
    assert(vbt.size==7);
    assert(memcmp(vbt.data, "byenow", 7)==0);

    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
    assert(r==0);
    assert(kbt.size==6);
    assert(memcmp(kbt.data, "hello", 6)==0);
    assert(vbt.size==6);
    assert(memcmp(vbt.data, "there", 6)==0);

    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
    assert(r==DB_NOTFOUND);

    r = toku_close_brt(brt);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    r = toku_cachetable_close(&ct); assert(r==0);
    //printf("%s:%d %d alloced\n", __FILE__, __LINE__, toku_get_n_items_malloced()); toku_print_malloced_items();
    toku_memory_check_all_free();

}

static DB nonce_db;

static int wrong_compare_fun(DB *db, const DBT *a, const DBT *b) {
    unsigned int i;
    unsigned char *ad=a->data;
    unsigned char *bd=b->data;
    unsigned int siz=a->size;
    assert(a->size==b->size);
    assert(db==&nonce_db); // make sure the db was passed  down correctly
    for (i=0; i<siz; i++) {
	if (ad[siz-1-i]<bd[siz-1-i]) return -1;
	if (ad[siz-1-i]>bd[siz-1-i]) return +1;
    }
    return 0;

}

static void test_wrongendian_compare (int wrong_p, unsigned int N) {
    const char *n="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;
    unsigned int i;

    unlink(n);
    toku_memory_check_all_free();

    {
	char a[4]={0,1,0,0};
	char b[4]={1,0,0,0};
	DBT at, bt;
	assert(wrong_compare_fun(&nonce_db, toku_fill_dbt(&at, a, 4), toku_fill_dbt(&bt, b, 4))>0);
	assert(wrong_compare_fun(&nonce_db, toku_fill_dbt(&at, b, 4), toku_fill_dbt(&bt, a, 4))<0);
    }

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);       assert(r==0);
    //printf("%s:%d WRONG=%d\n", __FILE__, __LINE__, wrong_p);

    if (0) { // ???? Why is this commented out?
    r = toku_open_brt(n, 0, 1, &brt, 1<<20, ct, null_txn, wrong_p ? wrong_compare_fun : toku_default_compare_fun, &nonce_db);  assert(r==0);
    for (i=1; i<257; i+=255) {
	unsigned char a[4],b[4];
	b[3] = a[0] = i&255;
	b[2] = a[1] = (i>>8)&255;
	b[1] = a[2] = (i>>16)&255;
	b[0] = a[3] = (i>>24)&255;
	toku_fill_dbt(&kbt, a, sizeof(a));
	toku_fill_dbt(&vbt, b, sizeof(b));
	if (verbose)
	    printf("%s:%d insert: %02x%02x%02x%02x -> %02x%02x%02x%02x\n", __FILE__, __LINE__,
		   ((char*)kbt.data)[0], ((char*)kbt.data)[1], ((char*)kbt.data)[2], ((char*)kbt.data)[3],
		   ((char*)vbt.data)[0], ((char*)vbt.data)[1], ((char*)vbt.data)[2], ((char*)vbt.data)[3]);
	r = toku_brt_insert(brt, &kbt, &vbt, null_txn);
	assert(r==0);
    }
    r = toku_brt_cursor(brt, &cursor);            assert(r==0);

    for (i=0; i<2; i++) {
	toku_init_dbt(&kbt); toku_init_dbt(&vbt);
	r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
	assert(r==0);
	assert(kbt.size==4 && vbt.size==4);
	if (verbose)
	    printf("%s:%d %02x%02x%02x%02x -> %02x%02x%02x%02x\n", __FILE__, __LINE__,
		   ((char*)kbt.data)[0], ((char*)kbt.data)[1], ((char*)kbt.data)[2], ((char*)kbt.data)[3],
		   ((char*)vbt.data)[0], ((char*)vbt.data)[1], ((char*)vbt.data)[2], ((char*)vbt.data)[3]);
    }


    r = toku_close_brt(brt);
    }

    {
	toku_cachetable_verify(ct);
	r = toku_open_brt(n, 0, 1, &brt, 1<<20, ct, null_txn, wrong_p ? wrong_compare_fun : toku_default_compare_fun, &nonce_db);  assert(r==0);
	toku_cachetable_verify(ct);

	for (i=0; i<N; i++) {
	    unsigned char a[4],b[4];
	    b[3] = a[0] = i&255;
	    b[2] = a[1] = (i>>8)&255;
	    b[1] = a[2] = (i>>16)&255;
	    b[0] = a[3] = (i>>24)&255;
	    toku_fill_dbt(&kbt, a, sizeof(a));
	    toku_fill_dbt(&vbt, b, sizeof(b));
	    if (0) printf("%s:%d insert: %02x%02x%02x%02x -> %02x%02x%02x%02x\n", __FILE__, __LINE__,
			  ((unsigned char*)kbt.data)[0], ((unsigned char*)kbt.data)[1], ((unsigned char*)kbt.data)[2], ((unsigned char*)kbt.data)[3],
			  ((unsigned char*)vbt.data)[0], ((unsigned char*)vbt.data)[1], ((unsigned char*)vbt.data)[2], ((unsigned char*)vbt.data)[3]);
	    r = toku_brt_insert(brt, &kbt, &vbt, null_txn);
	    assert(r==0);
	    toku_cachetable_verify(ct);
	}
	r = toku_brt_cursor(brt, &cursor);            assert(r==0);
	
	int prev=-1;
	for (i=0; i<N; i++) {
	    int this;
	    toku_init_dbt(&kbt); toku_init_dbt(&vbt);
	    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
	    assert(r==0);
	    assert(kbt.size==4 && vbt.size==4);
	    if (0) printf("%s:%d %02x%02x%02x%02x -> %02x%02x%02x%02x\n", __FILE__, __LINE__,
			  ((unsigned char*)kbt.data)[0], ((unsigned char*)kbt.data)[1], ((unsigned char*)kbt.data)[2], ((unsigned char*)kbt.data)[3],
			  ((unsigned char*)vbt.data)[0], ((unsigned char*)vbt.data)[1], ((unsigned char*)vbt.data)[2], ((unsigned char*)vbt.data)[3]);
	    this= ( (((unsigned char*)kbt.data)[3] << 24) +
		    (((unsigned char*)kbt.data)[2] << 16) +
		    (((unsigned char*)kbt.data)[1] <<  8) +
		    (((unsigned char*)kbt.data)[0] <<  0));
	    assert(prev<this);
	    prev=this;
	    assert(this==(int)i);
	    toku_cachetable_verify(ct);
	}

	r = toku_close_brt(brt);
    }
    r = toku_cachetable_close(&ct); assert(r==0);
    toku_memory_check_all_free();
}

static int test_cursor_debug = 0;

static int test_brt_cursor_keycompare(DB *db __attribute__((unused)), const DBT *a, const DBT *b) {
    return toku_keycompare(a->data, a->size, b->data, b->size);
}

static void assert_cursor_notfound(BRT brt, int position) {
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
    toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, position, null_txn);
    assert(r == DB_NOTFOUND);

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void assert_cursor_value(BRT brt, int position, long long value) {
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;
    long long v;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    if (test_cursor_debug && verbose) printf("key: ");
    toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
    toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, position, null_txn);
    assert(r == 0);
    if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
    assert(vbt.size == sizeof v);
    memcpy(&v, vbt.data, vbt.size);
    assert(v == value);
    toku_free(kbt.data);
    toku_free(vbt.data);
    if (test_cursor_debug && verbose) printf("\n");

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void assert_cursor_first_last(BRT brt, long long firstv, long long lastv) {
    BRT_CURSOR cursor;
    int r;
    DBT kbt, vbt;
    long long v;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    if (test_cursor_debug && verbose) printf("first key: ");
    toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
    toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_FIRST, null_txn);
    assert(r == 0);
    if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
    assert(vbt.size == sizeof v);
    memcpy(&v, vbt.data, vbt.size);
    assert(v == firstv);
    toku_free(kbt.data);
    toku_free(vbt.data);
    if (test_cursor_debug && verbose) printf("\n");

    if (test_cursor_debug && verbose) printf("last key:");
    toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
    toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_LAST, null_txn);
    assert(r == 0);
    if (test_cursor_debug)printf("%s ", (char*)kbt.data);
    assert(vbt.size == sizeof v);
    memcpy(&v, vbt.data, vbt.size);
    assert(v == lastv);
    toku_free(kbt.data);
    toku_free(vbt.data);
    if (test_cursor_debug) printf("\n");

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void test_brt_cursor_first(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_first:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (i=0; i<n; i++) {
        char key[8]; long long v;
        DBT kbt, vbt;

        snprintf(key, sizeof key, "%4.4d", i);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    if (n == 0)
        assert_cursor_notfound(brt, DB_FIRST);
    else
        assert_cursor_value(brt, DB_FIRST, 0);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_brt_cursor_last(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_last:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert keys 0, 1, .. (n-1) */
    for (i=0; i<n; i++) {
        char key[8]; long long v;
        DBT kbt, vbt;

        snprintf(key, sizeof key, "%4.4d", i);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    if (n == 0)
        assert_cursor_notfound(brt, DB_LAST);
    else
        assert_cursor_value(brt, DB_LAST, n-1);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_brt_cursor_first_last(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_first_last:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (i=0; i<n; i++) {
        char key[8]; long long v;
        DBT kbt, vbt;

        snprintf(key, sizeof key, "%4.4d", i);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);

        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    if (n == 0) {
        assert_cursor_notfound(brt, DB_FIRST);
        assert_cursor_notfound(brt, DB_LAST);
    } else
        assert_cursor_first_last(brt, 0, n-1);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);


}

static void test_brt_cursor_rfirst(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_rfirst:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert keys n-1, n-2, ... , 0 */
    for (i=n-1; i>=0; i--) {
        char key[8]; long long v;
        DBT kbt, vbt;


        snprintf(key, sizeof key, "%4.4d", i);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    if (n == 0)
        assert_cursor_notfound(brt, DB_FIRST);
    else
        assert_cursor_value(brt, DB_FIRST, 0);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void assert_cursor_walk(BRT brt, int n) {
    BRT_CURSOR cursor;
    int i;
    int r;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    if (test_cursor_debug && verbose) printf("key: ");
    for (i=0; ; i++) {
        DBT kbt, vbt;
        long long v;

        toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
        toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
        if (r != 0)
            break;
        if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
        assert(vbt.size == sizeof v);
        memcpy(&v, vbt.data, vbt.size);
        assert(v == i);
        toku_free(kbt.data);
        toku_free(vbt.data);
    }
    if (test_cursor_debug && verbose) printf("\n");
    assert(i == n);

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void test_brt_cursor_walk(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_walk:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (i=0; i<n; i++) {
        char key[8]; long long v;
        DBT kbt, vbt;

        snprintf(key, sizeof key, "%4.4d", i);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    /* walk the tree */
    assert_cursor_walk(brt, n);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);

}

static void assert_cursor_rwalk(BRT brt, int n) {
    BRT_CURSOR cursor;
    int i;
    int r;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    if (test_cursor_debug && verbose) printf("key: ");
    for (i=n-1; ; i--) {
        DBT kbt, vbt;
        long long v;

        toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
        toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_PREV, null_txn);
        if (r != 0)
            break;
        if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
        assert(vbt.size == sizeof v);
        memcpy(&v, vbt.data, vbt.size);
        assert(v == i);
        toku_free(kbt.data);
        toku_free(vbt.data);
    }
    if (test_cursor_debug && verbose) printf("\n");
    assert(i == -1);

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void test_brt_cursor_rwalk(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_rwalk:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (i=0; i<n; i++) {
        int k; long long v;
        DBT kbt, vbt;

        k = htonl(i);
        toku_fill_dbt(&kbt, &k, sizeof k);
        v = i;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    /* walk the tree */
    assert_cursor_rwalk(brt, n);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);

}

static void assert_cursor_walk_inorder(BRT brt, int n) {
    BRT_CURSOR cursor;
    int i;
    int r;
    char *prevkey;

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    prevkey = 0;
    if (test_cursor_debug && verbose) printf("key: ");
    for (i=0; ; i++) {
        DBT kbt, vbt;
        long long v;

        toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
        toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
        if (r != 0)
            break;
        if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
        assert(vbt.size == sizeof v);
        memcpy(&v, vbt.data, vbt.size);
        if (i != 0) {
	    assert(strcmp(prevkey, kbt.data) < 0);
	    toku_free(prevkey);
	}
        prevkey = kbt.data;
        toku_free(vbt.data);
    }
    if (prevkey) toku_free(prevkey);
    if (test_cursor_debug && verbose) printf("\n");
    assert(i == n);

    r = toku_brt_cursor_close(cursor);
    assert(r==0);
}

static void test_brt_cursor_rand(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    int r;
    int i;

    if (verbose) printf("test_brt_cursor_rand:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (i=0; i<n; i++) {
        char key[8]; long long v;
        DBT kbt, vbt;

        for (;;) {
	    v = ((long long) random() << 32) + random();
	    snprintf(key, sizeof key, "%lld", v);
	    toku_fill_dbt(&kbt, key, strlen(key)+1);
	    v = i;
	    toku_fill_dbt(&vbt, &v, sizeof v);
	    r = toku_brt_lookup(brt, &kbt, &vbt);
	    if (r == 0) {
                if (verbose) printf("dup");
                continue;
	    }
	    r = toku_brt_insert(brt, &kbt, &vbt, 0);
	    assert(r==0);
	    break;
        }
    }

    /* walk the tree */
    assert_cursor_walk_inorder(brt, n);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);

}

static void test_brt_cursor_split(int n, DB *db) {
    const char *fname="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;
    int r;
    int keyseqnum;
    int i;
    DBT kbt, vbt;

    if (verbose) printf("test_brt_cursor_split:%d %p\n", n, db);

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    /* insert a bunch of kv pairs */
    for (keyseqnum=0; keyseqnum < n/2; keyseqnum++) {
        char key[8]; long long v;

        snprintf(key, sizeof key, "%4.4d", keyseqnum);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = keyseqnum;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    if (test_cursor_debug && verbose) printf("key: ");
    for (i=0; i<n/2; i++) {
        toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
        toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
        assert(r==0);
        if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
        toku_free(kbt.data);
        toku_free(vbt.data);
    }
    if (test_cursor_debug && verbose) printf("\n");

    for (; keyseqnum<n; keyseqnum++) {
        char key[8]; long long v;

        snprintf(key, sizeof key, "%4.4d", keyseqnum);
        toku_fill_dbt(&kbt, key, strlen(key)+1);
        v = keyseqnum;
        toku_fill_dbt(&vbt, &v, sizeof v);
        r = toku_brt_insert(brt, &kbt, &vbt, 0);
        assert(r==0);
    }

    if (test_cursor_debug && verbose) printf("key: ");
    for (;;) {
        toku_init_dbt(&kbt); kbt.flags = DB_DBT_MALLOC;
        toku_init_dbt(&vbt); vbt.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &kbt, &vbt, DB_NEXT, null_txn);
        if (r != 0)
            break;
        if (test_cursor_debug && verbose) printf("%s ", (char*)kbt.data);
        toku_free(kbt.data);
        toku_free(vbt.data);
    }
    if (test_cursor_debug && verbose) printf("\n");

    r = toku_brt_cursor_close(cursor);
    assert(r==0);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_multiple_brt_cursors(int n, DB *db) {
    if (verbose) printf("test_multiple_brt_cursors:%d %p\n", n, db);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursors[n];

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    int i;
    for (i=0; i<n; i++) {
        r = toku_brt_cursor(brt, &cursors[i]);
        assert(r == 0);
    }

    for (i=0; i<n; i++) {
        r = toku_brt_cursor_close(cursors[i]);
        assert(r == 0);
    }

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static int log16(int n) {
    int r = 0;
    int b = 1;
    while (b < n) {
        b *= 16;
        r += 1;
    }
    return r;
}

static void test_multiple_brt_cursor_walk(int n, DB *db) {
    if (verbose) printf("test_multiple_brt_cursor_walk:%d %p\n", n, db);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    const int cursor_gap = 1000;
    const int ncursors = n/cursor_gap;
    BRT_CURSOR cursors[ncursors];

    unlink(fname);

    int nodesize = 1<<12;
    int h = log16(n);
    int cachesize = 2 * h * ncursors * nodesize;
    r = toku_brt_create_cachetable(&ct, cachesize, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    int c;
    /* create the cursors */
    for (c=0; c<ncursors; c++) {
        r = toku_brt_cursor(brt, &cursors[c]);
        assert(r == 0);
    }

    DBT key, val;
    int k, v;

    /* insert keys 0, 1, 2, ... n-1 */
    int i;
    for (i=0; i<n; i++) {
        k = htonl(i);
        v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(brt, &key, &val, 0);
        assert(r == 0);

        /* point cursor i / cursor_gap to the current last key i */
        if ((i % cursor_gap) == 0) {
            c = i / cursor_gap;
            toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
            toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
            r = toku_brt_cursor_get(cursors[c], &key, &val, DB_LAST, null_txn);
            assert(r == 0);
            toku_free(key.data);
            toku_free(val.data);
        }
    }

    /* walk the cursors by cursor_gap */
    for (i=0; i<cursor_gap; i++) {
        for (c=0; c<ncursors; c++) {
            toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
            toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
            r = toku_brt_cursor_get(cursors[c], &key, &val, DB_NEXT, null_txn);
            if (r == DB_NOTFOUND) {
                /* we already consumed 1 previously */
                assert(i == cursor_gap-1);
            } else {
                assert(r == 0);
                int vv;
                assert(val.size == sizeof vv);
                memcpy(&vv, val.data, val.size);
                assert(vv == c*cursor_gap + i + 1);
                toku_free(key.data);
                toku_free(val.data);
            }
        }
    }

    for (i=0; i<ncursors; i++) {
        r = toku_brt_cursor_close(cursors[i]);
        assert(r == 0);
    }

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_brt_cursor_set(int n, int cursor_op, DB *db) {
    if (verbose) printf("test_brt_cursor_set:%d %d %p\n", n, cursor_op, db);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    int i;
    DBT key, val;
    int k, v;

    /* insert keys 0, 10, 20 .. 10*(n-1) */
    for (i=0; i<n; i++) {
        k = htonl(10*i);
        v = 10*i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(brt, &key, &val, 0);
        assert(r == 0);
    }

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    /* set cursor to random keys in set { 0, 10, 20, .. 10*(n-1) } */
    for (i=0; i<n; i++) {
        int vv;

        v = 10*(random() % n);
        k = htonl(v);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, cursor_op, null_txn);
        assert(r == 0);
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == v);
        toku_free(val.data);
        if (cursor_op == DB_SET) assert(key.data == &k);
    }

    /* try to set cursor to keys not in the tree, all should fail */
    for (i=0; i<10*n; i++) {
        if (i % 10 == 0)
            continue;
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, DB_SET, null_txn);
        assert(r == DB_NOTFOUND);
        assert(key.data == &k);
    }

    r = toku_brt_cursor_close(cursor);
    assert(r==0);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_brt_cursor_set_range(int n, DB *db) {
    if (verbose) printf("test_brt_cursor_set_range:%d %p\n", n, db);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(r==0);

    int i;
    DBT key, val;
    int k, v;

    /* insert keys 0, 10, 20 .. 10*(n-1) */
    int max_key = 10*(n-1);
    for (i=0; i<n; i++) {
        k = htonl(10*i);
        v = 10*i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(brt, &key, &val, 0);
        assert(r == 0);
    }

    r = toku_brt_cursor(brt, &cursor);
    assert(r==0);

    /* pick random keys v in 0 <= v < 10*n, the cursor should point
       to the smallest key in the tree that is >= v */
    for (i=0; i<n; i++) {
        int vv;

        v = random() % (10*n);
        k = htonl(v);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, DB_SET_RANGE, null_txn);
        if (v > max_key)
            /* there is no smallest key if v > the max key */
            assert(r == DB_NOTFOUND);
        else {
            assert(r == 0);
            assert(val.size == sizeof vv);
            memcpy(&vv, val.data, val.size);
            assert(vv == (((v+9)/10)*10));
            toku_free(val.data);
        }
    }

    r = toku_brt_cursor_close(cursor);
    assert(r==0);

    r = toku_close_brt(brt);
    assert(r==0);

    r = toku_cachetable_close(&ct);
    assert(r==0);
}

static void test_brt_cursor_delete(int n, DB *db) {
    if (verbose) printf("test_brt_cursor_delete:%d %p\n", n, db);

    int error;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    unlink(fname);

    error = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(error == 0);

    error = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(error == 0);

    error = toku_brt_cursor(brt, &cursor);
    assert(error == 0);

    DBT key, val;
    int k, v;

    int i;
    /* insert keys 0, 1, 2, .. (n-1) */
    for (i=0; i<n; i++) {
        k = htonl(i);
        v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        error = toku_brt_insert(brt, &key, &val, 0);
        assert(error == 0);
    }

    /* walk the tree and delete under the cursor */
    for (;;) {
        toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        error = toku_brt_cursor_get(cursor, &key, &val, DB_NEXT, null_txn);
        if (error == DB_NOTFOUND)
            break;
        assert(error == 0);
        toku_free(key.data);
        toku_free(val.data);

        error = toku_brt_cursor_delete(cursor, 0, null_txn);
        assert(error == 0);
    }

    error = toku_brt_cursor_delete(cursor, 0, null_txn);
    assert(error != 0);

    error = toku_brt_cursor_close(cursor);
    assert(error == 0);

    error = toku_close_brt(brt);
    assert(error == 0);

    error = toku_cachetable_close(&ct);
    assert(error == 0);
}

static void test_brt_cursor_get_both(int n, DB *db) {
    if (verbose) printf("test_brt_cursor_get_both:%d %p\n", n, db);

    int error;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    unlink(fname);

    error = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(error == 0);

    error = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db);
    assert(error == 0);

    error = toku_brt_cursor(brt, &cursor);
    assert(error == 0);

    DBT key, val;
    int k, v;

    /* verify get_both on an empty tree fails */
    k = htonl(n+1);
    v = n+1;
    toku_fill_dbt(&key, &k, sizeof k);
    toku_fill_dbt(&val, &v, sizeof v);
    error = toku_brt_cursor_get(cursor, &key, &val, DB_GET_BOTH, null_txn);
    assert(error == DB_NOTFOUND);

    int i;
    /* insert keys 0, 1, 2, .. (n-1) */
    for (i=0; i<n; i++) {
        k = htonl(i);
        v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        error = toku_brt_insert(brt, &key, &val, 0);
        assert(error == 0);
    }

    /* verify that keys not in the tree fail */
    k = htonl(n+1);
    v = n-1;
    toku_fill_dbt(&key, &k, sizeof k);
    toku_fill_dbt(&val, &v, sizeof v);
    error = toku_brt_cursor_get(cursor, &key, &val, DB_GET_BOTH, null_txn);
    assert(error == DB_NOTFOUND);

    /* verify that key match but data mismatch fails */
    for (i=0; i<n; i++) {
        k = htonl(i);
        v = i+1;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        error = toku_brt_cursor_get(cursor, &key, &val, DB_GET_BOTH, null_txn);
        assert(error == DB_NOTFOUND);
    }

    /* verify that key and data matches succeeds */
    for (i=0; i<n; i++) {
        k = htonl(i);
        v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        error = toku_brt_cursor_get(cursor, &key, &val, DB_GET_BOTH, null_txn);
        assert(error == 0);
#ifdef DB_CURRENT
        toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        error = toku_brt_cursor_get(cursor, &key, &val, DB_CURRENT, 0);
        assert(error == 0);
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == i);
        toku_free(key.data);
        toku_free(val.data);
#endif
        error = toku_brt_cursor_delete(cursor, 0, null_txn);
        assert(error == 0);

        k = htonl(i);
        v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        error = toku_brt_cursor_get(cursor, &key, &val, DB_GET_BOTH, null_txn);
        assert(error == DB_NOTFOUND);
    }

    error = toku_brt_cursor_delete(cursor, 0, null_txn);
    assert(error != 0);

    error = toku_brt_cursor_close(cursor);
    assert(error == 0);

    error = toku_close_brt(brt);
    assert(error == 0);

    error = toku_cachetable_close(&ct);
    assert(error == 0);
}


static int test_brt_cursor_inc = 1000;
static int test_brt_cursor_limit = 10000;

static void test_brt_cursor(DB *db) {
    int n;

    test_multiple_brt_cursors(1, db);
    test_multiple_brt_cursors(2, db);
    test_multiple_brt_cursors(3, db);

    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_first(n, db); toku_memory_check_all_free();
     }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_rfirst(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_walk(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_last(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_first_last(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_split(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_rand(n, db); toku_memory_check_all_free();
    }
    for (n=0; n<test_brt_cursor_limit; n += test_brt_cursor_inc) {
        test_brt_cursor_rwalk(n, db); toku_memory_check_all_free();
    }

    test_brt_cursor_set(1000, DB_SET, db); toku_memory_check_all_free();
    test_brt_cursor_set(10000, DB_SET, db); toku_memory_check_all_free();
    test_brt_cursor_set(1000, DB_SET_RANGE, db); toku_memory_check_all_free();
    test_brt_cursor_set_range(1000, db); toku_memory_check_all_free();
    test_brt_cursor_set_range(10000, db); toku_memory_check_all_free();


    test_brt_cursor_delete(1000, db); toku_memory_check_all_free();
    test_multiple_brt_cursor_walk(10000, db); toku_memory_check_all_free();
    test_multiple_brt_cursor_walk(100000, db); toku_memory_check_all_free();
    test_brt_cursor_get_both(1000, db); toku_memory_check_all_free();
}

static void test_large_kv(int bsize, int ksize, int vsize) {
    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";

    if (verbose) printf("test_large_kv: %d %d %d\n", bsize, ksize, vsize);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, bsize, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key, val;
    char *k, *v;
    k = toku_malloc(ksize); assert(k); memset(k, 0, ksize);
    v = toku_malloc(vsize); assert(v); memset(v, 0, vsize);
    toku_fill_dbt(&key, k, ksize);
    toku_fill_dbt(&val, v, vsize);

    r = toku_brt_insert(t, &key, &val, 0);
    assert(r == 0);

    toku_free(k);
    toku_free(v);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

/*
 * test the key and value limits
 * the current implementation crashes when kvsize == bsize/2 rather than fails
 */
static void test_brt_limits() {
    int bsize = 1024;
    int kvsize = 4;
    while (kvsize < bsize/2) {
        test_large_kv(bsize, kvsize, kvsize);        toku_memory_check_all_free();
        kvsize *= 2;
    }
}

/*
 * verify that a delete on an empty tree fails
 */
static void test_brt_delete_empty() {
    if (verbose) printf("test_brt_delete_empty\n");

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 4096, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key;
    int k = htonl(1);
    toku_fill_dbt(&key, &k, sizeof k);
    r = toku_brt_delete(t, &key, null_txn);
    assert(r == 0);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

/*
 * insert n keys, delete all n keys, verify that lookups for all the keys fail,
 * verify that a cursor walk of the tree finds nothing
 */
static void test_brt_delete_present(int n) {
    if (verbose) printf("test_brt_delete_present:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 4096, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key, val;
    int k, v;

    /* insert 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i); v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(t, &key, &val, 0);
        assert(r == 0);
    }

    /* delete 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        r = toku_brt_delete(t, &key, null_txn);
        assert(r == 0);
    }

    /* lookups should all fail */
    for (i=0; i<n; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val);
        assert(r == DB_NOTFOUND);
    }

    /* cursor should not find anything */
    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor);
    assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &key, &val, DB_FIRST, null_txn);
    assert(r != 0);

    r = toku_brt_cursor_close(cursor);
    assert(r == 0);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

static void test_brt_delete_not_present(int n) {
    if (verbose) printf("test_brt_delete_not_present:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 4096, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key, val;
    int k, v;

    /* insert 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i); v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(t, &key, &val, 0);
        assert(r == 0);
    }

    /* delete 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        r = toku_brt_delete(t, &key, null_txn);
        assert(r == 0);
    }

    /* try to delete key n+1 not in the tree */
    k = htonl(n+1);
    toku_fill_dbt(&key, &k, sizeof k);
    r = toku_brt_delete(t, &key, null_txn);
    /* the delete may be buffered or may be executed on a leaf node, so the
       return value depends */
    if (verbose) printf("toku_brt_delete k=%d %d\n", k, r);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

static void test_brt_delete_cursor_first(int n) {
    if (verbose) printf("test_brt_delete_cursor_first:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 4096, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key, val;
    int k, v;

    /* insert 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i); v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(t, &key, &val, 0);
        assert(r == 0);
    }

    /* lookups 0 .. n-1 should succeed */
    for (i=0; i<n; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val);
        assert(r == 0);
        assert(val.size == sizeof (int));
        int vv;
        memcpy(&vv, val.data, val.size);
        assert(vv == i);
        toku_free(val.data);
    }

    /* delete 0 .. n-2 */
    for (i=0; i<n-1; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        r = toku_brt_delete(t, &key, null_txn);
        assert(r == 0);

        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val);
        assert(r == DB_NOTFOUND);
    }

    /* lookup of 0 .. n-2 should all fail */
    for (i=0; i<n-1; i++) {
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val);
        assert(r == DB_NOTFOUND);
    }

    /* cursor should find the last key: n-1 */
    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor);
    assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
    r = toku_brt_cursor_get(cursor, &key, &val, DB_FIRST, null_txn);
    assert(r == 0);
    int vv;
    assert(val.size == sizeof vv);
    memcpy(&vv, val.data, val.size);
    assert(vv == n-1);
    toku_free(key.data);
    toku_free(val.data);

    r = toku_brt_cursor_close(cursor);
    assert(r == 0);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

/* test for bug: insert cmd in a nonleaf node, delete removes the
   insert cmd, but lookup finds the insert cmd

   build a 2 level tree, and expect the last insertion to be
   buffered. then delete and lookup. */

static void test_insert_delete_lookup(int n) {
    if (verbose) printf("test_insert_delete_lookup:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);
    assert(r==0);
    unlink(fname);
    r = toku_open_brt(fname, 0, 1, &t, 4096, ct, null_txn, toku_default_compare_fun, null_db);
    assert(r==0);

    DBT key, val;
    int k, v;

    /* insert 0 .. n-1 */
    for (i=0; i<n; i++) {
        k = htonl(i); v = i;
        toku_fill_dbt(&key, &k, sizeof k);
        toku_fill_dbt(&val, &v, sizeof v);
        r = toku_brt_insert(t, &key, &val, 0);
        assert(r == 0);
    }

    if (n > 0) {
        k = htonl(n-1);
        toku_fill_dbt(&key, &k, sizeof k);
        r = toku_brt_delete(t, &key, null_txn);
        assert(r == 0);

        k = htonl(n-1);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val);
        assert(r == DB_NOTFOUND);
    }

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

/* insert <0,0>, <0,1>, .. <0,n>
   delete_both <0,i> for all even i
   verify <0,i> exists for all odd i */

void test_brt_delete_both(int n) {
    if (verbose) printf("test_brt_delete_both:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, TOKU_DB_DUP + TOKU_DB_DUPSORT); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, (DB*)0);
    assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(0); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0);
        assert(r == 0);
    }

    for (i=0; i<n; i += 2) {
        k = htonl(0); v = htonl(i);
        r = toku_brt_delete_both(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), null_txn); assert(r == 0);
    }

#if 0
    for (i=1; i<n; i += 2) {
        k = htonl(0);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_lookup(t, &key, &val); assert(r == 0);
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));
        if (val.data) free(val.data);
        r = toku_brt_delete_both(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &vv, sizeof vv), null_txn); assert(r == 0);
    }
#endif

    /* cursor should find only odd pairs */
    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    for (i=1; ; i += 2) {
        toku_init_dbt(&key); key.flags = DB_DBT_MALLOC;
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, DB_NEXT, null_txn);
        if (r != 0) break;
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));
        toku_free(key.data);
        toku_free(val.data);
    }

    r = toku_brt_cursor_close(cursor);  assert(r == 0);

    r = toku_close_brt(t);              assert(r==0);
    r = toku_cachetable_close(&ct);     assert(r==0);
}

static void test_brt_delete() {
    test_brt_delete_empty(); toku_memory_check_all_free();
    test_brt_delete_present(1); toku_memory_check_all_free();
    test_brt_delete_present(100); toku_memory_check_all_free();
    test_brt_delete_present(500); toku_memory_check_all_free();
    test_brt_delete_not_present(1); toku_memory_check_all_free();
    test_brt_delete_not_present(100); toku_memory_check_all_free();
    test_brt_delete_not_present(500); toku_memory_check_all_free();
    test_brt_delete_cursor_first(1); toku_memory_check_all_free();
    test_brt_delete_cursor_first(100); toku_memory_check_all_free();
    test_brt_delete_cursor_first(500); toku_memory_check_all_free();
    test_brt_delete_cursor_first(10000); toku_memory_check_all_free();
    test_insert_delete_lookup(512); toku_memory_check_all_free();
}

static void test_new_brt_cursor_create_close() {
    int r;
    BRT brt;
    int n = 8;
    BRT_CURSOR cursors[n];

    r = toku_brt_create(&brt); assert(r == 0);

    int i;
    for (i=0; i<n; i++) {
        r = toku_brt_cursor(brt, &cursors[i]); assert(r == 0);
    }

    for (i=0; i<n; i++) {
        r = toku_brt_cursor_close(cursors[i]); assert(r == 0);
    }

    r = toku_close_brt(brt); assert(r == 0);
}

static void test_new_brt_cursor_first(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_first:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(i); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_REALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_REALLOC;

    for (i=0; ; i++) {
        r = toku_brt_cursor_get(cursor, &key, &val, DB_FIRST, null_txn);
        if (r != 0) break;
        int kk;
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));

        r = toku_brt_cursor_delete(cursor, 0, null_txn); assert(r == 0);
    }
    assert(i == n);

    if (key.data) toku_free(key.data);
    if (val.data) toku_free(val.data);

    r = toku_brt_cursor_close(cursor); assert(r == 0);
    r = toku_close_brt(t); assert(r==0);
    r = toku_cachetable_close(&ct);assert(r==0);
}

static void test_new_brt_cursor_last(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_last:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(i); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_REALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_REALLOC;

    for (i=n-1; ; i--) {
        r = toku_brt_cursor_get(cursor, &key, &val, DB_LAST, null_txn);
        if (r != 0) break;
        int kk;
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));

        r = toku_brt_cursor_delete(cursor, 0, null_txn); assert(r == 0);
    }
    assert(i == -1);

    if (key.data) toku_free(key.data);
    if (val.data) toku_free(val.data);

    r = toku_brt_cursor_close(cursor); assert(r == 0);
    r = toku_close_brt(t); assert(r==0);
    r = toku_cachetable_close(&ct);assert(r==0);
}

static void test_new_brt_cursor_next(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_next:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(i); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    toku_init_dbt(&key); key.flags = DB_DBT_REALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_REALLOC;

    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    for (i=0; ; i++) {
        r = toku_brt_cursor_get(cursor, &key, &val, DB_NEXT, null_txn);
        if (r != 0) break;
        int kk;
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));
    }
    assert(i == n);

    if (key.data) toku_free(key.data);
    if (val.data) toku_free(val.data);

    r = toku_brt_cursor_close(cursor); assert(r == 0);
    r = toku_close_brt(t); assert(r==0);
    r = toku_cachetable_close(&ct);assert(r==0);
}

static void test_new_brt_cursor_prev(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_prev:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(i); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_REALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_REALLOC;

    for (i=n-1; ; i--) {
        r = toku_brt_cursor_get(cursor, &key, &val, DB_PREV, null_txn);
        if (r != 0) break;
        int kk;
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));
    }
    assert(i == -1);

    if (key.data) toku_free(key.data);
    if (val.data) toku_free(val.data);

    r = toku_brt_cursor_close(cursor); assert(r == 0);
    r = toku_close_brt(t); assert(r==0);
    r = toku_cachetable_close(&ct);assert(r==0);
}

static void test_new_brt_cursor_current(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_current:%d\n", n);

    BRT t;
    int r;
    CACHETABLE ct;
    char fname[]="testbrt.brt";
    int i;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&t); assert(r == 0);
    r = toku_brt_set_flags(t, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(t, 4096); assert(r == 0);
    r = toku_brt_open(t, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    DBT key, val;
    int k, v;

    for (i=0; i<n; i++) {
        k = htonl(i); v = htonl(i);
        r = toku_brt_insert(t, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    BRT_CURSOR cursor;

    r = toku_brt_cursor(t, &cursor); assert(r == 0);

    toku_init_dbt(&key); key.flags = DB_DBT_REALLOC;
    toku_init_dbt(&val); val.flags = DB_DBT_REALLOC;

    for (i=0; ; i++) {
        r = toku_brt_cursor_get(cursor, &key, &val, DB_FIRST, null_txn);
        if (r != 0) break;
        int kk;
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        int vv;
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));

        r = toku_brt_cursor_get(cursor, &key, &val, DB_CURRENT, null_txn); assert(r == 0);
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));

        r = toku_brt_cursor_get(cursor, &key, &val, DB_CURRENT+256, null_txn); assert(r == 0);
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));

        r = toku_brt_cursor_delete(cursor, 0, null_txn); assert(r == 0);

        r = toku_brt_cursor_get(cursor, &key, &val, DB_CURRENT, null_txn); assert(r == DB_KEYEMPTY);

        r = toku_brt_cursor_get(cursor, &key, &val, DB_CURRENT+256, null_txn); assert(r == 0);
        assert(key.size == sizeof kk);
        memcpy(&kk, key.data, key.size);
        assert(kk == (int) htonl(i));
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == (int) htonl(i));
    }
    assert(i == n);

    if (key.data) toku_free(key.data);
    if (val.data) toku_free(val.data);

    r = toku_brt_cursor_close(cursor); assert(r == 0);
    r = toku_close_brt(t); assert(r==0);
    r = toku_cachetable_close(&ct);assert(r==0);
}

static void test_new_brt_cursor_set_range(int n, int dup_mode) {
    if (verbose) printf("test_brt_cursor_set_range:%d %d\n", n, dup_mode);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);
    unlink(fname);
    r = toku_brt_create(&brt); assert(r == 0);
    r = toku_brt_set_flags(brt, dup_mode); assert(r == 0);
    r = toku_brt_set_nodesize(brt, 4096); assert(r == 0);
    r = toku_brt_open(brt, fname, fname, 0, 1, 1, 0, ct, null_txn, 0); assert(r==0);

    int i;
    DBT key, val;
    int k, v;

    /* insert keys 0, 10, 20 .. 10*(n-1) */
    int max_key = 10*(n-1);
    for (i=0; i<n; i++) {
        k = htonl(10*i);
        v = 10*i;
        r = toku_brt_insert(brt, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    r = toku_brt_cursor(brt, &cursor); assert(r==0);

    /* pick random keys v in 0 <= v < 10*n, the cursor should point
       to the smallest key in the tree that is >= v */
    for (i=0; i<n; i++) {
        int vv;

        v = random() % (10*n);
        k = htonl(v);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, DB_SET_RANGE, null_txn);
        if (v > max_key)
            /* there is no smallest key if v > the max key */
            assert(r == DB_NOTFOUND);
        else {
            assert(r == 0);
            assert(val.size == sizeof vv);
            memcpy(&vv, val.data, val.size);
            assert(vv == (((v+9)/10)*10));
            toku_free(val.data);
        }
    }

    r = toku_brt_cursor_close(cursor); assert(r==0);

    r = toku_close_brt(brt); assert(r==0);

    r = toku_cachetable_close(&ct); assert(r==0);
}

static void test_new_brt_cursor_set(int n, int cursor_op, DB *db) {
    if (verbose) printf("test_brt_cursor_set:%d %d %p\n", n, cursor_op, db);

    int r;
    char fname[]="testbrt.brt";
    CACHETABLE ct;
    BRT brt;
    BRT_CURSOR cursor;

    unlink(fname);

    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER); assert(r==0);

    r = toku_open_brt(fname, 0, 1, &brt, 1<<12, ct, null_txn, test_brt_cursor_keycompare, db); assert(r==0);

    int i;
    DBT key, val;
    int k, v;

    /* insert keys 0, 10, 20 .. 10*(n-1) */
    for (i=0; i<n; i++) {
        k = htonl(10*i);
        v = 10*i;
        r = toku_brt_insert(brt, toku_fill_dbt(&key, &k, sizeof k), toku_fill_dbt(&val, &v, sizeof v), 0); assert(r == 0);
    }

    r = toku_brt_cursor(brt, &cursor); assert(r==0);

    /* set cursor to random keys in set { 0, 10, 20, .. 10*(n-1) } */
    for (i=0; i<n; i++) {
        int vv;

        v = 10*(random() % n);
        k = htonl(v);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, cursor_op, null_txn);
        assert(r == 0);
        assert(val.size == sizeof vv);
        memcpy(&vv, val.data, val.size);
        assert(vv == v);
        toku_free(val.data);
        if (cursor_op == DB_SET) assert(key.data == &k);
    }

    /* try to set cursor to keys not in the tree, all should fail */
    for (i=0; i<10*n; i++) {
        if (i % 10 == 0)
            continue;
        k = htonl(i);
        toku_fill_dbt(&key, &k, sizeof k);
        toku_init_dbt(&val); val.flags = DB_DBT_MALLOC;
        r = toku_brt_cursor_get(cursor, &key, &val, DB_SET, null_txn);
        assert(r == DB_NOTFOUND);
        assert(key.data == &k);
    }

    r = toku_brt_cursor_close(cursor); assert(r==0);

    r = toku_close_brt(brt); assert(r==0);

    r = toku_cachetable_close(&ct); assert(r==0);
}

static void test_new_brt_cursors(int dup_mode) {
    test_new_brt_cursor_create_close(dup_mode);   toku_memory_check_all_free();
    test_new_brt_cursor_first(8, dup_mode);       toku_memory_check_all_free();
    test_new_brt_cursor_last(8, dup_mode);        toku_memory_check_all_free();
    test_new_brt_cursor_last(512, dup_mode);      toku_memory_check_all_free();
    test_new_brt_cursor_next(8, dup_mode);        toku_memory_check_all_free();
    test_new_brt_cursor_prev(8, dup_mode);        toku_memory_check_all_free();
    test_new_brt_cursor_current(8, dup_mode);     toku_memory_check_all_free();
    test_new_brt_cursor_next(512, dup_mode);      toku_memory_check_all_free();
    test_new_brt_cursor_set_range(512, dup_mode); toku_memory_check_all_free();
    test_new_brt_cursor_set(512, DB_SET, 0);      toku_memory_check_all_free();
};

static void brt_blackbox_test (void) {
    toku_memory_check = 1;
    test_wrongendian_compare(0, 2);          toku_memory_check_all_free();
    test_wrongendian_compare(1, 2);          toku_memory_check_all_free();
    test_wrongendian_compare(1, 257);        toku_memory_check_all_free();
    test_wrongendian_compare(1, 1000);        toku_memory_check_all_free();

    test_new_brt_cursors(0);
    test_new_brt_cursors(TOKU_DB_DUP+TOKU_DB_DUPSORT);
    test_brt_delete_both(512);               toku_memory_check_all_free();

    test_read_what_was_written();         toku_memory_check_all_free(); if (verbose) printf("did read_what_was_written\n");
    test_cursor_next();                   toku_memory_check_all_free();
    test_multiple_dbs_many();             toku_memory_check_all_free();
    test_cursor_last_empty();             toku_memory_check_all_free();
    test_multiple_brts_one_db_one_file(); toku_memory_check_all_free();
    test_dump_empty_db();                 toku_memory_check_all_free();
    test_named_db();
    toku_memory_check_all_free();
    test_multiple_dbs();
    toku_memory_check_all_free();
    if (verbose) printf("test0 A\n");
    test0();
    if (verbose) printf("test0 B\n");
    test0(); /* Make sure it works twice. */
    if (verbose) printf("test1\n");
    test1();
    if (verbose) printf("test2 checking memory\n");
    test2(1);
    if (verbose) printf("test2 faster\n");
    test2(0);
    if (verbose) printf("test5\n");
    test5();
    if (verbose) printf("test_multiple_files\n");
    test_multiple_files();
    if (verbose) printf("test3 slow\n");
    toku_memory_check=0;
    test3(2048, 1<<15, 1);
    if (verbose) printf("test4 slow\n");
    test4(2048, 1<<15, 1);
    if (verbose) printf("test3 fast\n");

    if (verbose) toku_pma_show_stats();

    test3(1<<15, 1024, 1);
    test4(1<<15, 1024, 1);
    if (verbose) printf("test3 fast\n");

    test3(1<<18, 1<<20, 0);
    test4(1<<18, 1<<20, 0);

    // Once upon a time srandom(8) caused this test to fail.
    srandom(8); test4(2048, 1<<15, 1);

    toku_memory_check = 1;

    test_brt_limits();

    DB a_db;
    DB *db = &a_db;
    test_brt_cursor(db);

    test_brt_delete();

    int old_brt_do_push_cmd = toku_brt_do_push_cmd;
    toku_brt_do_push_cmd = 0;

    test_brt_delete();
    test_brt_cursor(db);

    toku_brt_do_push_cmd = old_brt_do_push_cmd;

//    test3(1<<19, 1<<20, 0);
//    test4(1<<19, 1<<20, 0);

//    test3(1<<20, 1<<20, 0);
//    test4(1<<20, 1<<20, 0);

//    test3(1<<20, 1<<21, 0);
//    test4(1<<20, 1<<21, 0);

//    test3(1<<20, 1<<22, 0);
//    test4(1<<20, 1<<22, 0);

}

int main (int argc , const char *argv[]) {
    default_parse_args(argc, argv);
    brt_blackbox_test();
    toku_malloc_cleanup();
    if (verbose) printf("test ok\n");
    return 0;
}
