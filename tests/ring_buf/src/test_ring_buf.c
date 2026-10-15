/*
 * Ring Buffer Module - Unit Tests (L08 Task 1)
 *
 * All tests follow TEST_SPEC.md. test_fresh_state was provided as the
 * worked example; the other seven are implemented below.
 *
 * Run:
 *   west twister -T tests/ring_buf -p native_sim -v
 */

#include <zephyr/ztest.h>
#include <errno.h>

#include "ring_buf.h"

/*
 * Shared before hook: every suite reinitialises the ring buffer with a
 * capacity of 4 so tests start from a clean, known state. Capacity 4 is
 * enough to exercise FIFO order (push 1, 2, 3) and overflow (full at 4).
 */
static void before(void *f)
{
	ARG_UNUSED(f);
	rb_init(4);
}

/*
 * ============================================================================
 * Test Suite: ring_buf_init
 *
 * Initial state and re-initialization behaviour.
 * ============================================================================
 */
ZTEST_SUITE(ring_buf_init, NULL, NULL, before, NULL, NULL);

/* PROVIDED — study this test before writing the rest. */
ZTEST(ring_buf_init, test_fresh_state)
{
	zassert_true(rb_is_empty(), "Fresh buffer must be empty");
	zassert_equal(rb_count(), 0, "Fresh buffer count must be 0");
}

/* TEST_SPEC "Suite ring_buf_init" #2 */
ZTEST(ring_buf_init, test_reinit_clears_state)
{
	/*
	 * The push is setup, not the thing being verified, so it uses zassume:
	 * if it fails the test is skipped as "not my job" instead of failing.
	 */
	zassume_ok(rb_push(99), "Precondition: push(99) must succeed");

	rb_init(4);

	zassert_true(rb_is_empty(), "Buffer must be empty after reinit");
	zassert_equal(rb_count(), 0, "Count must be 0 after reinit");
}

/*
 * ============================================================================
 * Test Suite: ring_buf_push_pop
 *
 * Single push/pop round-trip, FIFO order, full error path.
 * ============================================================================
 */
ZTEST_SUITE(ring_buf_push_pop, NULL, NULL, before, NULL, NULL);

/* TEST_SPEC "Suite ring_buf_push_pop" #1 */
ZTEST(ring_buf_push_pop, test_single_push_pop)
{
	int v = 0;

	zassert_ok(rb_push(42), "push(42) must succeed");
	zassert_ok(rb_pop(&v), "pop must succeed");
	zassert_equal(v, 42, "Popped %d, expected 42", v);
	zassert_true(rb_is_empty(), "Buffer must be empty after the pop");
}

/* TEST_SPEC "Suite ring_buf_push_pop" #2 */
ZTEST(ring_buf_push_pop, test_fifo_order)
{
	int v = 0;

	for (int i = 1; i <= 3; i++) {
		zassert_ok(rb_push(i), "push(%d) must succeed", i);
	}

	for (int expected = 1; expected <= 3; expected++) {
		zassert_ok(rb_pop(&v), "pop must succeed");
		zassert_equal(v, expected,
			      "FIFO order broken: popped %d, expected %d",
			      v, expected);
	}

	zassert_true(rb_is_empty(), "Buffer must be empty after three pops");
}

/* TEST_SPEC "Suite ring_buf_push_pop" #3 */
ZTEST(ring_buf_push_pop, test_push_full_returns_enospc)
{
	for (int i = 1; i <= 4; i++) {
		zassert_ok(rb_push(i), "push(%d) must succeed", i);
	}

	zassert_true(rb_is_full(), "Buffer must be full after 4 pushes");
	zassert_equal(rb_push(99), -ENOSPC,
		      "push to a full buffer must return -ENOSPC");
	zassert_equal(rb_count(), 4,
		      "Rejected push must not consume a slot");
}

/*
 * ============================================================================
 * Test Suite: ring_buf_boundaries
 *
 * Peek semantics and NULL-pointer boundary conditions.
 * ============================================================================
 */
ZTEST_SUITE(ring_buf_boundaries, NULL, NULL, before, NULL, NULL);

/* TEST_SPEC "Suite ring_buf_boundaries" #1 */
ZTEST(ring_buf_boundaries, test_peek_does_not_consume)
{
	int v = 0;

	zassume_ok(rb_push(7), "Precondition: push(7) must succeed");

	zassert_ok(rb_peek(&v), "First peek must succeed");
	zassert_equal(v, 7, "First peek returned %d, expected 7", v);

	v = 0; /* clear it, so the second peek has to write the value again */
	zassert_ok(rb_peek(&v), "Second peek must succeed");
	zassert_equal(v, 7, "Second peek returned %d, expected 7", v);

	zassert_equal(rb_count(), 1, "Peek must not consume the element");
}

/* TEST_SPEC "Suite ring_buf_boundaries" #2 */
ZTEST(ring_buf_boundaries, test_pop_null_returns_einval)
{
	zassert_equal(rb_pop(NULL), -EINVAL,
		      "rb_pop(NULL) must return -EINVAL");
}

/* TEST_SPEC "Suite ring_buf_boundaries" #3 */
ZTEST(ring_buf_boundaries, test_is_full_after_fill)
{
	for (int i = 1; i <= 4; i++) {
		zassert_ok(rb_push(i), "push(%d) must succeed", i);
	}

	zassert_true(rb_is_full(), "Buffer must report full after 4 pushes");
	zassert_equal(rb_count(), 4, "Count must be 4 when full");
}
