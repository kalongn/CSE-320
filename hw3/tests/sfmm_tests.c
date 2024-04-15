#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
	int cnt = 0;
	for (int i = 0; i < NUM_FREE_LISTS; i++) {
		sf_block *bp = sf_free_list_heads[i].body.links.next;
		while (bp != &sf_free_list_heads[i]) {
			if (size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
				cnt++;
			bp = bp->body.links.next;
		}
	}
	if (size == 0) {
		cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
			count, cnt);
	} else {
		cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
			size, count, cnt);
	}
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
	int cnt = 0;
	sf_block *bp = sf_free_list_heads[index].body.links.next;
	while (bp != &sf_free_list_heads[index]) {
		cnt++;
		bp = bp->body.links.next;
	}
	cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		index, size, cnt);
}

/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
void assert_quick_list_block_count(size_t size, int count) {
	int cnt = 0;
	for (int i = 0; i < NUM_QUICK_LISTS; i++) {
		sf_block *bp = sf_quick_lists[i].first;
		while (bp != NULL) {
			if (size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
				cnt++;
			bp = bp->body.links.next;
		}
	}
	if (size == 0) {
		cr_assert_eq(cnt, count, "Wrong number of quick list blocks (exp=%d, found=%d)",
			count, cnt);
	} else {
		cr_assert_eq(cnt, count, "Wrong number of quick list blocks of size %ld (exp=%d, found=%d)",
			size, count, cnt);
	}
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + 8192 == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header and the link pointers.
	void *x = sf_malloc(32704);
	cr_assert_not_null(x, "x is NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(163840);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(163792, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_quick, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 32, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(8032, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(7872, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(528, 1);
	assert_free_block_count(7552, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
	size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 500, sz_y = 200, sz_z = 700;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 4);
	assert_free_block_count(208, 3);
	assert_free_block_count(5968, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 16,
		"Wrong first block in free list %d: (found=%p, exp=%p)",
		i, bp, (char *)y - 16);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
	size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96,
		"Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		(bp->header ^ MAGIC) & ~0xf, 96);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(7984, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
	size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)y - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96,
		"Block size (0x%ld) not what was expected (0x%ld)!",
		(bp->header ^ MAGIC) & ~0xf, 96);

  // There should be only one free block.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8048, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
	size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)y - 16);
	cr_assert((bp->header ^ MAGIC) & 0x2, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 32,
		"Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		(bp->header ^ MAGIC) & ~0xf, 32);

  // After realloc'ing x, we can return a block of size ADJUSTED_BLOCK_SIZE(sz_x) - ADJUSTED_BLOCK_SIZE(sz_y)
  // to the freelist.  This block will go into the main freelist and be coalesced.
  // Note that we don't put split blocks into the quick lists because their sizes are not sizes
  // that were requested by the client, so they are not very likely to satisfy a new request.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE OR OTHERWISE MANGLE THESE COMMENTS
//############################################

/**
 * @brief it xor the size_t number with the MAGIC number
 *
 * @param number
 *      the header / footer content most likely
 * @return size_t
 *      the xor with magic number version of the content
 */
size_t xor_magic(size_t number) {
	return number ^ MAGIC;
}

/**
 * @brief return the block structure address as intended in the sf_block structure
 *
 * @param pointer
 * 		the pointer to the content returned by sf_malloc or sf_realloc
 * @return sf_block*
 * 		return the address of the start of the point address
 */
sf_block *block_address_from_pointer(void *pointer) {
	return (sf_block *)(pointer - 16);
}

/**
 * @brief assert that the PREV_BLOCK_ALLOCATED in the block header is on.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_prev_bit_on(sf_block *block) {
	cr_assert(xor_magic(block->header) & PREV_BLOCK_ALLOCATED, "Prev alloc bit is not set!");
}

/**
 * @brief assert that the PREV_BLOCK_ALLOCATED in the block header is off.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_prev_bit_off(sf_block *block) {
	cr_assert(xor_magic(block->header) & (~PREV_BLOCK_ALLOCATED), "Prev alloc bit is set!");
}

/**
 * @brief assert that the THIS_BLOCK_ALLOCATED in the block header is on.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_allocate_bit_on(sf_block *block) {
	cr_assert(xor_magic(block->header) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
}

/**
 * @brief assert that the THIS_BLOCK_ALLOCATED in the block header is off.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_allocate_bit_off(sf_block *block) {
	cr_assert(xor_magic(block->header) & (~THIS_BLOCK_ALLOCATED), "Allocated bit is set!");
}

/**
 * @brief assert that the IN_QUICK_LIST in the block header is on.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_quick_bit_on(sf_block *block) {
	cr_assert(xor_magic(block->header) & IN_QUICK_LIST, "Quick bit is not set!");
}

/**
 * @brief assert that the IN_QUICK_LIST in the block header is off.
 *
 * @param block
 * 		the address of the start of the block.
 */
void assert_quick_bit_off(sf_block *block) {
	cr_assert(xor_magic(block->header) & (~IN_QUICK_LIST), "Quick bit is set!");
}

/**
 * @brief assert that the size of the block matches the expected_size.
 *
 * @param block
 * 		the address of the start of the block
 * @param expect_size
 * 		the expect dimension of the block.
 */
void assert_block_size(sf_block *block, size_t expect_size) {
	size_t current_size = xor_magic(block->header) & (~0xf);
	cr_assert(current_size == expect_size,
		"Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		current_size, expect_size);
}

Test(sfmm_student_suite, malloc_0_byte, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = 0;
	int *x = sf_malloc(sz);

	cr_assert_null(x, "x is not NULL!");
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_student_suite, malloc_1_byte_0_byte, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sx = 1, sy = 0;
	char *x = sf_malloc(sx);
	int *y = sf_malloc(sy);
	cr_assert_not_null(x, "Memory allocation for x failed.");
	*x = 'L';
	cr_assert_null(y, "Memory allocation for y should fail with 0 size.");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);

	cr_assert(*x == 'L', "Value assigned to x is not 'L'.");
	sf_block *block_of_x = block_address_from_pointer(x);
	assert_block_size(block_of_x, 32);
	assert_allocate_bit_on(block_of_x);
	assert_quick_bit_off(block_of_x);
	assert_prev_bit_on(block_of_x);

	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	double exp_util = (double)(32 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_entire_page, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sx = 8136;
	void *x = sf_malloc(sx);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	sf_block *block_of_x = block_address_from_pointer(x);
	assert_block_size(block_of_x, 8144);
	assert_allocate_bit_on(block_of_x);
	assert_quick_bit_off(block_of_x);
	assert_prev_bit_on(block_of_x);

	double exp_util = (double)(8144 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_two_page_sequentially, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sx = 8136, sy = 8184;
	void *x = sf_malloc(sx);
	void *y = sf_malloc(sy);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	sf_block *block_of_x = block_address_from_pointer(x);
	assert_block_size(block_of_x, 8144);
	assert_allocate_bit_on(block_of_x);
	assert_quick_bit_off(block_of_x);
	assert_prev_bit_on(block_of_x);

	sf_block *block_of_y = block_address_from_pointer(y);
	assert_block_size(block_of_y, 8192);
	assert_allocate_bit_on(block_of_y);
	assert_quick_bit_off(block_of_y);
	assert_prev_bit_on(block_of_y);

	double exp_util = (double)(sx + sy) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_and_check_contnet, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	const int size = 6;
	typedef struct test_block {
		int arr[size];
	} test_block;
	size_t sx = sizeof(test_block);
	test_block *block_one = sf_malloc(sx);
	for (int i = 0; i < size; i++) {
		block_one->arr[i] = i;
	}
	test_block *block_two = sf_malloc(sx);
	for (int i = 0; i < size; i++) {
		block_two->arr[i] = i * 10;
	}
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8080, 1);
	for (int i = 0; i < size; i++) {
		cr_assert(block_one->arr[i] == i, "block_one content not the name.");
		cr_assert(block_two->arr[i] == i * 10, "block_one content not the name.");
	}
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	double exp_util = (double)(2 * sizeof(test_block)) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_from_quick_list, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sx = sizeof(int);
	void *test = sf_malloc(sx);
	sf_free(test);
	// test free
	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
	// test malloc
	test = sf_malloc(sx);
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	double exp_util = (double)(24) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_split_32_exactly, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *a = sf_malloc(232);
	sf_malloc(4000);
	sf_free(a);
	sf_malloc(200);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(32, 1);
	assert_free_block_count(3888, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	double exp_util = (double)(232 + 4016 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_success_then_out_of_bound, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	sf_malloc(sizeof(int));
	void *x = sf_malloc(163840);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(163760, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_student_suite, free_and_flush_quick_in_order, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_y = 32;
	void *a = sf_malloc(sz_y);
	void *b = sf_malloc(sz_y);
	void *c = sf_malloc(sz_y);
	void *d = sf_malloc(sz_y);
	void *e = sf_malloc(sz_y);
	void *f = sf_malloc(sz_y);

	sf_free(a);
	sf_free(b);
	sf_free(c);
	sf_free(d);
	sf_free(e);
	sf_free(f);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 2);
	assert_free_block_count(240, 1);
	assert_free_block_count(7856, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)((48 - 8) * 6) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, free_and_flush_quick_reverse_order, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_y = 32;
	void *a = sf_malloc(sz_y);
	void *b = sf_malloc(sz_y);
	void *c = sf_malloc(sz_y);
	void *d = sf_malloc(sz_y);
	void *e = sf_malloc(sz_y);
	void *f = sf_malloc(sz_y);

	sf_free(f);
	sf_free(e);
	sf_free(d);
	sf_free(c);
	sf_free(b);
	sf_free(a);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(8096, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)((48 - 8) * 6) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, free_invalid_1_0, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_free(0);
}

Test(sfmm_student_suite, free_invalid_2_NULL, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_free(NULL);
}

Test(sfmm_student_suite, free_invalid_3_mem_start, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_malloc(1);
	void *start = sf_mem_start();
	sf_free(start);
}

Test(sfmm_student_suite, free_invalid_4_mem_end, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_malloc(1);
	void *start = sf_mem_end();
	sf_free(start);
}

Test(sfmm_student_suite, free_invalid_5_header_messed_up, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	sf_header *header = x - 8;
	*header = (0 ^ MAGIC);
	sf_free(x);
}

Test(sfmm_student_suite, free_invalid_6_header_allocation_bit_messed_up, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	sf_header *header = x - 8;
	*header = xor_magic(xor_magic(*header) & (~THIS_BLOCK_ALLOCATED));
	sf_free(x);
}

Test(sfmm_student_suite, free_invalid_prev_block_allocated_but_current_block_say_it_is_free, .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int)); // 32
	sf_header *header = x - 8;
	*header = xor_magic(xor_magic(*header) & (~PREV_BLOCK_ALLOCATED));

	// the issue is that since footer_of_prev is the footer of prologue, it is never modified hence it contains garbage values.
	// when we attempted to read garbage value, the footer may've indicated that the block is free if the bit of THIS_BLOCK_ALLOCATED
	// is set to off.

	// hence making this "required" to test the condition
	sf_footer *footer_of_prev = (void *)header - 8;
	*footer_of_prev = xor_magic(xor_magic(*footer_of_prev) | THIS_BLOCK_ALLOCATED);

	sf_free(x);
}

Test(sfmm_student_suite, free_overall_test, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	void *y = sf_malloc(8144);
	sf_free(x);
	y = sf_realloc(y, 16000);
	y = sf_realloc(y, 0);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(24496, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(8160 - 8 + 16016 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_free_quick_list_all, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t size = 32;
	for (int i = 0; i < NUM_QUICK_LISTS; i++) {
		void *ptr_arr[6];
		for (int j = 0; j < QUICK_LIST_MAX + 1; j++) {
			ptr_arr[j] = sf_malloc(size - 8 + i * 16);
		}
		for (int j = 0; j < QUICK_LIST_MAX + 1; j++) {
			sf_free(ptr_arr[QUICK_LIST_MAX - j]);
		}
	}
	assert_quick_list_block_count(0, 10);
	for (int i = 0; i < NUM_QUICK_LISTS; i++) {
		assert_quick_list_block_count(size + 16 * i, 1);
	}
	assert_free_block_count(0, 1);
	assert_free_block_count(7104, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)((176 - 8) * (QUICK_LIST_MAX + 1)) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_free_coal_dont_mess_with_prologue, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(4056);
	void *y = sf_malloc(4056);
	sf_free(x);
	sf_free(y);
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8144, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(4064 - 8 + 4080 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, malloc_free_coal_dont_mess_with_epilogue, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(4056);
	void *y = sf_malloc(4056);
	sf_free(y);
	sf_free(x);
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(8144, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(4064 - 8 + 4080 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, realloc_invalid_1_0, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	sf_realloc(0, 0);
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL!");
}

Test(sfmm_student_suite, realloc_invalid_2_NULL, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	sf_realloc(NULL, 0);
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL!");
}

Test(sfmm_student_suite, realloc_act_as_free, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(24);
	void *y = sf_malloc(200);
	sf_realloc(x, 0);
	sf_realloc(y, 0);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");

	double exp_util = (double)(32 - 8 + 208 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, black_box_test_1, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int)); // 32 - 8
	void *y = sf_malloc(8144); // 8160 - 8 + 32 - 8
	void *z = sf_malloc(sizeof(char)); // 32 - 8 + 8160 - 8 + 32 - 8
	sf_free(y); // 32 - 8 + 32 - 8
	x = sf_realloc(x, 1000); // 32 - 8 + 1008 - 8 + 32 - 8
	z = sf_realloc(z, 8192); // 1008 - 8 + 32 - 8 + 8208 - 8
	sf_free(x); // 8208 - 8
	sf_free(z); // 0

	x = sf_malloc(168); // 176 - 8
	z = sf_malloc(56); // 176 - 8 + 64 - 8

	sf_free(x); // 64 - 8
	sf_free(z); // 0

	assert_quick_list_block_count(0, 4);
	assert_quick_list_block_count(32, 2);
	assert_quick_list_block_count(64, 1);
	assert_quick_list_block_count(176, 1);

	assert_free_block_count(0, 2);
	assert_free_block_count(7920, 1);
	assert_free_block_count(16304, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(1008 - 8 + 32 - 8 + 8208 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, black_box_test_2, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(100); // 112 - 8
	x = sf_realloc(x, 24); // 32 - 8
	void *y = sf_malloc(24); // 32 - 8 + 32 - 8
	sf_free(x); // 32 - 8
	sf_free(y); // 0

	assert_quick_list_block_count(0, 2);
	assert_quick_list_block_count(32, 2);
	assert_free_block_count(0, 1);
	assert_free_block_count(8080, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(112 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}

Test(sfmm_student_suite, black_box_test_3, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(200); // 208 - 8
	x = sf_realloc(x, 24); // 32 - 8
	void *y = sf_malloc(24); // 32 - 8 + 32 - 8
	void *z = sf_malloc(3016); // 3024 - 8 + 32 - 8 + 32 - 8
	y = sf_realloc(y, 10008); // 10016 - 8 + 3024 - 8 + 32 - 8 + 32 - 8
	y = sf_realloc(y, 10000); // 10016 - 8 + 3024 - 8 + 32 - 8
	y = sf_realloc(y, 24); // 3024 - 8 + 32 - 8 + 32 - 8
	sf_free(x); // 3024 - 8 + 32 - 8
	sf_free(y); // 3024 - 8
	sf_free(z); // 0

	assert_quick_list_block_count(0, 3);
	assert_quick_list_block_count(32, 3);
	assert_free_block_count(0, 2);
	assert_free_block_count(3024, 1);
	assert_free_block_count(13216, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");

	double exp_util = (double)(10016 - 8 + 3024 - 8 + 32 - 8 + 32 - 8) / (double)(sf_mem_end() - sf_mem_start());
	double act_util = sf_utilization();
	cr_assert(act_util == exp_util, "act_util = %f while expect value = %f", act_util, exp_util);
}
