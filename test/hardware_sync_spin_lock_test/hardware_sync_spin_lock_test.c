#include "pico/test.h"
#include "pico/test/xrand.h"

#include "hardware/sync.h"
#include "hardware/sync/spin_lock.h"
#include "pico/multicore.h"
#include "pico/stdio.h"

#include <stdio.h>

uint counter_local[NUM_CORES][NUM_SPIN_LOCKS];
uint counter_shared[NUM_SPIN_LOCKS];

typedef struct test {
	const char *name;
	void (*prepare)();
	void (*run_per_core)();
	// Return true for ok:
	bool (*check)();
} test_t;

// Increase until fear turns to boredom:
static const uint ITERATIONS = 0x40000;

void prepare_clear_counters(void) {
	__mem_fence_acquire();
	for (int i = 0; i < NUM_SPIN_LOCKS; ++i) {
		for (int j = 0; j < NUM_CORES; ++j) {
			counter_local[j][i] = 0;
		}
		counter_shared[i] = 0;
	}
	__mem_fence_release();
}

bool check_counter_sums(void) {
	__mem_fence_acquire();
	bool all_ok = true;
	uint full_sum = 0;
	for (int i = 0; i < NUM_SPIN_LOCKS; ++i) {
		uint per_lock_sum = 0;
		for (int j = 0; j < NUM_CORES; ++j) {
			per_lock_sum += counter_local[j][i];
			if (counter_local[j][i] > ITERATIONS) {
				printf("Impossible local counter value %d on core %d: %08x (max %08x)\n",
					i, j, counter_local[j][i], ITERATIONS);
				all_ok = false;
			}
		}
		if (per_lock_sum != counter_shared[i]) {
			printf("Failed sum check for lock %d: expected %08x, actual %08x\n",
				i, per_lock_sum, counter_shared[i]
			);
			all_ok = false;
		} 
		if (counter_shared[i] > ITERATIONS * NUM_CORES) {
			printf("Impossible shared counter value %d: %08x (max %08x)\n",
				i, counter_shared[i], ITERATIONS * NUM_CORES);
			all_ok = false;
		}
		full_sum += per_lock_sum;
	}
	if (full_sum != ITERATIONS * NUM_CORES) {
		printf("Incorrect counter total: expected %08x, got %08x\n",
			ITERATIONS, full_sum);
		all_ok = false;
	}
	return all_ok;
}

void counter_test_per_core(uint lock_index_mask) {
	// Each lock has a global counter. Repeatedly, randomly select a lock and
	// write to its counter while holding the lock. Also increment a per-core
	// counter for that lock, so we can check at the end that the per-core
	// values add up.
	xrand_state_t state = XRAND_DEFAULT_INIT;
	uint core_num = get_core_num();
	for (uint i = 0; i < core_num; ++i) {
		xrand_jump(&state);
	}
	for (uint i = 0; i < ITERATIONS; ++i) {
		uint lock_index = xrand_next(&state) & lock_index_mask;
		spin_lock_t *lock = spin_lock_instance(lock_index);
		uint32_t flags = spin_lock_blocking(lock);
		counter_shared[lock_index]++;
		spin_unlock(lock, flags);
		counter_local[core_num][lock_index]++;
		busy_wait_at_least_cycles(xrand_next(&state) & 0xffu);
	}
}

void counter_try_test_per_core(uint lock_index_mask) {
	// Same as counter_test but use the try_lock variant -- worth testing as
	// it may be a different asm implementation altogether.
	xrand_state_t state = XRAND_DEFAULT_INIT;
	uint core_num = get_core_num();
	for (uint i = 0; i < core_num; ++i) {
		xrand_jump(&state);
	}
	for (uint i = 0; i < ITERATIONS; ++i) {
		uint lock_index = xrand_next(&state) & lock_index_mask;
		spin_lock_t *lock = spin_lock_instance(lock_index);
		// Assume this test runs without IRQs active
		while (!spin_try_lock_unsafe(lock))
			;
		counter_shared[lock_index]++;
		spin_unlock_unsafe(lock);
		counter_local[core_num][lock_index]++;
		busy_wait_at_least_cycles(xrand_next(&state) & 0xffu);
	}
}


// Test with successively fewer locks to increase contention
void counter_test1(void) {
	counter_test_per_core(NUM_SPIN_LOCKS - 1);
}

void counter_test2(void) {
	counter_test_per_core((NUM_SPIN_LOCKS - 1) >> 1);
}

void counter_test3(void) {
	counter_test_per_core((NUM_SPIN_LOCKS - 1) >> 2);
}

void counter_test4(void) {
	counter_test_per_core((NUM_SPIN_LOCKS - 1) >> 3);
}

void counter_test5(void) {
	counter_test_per_core((NUM_SPIN_LOCKS - 1) >> 4);
}

void counter_try_test1(void) {
	counter_try_test_per_core(NUM_SPIN_LOCKS - 1);
}

void counter_try_test2(void) {
	counter_try_test_per_core((NUM_SPIN_LOCKS - 1) >> 4);
}

void counter_test_with_irqs(void) {

}


static const test_t tests[] = {
	{
		"counter test, all locks\n",
		prepare_clear_counters,
		counter_test1,
		check_counter_sums
	},
	{
		"counter test, half of locks\n",
		prepare_clear_counters,
		counter_test2,
		check_counter_sums
	},
	{
		"counter test, 1/4 of locks\n",
		prepare_clear_counters,
		counter_test3,
		check_counter_sums
	},
	{
		"counter test, 1/8 of locks\n",
		prepare_clear_counters,
		counter_test4,
		check_counter_sums
	},
	{
		"counter test, 1/16 of locks\n",
		prepare_clear_counters,
		counter_test5,
		check_counter_sums
	},
	{
		"counter test with try_lock, all locks\n",
		prepare_clear_counters,
		counter_try_test1,
		check_counter_sums
	},
	{
		"counter test with try_lock, 1/16 of locks\n",
		prepare_clear_counters,
		counter_try_test2,
		check_counter_sums
	},
};

// ---------------------------------------------------------------------------
#if PICO_USE_SW_SPIN_LOCKS && PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT
#define SEV_PROBE_EXPECT_OWN_EVENT 1
#else
#define SEV_PROBE_EXPECT_OWN_EVENT 0
#endif

// Verify the PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT setting
//
// Clear the event flag, do a lock/unlock pair, then __wfe() and see whether it returned of its
// own accord or only once core 1's backstop arrived. A control pass with the lock/unlock
// omitted runs first and must block, otherwise ambient interrupt activity is doing the waking
// and the measurement means nothing -- reported as inconclusive rather than as a failure.
//
// Only core 0 is probed: the event register is per core, but both cores run the same spin lock
// code against the same fabric.
// ---------------------------------------------------------------------------

// long enough that core 0 is certainly parked in its __wfe() before this expires
static const uint SEV_PROBE_BACKSTOP_CYCLES = 200000;
static const uint SEV_PROBE_ROUNDS = 5;
#define SEV_PROBE_LOCK_NUM 0

static volatile bool sev_probe_backstop_fired;

// runs on core 1
void sev_probe_backstop(void) {
	busy_wait_at_least_cycles(SEV_PROBE_BACKSTOP_CYCLES);
	sev_probe_backstop_fired = true;
	__mem_fence_release();
	__sev();
}

// returns true if the __wfe() blocked until the backstop, false if it returned earlier
static bool sev_probe_wfe_blocked(bool with_lock_unlock) {
	sev_probe_backstop_fired = false;
	__mem_fence_release();
	// this SEVs, so it must happen before we clear the event flag below
	multicore_fifo_push_blocking((uintptr_t)sev_probe_backstop);

	// clear our event flag: the __sev() sets it and the __wfe() consumes it without blocking
	__sev();
	__wfe();

	if (with_lock_unlock) {
		spin_lock_t *lock = spin_lock_instance(SEV_PROBE_LOCK_NUM);
		uint32_t save = spin_lock_blocking(lock);
		spin_unlock(lock, save);
	}
	__wfe();
	__mem_fence_acquire();
	bool blocked = sev_probe_backstop_fired;

	// resynchronise with core 1, which may still be spinning
	(void)multicore_fifo_pop_blocking();
	return blocked;
}

static bool sev_probe_run(void) {
	spin_lock_init(SEV_PROBE_LOCK_NUM);
	uint control_blocked = 0;
	uint probe_blocked = 0;
	for (uint i = 0; i < SEV_PROBE_ROUNDS; ++i) {
		printf("  round %u: control...\n", i);
		control_blocked += sev_probe_wfe_blocked(false);
		printf("  round %u: probe...\n", i);
		probe_blocked += sev_probe_wfe_blocked(true);
	}
	printf("control (no lock/unlock): blocked %u/%u\n", control_blocked, SEV_PROBE_ROUNDS);
	printf("probe   (lock + unlock):  blocked %u/%u\n", probe_blocked, SEV_PROBE_ROUNDS);

	if (control_blocked != SEV_PROBE_ROUNDS) {
		printf("INCONCLUSIVE: a bare __wfe() did not always block, so something else is\n"
			"setting this core's event flag; cannot attribute anything to the spin lock.\n");
		// not a failure of the code under test
		return true;
	}
	bool observed;
	if (probe_blocked == 0) {
		observed = true;
	} else if (probe_blocked == SEV_PROBE_ROUNDS) {
		observed = false;
	} else {
		printf("Failed: spin lock unlock set the event flag inconsistently (%u/%u blocked)\n",
			probe_blocked, SEV_PROBE_ROUNDS);
		return false;
	}
	printf("observed: spin lock lock/unlock %s the calling core's event flag\n",
		observed ? "DOES set" : "does NOT set");
	if (observed != (bool)SEV_PROBE_EXPECT_OWN_EVENT) {
		printf("Failed: PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT is %d but hardware says %d.\n"
			"The lock_core wait/notify primitives in pico/lock_core.h rely on this.\n",
			SEV_PROBE_EXPECT_OWN_EVENT, observed);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Remote spin lock traffic probe
//
// The probe above asks whether THIS core's own lock/unlock sets its own event. It cannot see
// the other question the lock_core wait primitives depend on: whether ANOTHER core's ordinary
// spin lock traffic sets this core's event. If it does, a core sitting in the usual
// "check condition, WFE" loop is woken by unrelated lock activity elsewhere and cannot stay
// asleep - and two cores both in wait loops can hold each other awake indefinitely.
//
// Measured the same way: core 1 does N lock/unlock pairs on a DIFFERENT lock (so this is about
// remote traffic, not contention), then flags and SEVs as a backstop. If core 0's __wfe()
// returns while the flag is still clear, remote traffic woke it.
//
// Note core 0 takes and releases a lock of its own before waiting, so this measures the pattern
// the wait primitives actually use. What comes out is therefore a property of pattern AND
// hardware together: a negative result means only that remote traffic cannot reach a core that
// waits this way, not that the part has no cross-core wake path. Where a core's own exclusive
// store clears its own reservation, it holds none by the time it waits, and the path - which is
// still there - has nothing to act on.
//
// Reported, not asserted: what the right expectation is per platform is exactly what this is
// here to establish.
// ---------------------------------------------------------------------------

#ifndef SEV_PROBE_SKIP_REMOTE
#define SEV_PROBE_SKIP_REMOTE 0
#endif

#define SEV_PROBE_REMOTE_LOCK_NUM 1
static const uint SEV_PROBE_REMOTE_PAIRS = 2000;

// runs on core 1
void sev_probe_remote_traffic(void) {
	// let core 0 reach its __wfe() first
	busy_wait_at_least_cycles(SEV_PROBE_BACKSTOP_CYCLES / 4);
	spin_lock_t *lock = spin_lock_instance(SEV_PROBE_REMOTE_LOCK_NUM);
	for (uint i = 0; i < SEV_PROBE_REMOTE_PAIRS; i++) {
		uint32_t save = spin_lock_blocking(lock);
		spin_unlock(lock, save);
	}
	sev_probe_backstop_fired = true;
	__mem_fence_release();
	__sev();
}

// runs on core 1: the control, same shape with no lock traffic
void sev_probe_remote_quiet(void) {
	busy_wait_at_least_cycles(SEV_PROBE_BACKSTOP_CYCLES / 4);
	busy_wait_at_least_cycles(SEV_PROBE_BACKSTOP_CYCLES);
	sev_probe_backstop_fired = true;
	__mem_fence_release();
	__sev();
}

static bool sev_probe_remote_blocked(bool with_traffic) {
	sev_probe_backstop_fired = false;
	__mem_fence_release();
	multicore_fifo_push_blocking((uintptr_t)(with_traffic ? sev_probe_remote_traffic
	                                                     : sev_probe_remote_quiet));
	// Take and release a lock of our own before waiting, exactly as every lock_core wait does.
	// This matters: a cross-PE store can only clear a reservation we actually hold, and whether
	// our own exclusive store left one behind is the implementation-defined part.
	spin_lock_t *own = spin_lock_instance(SEV_PROBE_LOCK_NUM);
	uint32_t save = spin_lock_blocking(own);
	spin_unlock(own, save);

	// clear our event flag, including any self-ping from the pair above
	__sev();
	__wfe();

	__wfe();
	__mem_fence_acquire();
	bool blocked = sev_probe_backstop_fired;
	(void)multicore_fifo_pop_blocking();
	return blocked;
}

static bool sev_probe_remote_run(void) {
	spin_lock_init(SEV_PROBE_LOCK_NUM);
	spin_lock_init(SEV_PROBE_REMOTE_LOCK_NUM);
	uint control_blocked = 0, traffic_blocked = 0;
	for (uint i = 0; i < SEV_PROBE_ROUNDS; ++i) {
		printf("  round %u: control...\n", i);
		control_blocked += sev_probe_remote_blocked(false);
		printf("  round %u: traffic...\n", i);
		traffic_blocked += sev_probe_remote_blocked(true);
	}
	printf("control (core 1 quiet):        blocked %u/%u\n", control_blocked, SEV_PROBE_ROUNDS);
	printf("probe   (core 1 lock traffic): blocked %u/%u\n", traffic_blocked, SEV_PROBE_ROUNDS);
	if (control_blocked != SEV_PROBE_ROUNDS) {
		printf("INCONCLUSIVE: a bare __wfe() did not always block even with core 1 quiet.\n");
		return true;
	}
	const bool woken = traffic_blocked != SEV_PROBE_ROUNDS;
	printf("observed: after using a spin lock itself, this core %s woken by another core's\n"
		"  spin lock traffic, so a lock_core wait loop %s stay asleep while another core\n"
		"  uses spin locks\n",
		woken ? "IS" : "is NOT", woken ? "CANNOT" : "can");
	printf("  note this is the composite of the access pattern and the hardware, not a\n"
		"  statement about the part: a cross-core wake path may exist and simply not be\n"
		"  reachable this way, e.g. where a core's own exclusive store has already cleared\n"
		"  its reservation by the time it waits\n");
	return true;
}

void core1_main(void) {
	while (true) {
		void (*f)() = (void(*)())multicore_fifo_pop_blocking();
		f();
		multicore_fifo_push_blocking(0);
	}
}

int main() {
	stdio_init_all();
	printf("Hello world\n");
	multicore_launch_core1(core1_main);
	uint failed = 0;

	printf(">>> Starting test: PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT probe (core 0)\n");
	spin_locks_reset();
	if (sev_probe_run()) {
		printf("OK.\n");
	} else {
		printf("Failed.\n");
		++failed;
	}
	printf(">>> Finished test: PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT probe\n");

#if !SEV_PROBE_SKIP_REMOTE
	printf(">>> Starting test: remote spin lock traffic probe (core 0)\n");
	spin_locks_reset();
	if (sev_probe_remote_run()) {
		printf("OK.\n");
	} else {
		printf("Failed.\n");
		++failed;
	}
	printf(">>> Finished test: remote spin lock traffic probe\n");
#endif

	for (int i = 0; i < count_of(tests); ++i) {
		const test_t *t = &tests[i];
		printf(">>> Starting test: %s\n", t->name);
		spin_locks_reset();
		t->prepare();
		multicore_fifo_push_blocking((uintptr_t)t->run_per_core);
		t->run_per_core();
		(void)multicore_fifo_pop_blocking();
		printf(">>> Finished test: %s\n", t->name);
		if (t->check()) {
			printf("OK.\n");
		} else {
			printf("Failed.\n");
			++failed;
		}
	}
	if (failed == 0u) {
		printf("PASSED\n");
		return 0;
	} else {
		printf("%u tests failed. Review log for details.\n", failed);
		return -1;
	}
}
