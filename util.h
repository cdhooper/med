#ifndef _UTIL
#define _UTIL

#ifndef ADDR8
#define ADDR8(x)    ((uint8_t *)  ((uintptr_t)(x)))
#define ADDR16(x)   ((uint16_t *) ((uintptr_t)(x)))
#define ADDR32(x)   ((uint32_t *) ((uintptr_t)(x)))
#define ADDR64(x)   ((uint64_t *) ((uintptr_t)(x)))
#endif

#ifndef VADDR8
#define VADDR8(x)    ((volatile uint8_t *)  ((uintptr_t)(x)))
#define VADDR16(x)   ((volatile uint16_t *) ((uintptr_t)(x)))
#define VADDR32(x)   ((volatile uint32_t *) ((uintptr_t)(x)))
#define VADDR64(x)   ((volatile uint64_t *) ((uintptr_t)(x)))
#endif

#define BIT(x) (1U << (x))
#define ARRAY_SIZE(x) ((sizeof (x) / sizeof ((x)[0])))

#ifndef TRUE
#define TRUE            (1)
#endif

#ifndef FALSE
#define FALSE           (0)
#endif

#endif
