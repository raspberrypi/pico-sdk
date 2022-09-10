#include "pico.h"

#include <stdlib.h>
#include <string.h>

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112)
#include <malloc.h>
#define aligned_alloc memalign
#endif

// From emutls.c:
// 'For every TLS variable xyz, there is one __emutls_control variable named __emutls_v.xyz. If xyz has
// non-zero initial value, __emutls_v.xyz's "value" will point to __emutls_t.xyz, which has the initial value.'
//
// The linker script groups all the __emutls_v.xyz variables into a single array and provides symbols
// __emutls_array_start and __emutls_array_end, which can be used to iterate over the array. This allows
// the storage for each core's thread local variables to be pre-allocated and pre-initialized, which leaves
// minimal work for __wrap___emutls_get_address.
//
// This array is available to other TLS implementations too, such a TLS implementation for an RTOS.

// Same layout as libgcc __emutls_object. Unfortunately, __emutls_object doesn't appear in any header files.
typedef struct {
	uint size;
	uint align;
  union {
      struct {
          uint offset;
          void *template;
      } s;
      void* lookup[NUM_CORES];
  } u;
} tls_object;

extern tls_object __emutls_array_start;
extern tls_object __emutls_array_end;

static char* tls_storage[NUM_CORES];

void tls_init(void);
void* __wrap___emutls_get_address(tls_object*);

// Must be called after it is safe to call memcpy.
void tls_init(void) {
    // Three passes:
    // 1) Calculate the offset of each thread local variable and the total storage to be allocated for each thread.
    uint offset = 0;
    uint max_align = 1;
    for (tls_object* object = &__emutls_array_start; object < &__emutls_array_end; ++object) {
        assert((object->align & (object->align - 1)) == 0);

        if (object->align > max_align) {
            max_align = object->align;
        }

        offset = (offset + object->align - 1) & ~(object->align - 1);
        object->u.s.offset = offset;
        offset += object->size;
    }

    if (offset == 0) {
        return;
    }

    // 2) Allocate storage for each thread and initialize the thread local variables to their initial value.
    for (uint i = 0; i < NUM_CORES; ++i) {
        // TODO: tls_init is invoked before pico_malloc's auto-initialized mutex has been initialized.
        // However, aligned_alloc and memalign are not wrapped by pico_malloc so don't acquire or release
        // the mutex so this works, though not for a satisfying reason.
        //
        // What I would like to do here, since malloc and friends ought not to be called at this point in
        // initialization, is decrement the heap limit buy the TLS storage size. At time of writing, the
        // heap limit is &__StackLimit, i.e. static. It could be dynamic though.
        char* storage = tls_storage[i] = (char*) aligned_alloc(max_align, offset);

        for (tls_object* object = &__emutls_array_start; object < &__emutls_array_end; ++object) {
            if (object->u.s.template) {
                memcpy(storage + object->u.s.offset, object->u.s.template, object->size);
            } else {
                memset(storage + object->u.s.offset, 0, object->size);
            }
        }
    }

    // 3) Repurpose the tls_objects so each contains a lookup table mapping from core index to pointer to
    //    thread local variable.
    for (tls_object* object = &__emutls_array_start; object < &__emutls_array_end; ++object) {
        uint offset = object->u.s.offset;
        for (uint i = 0; i < NUM_CORES; ++i) {
            object->u.lookup[i] = tls_storage[i] + offset;
        }
    }
}

void* __wrap___emutls_get_address(tls_object* object) {
    return object->u.lookup[get_core_num()];
}
