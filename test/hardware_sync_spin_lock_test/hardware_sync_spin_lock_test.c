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
		printf("All tests passed.\n");
		return 0;
	} else {
		printf("%u tests failed. Review log for details.\n", failed);
		return -1;
	}
}
