#ifndef _ELEMENTS_MEASUREMENT_H_
#define _ELEMENTS_MEASUREMENT_H_

#include "element.h"

#undef MEASUREMENT_POPULAR

struct packet_t;

struct measurement_t {
    struct element_t element;
    size_t tbl_size;
    uint32_t _tmp[MEASUREMENT_BUFFER_SIZE];

#ifdef MEASUREMENT_POPULAR
    uint32_t pop_count;
    uint32_t pop_sig[3];
#endif

    uint32_t *tbl;
};

struct measurement_t *measurement_create(void);
void measurement_resize(struct measurement_t *, size_t tbl_size);
void measurement_process_no_prefetching(struct element_t *, struct packet_t **, packet_index_t);
void measurement_process_prefetching(struct element_t *, struct packet_t **, packet_index_t);
void measurement_release(struct element_t *);
void measurement_report(struct element_t *);

#endif // _ELEMENTS_MEASUREMENT_H_
