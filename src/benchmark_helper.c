#include "defaults.h"

#include "elements/buffered_element.h"
#include "elements/checksum.h"
#include "elements/drop.h"
#include "elements/identity.h"
#include "elements/measurement.h"
#include "elements/routing.h"

#include "benchmark_helper.h"

inline struct element_t *el_identity_create(packet_index_t size) {
    return (struct element_t *)buffered_element_create(
            (struct element_t*)identity_create(), size);
}

inline struct element_t *el_drop_create(packet_index_t size) {
    return (struct element_t *)buffered_element_create(
            (struct element_t*)drop_create(), size);
}

inline struct element_t *el_checksum_create(packet_index_t size) {
    return (struct element_t *)buffered_element_create(
            (struct element_t*)checksum_create(), size);
}

inline struct element_t *el_routing_create(packet_index_t size) {
    struct routing_t *router = routing_create();
    ipv4_prefix_t prefix = { .ipv4=0, .mask=0 };

    routing_entry_add(router, &prefix, 0);
    return (struct element_t *)buffered_element_create(
            (struct element_t*)router, size);
}

inline struct element_t *el_routing_create_with_file(
        packet_index_t size, char const *fname) {
    struct routing_t *router = routing_create();
    routing_file_load(router, fname);

    return (struct element_t *)buffered_element_create(
            (struct element_t*)router, size);
}

inline struct element_t *el_measurement_create_with_size(
        packet_index_t size, size_t tbl_size) {
    struct measurement_t *monitor = measurement_create();
    measurement_resize(monitor, tbl_size);

    return (struct element_t *)buffered_element_create(
            (struct element_t*)monitor, size);
}

