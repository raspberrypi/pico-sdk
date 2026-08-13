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
// PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV probe
//
// The RP2350 lock_core wait/notify primitives (see PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND in
// pico/lock_core.h) are built on taking and releasing a software spin lock setting the calling
// core's own event flag, so that a following __wfe() returns immediately and a second one is
// needed to actually wait. PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV asserts that at compile time, and
// nothing checks it against the hardware. Worth checking, because the Arm and RISC-V software
// implementations use different instructions -- ldaexb/strexb, which takes a reservation,
// against amoor.w.aq, which does not -- so it does not follow that both engage whatever
// generates the event.
//
// Clear the event flag, do a lock/unlock pair, then __wfe() and see whether it returned of its
// own accord or only once core 1's backstop arrived. A control pass with the lock/unlock
// omitted runs first and must block, otherwise ambient interrupt activity is doing the waking
// and the measurement means nothing -- reported as inconclusive rather than as a failure.
//
// Only core 0 is probed: the event register is per core, but both cores run the same spin lock
// code against the same fabric.
// ---------------------------------------------------------------------------

#ifndef PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV
#define PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV 0
#endif

// long enough that core 0 is certainly parked in its __wfe() before this expires
static const uint SEV_PROBE_BACKSTOP_CYCLES = 2000000;
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
		control_blocked += sev_probe_wfe_blocked(false);
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
	if (observed != (bool)PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV) {
		printf("Failed: PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV is %d but hardware says %d.\n"
			"The lock_core wait/notify primitives in pico/lock_core.h rely on this.\n",
			PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV, observed);
		return false;
	}
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

	printf(">>> Starting test: PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV probe (core 0)\n");
	spin_locks_reset();
	if (sev_probe_run()) {
		printf("OK.\n");
	} else {
		printf("Failed.\n");
		++failed;
	}
	printf(">>> Finished test: PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV probe\n");

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
