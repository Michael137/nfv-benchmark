// This file was automatically generated at 04/11/2019 06:19:26
#include "memory.h"
#include "element.h"
#include "packets.h"
#include "util.h"
#include "checksum.h"
#include "rte_prefetch.h"
#include "elements/test.h"
#define MPF_SIZE 32
#define MPF_SIZE_HALF ( MPF_SIZE >> 1 )
#define MPF_TBL_SIZE ( 1 << 24 )
struct test_t* test_create(void)
{
struct test_t* test = (struct test_t*) mem_alloc(sizeof(struct test_t));
memset(test, 0, sizeof(struct test_t));
test->element.process    = test_process   ;
test->element.release    = test_release   ;
test->element.report     = test_report    ;
test->element.connect    = element_connect   ;
test->element.disconnect = element_disconnect;
test->element.hop_at     = element_hop_at    ;
// Set up measurement pool
test->tbl_size = ge_pow2_64(MPF_TBL_SIZE);
test->tbl = mem_alloc(sizeof(uint32_t) * test->tbl_size);
memset( test->tbl, 0, sizeof( uint32_t ) * test->tbl_size );
// Setup checksum: nop
return test;
}
void test_process(struct element_t* ele, struct packet_t** pkts, packet_index_t size)
{
struct test_t* self = (struct test_t*) ele;
struct packet_t* pkt;
struct packet_t* p[MPF_SIZE];
uint32_t hashes[MPF_SIZE_HALF];
struct __attribute__( ( packed ) )
{
	ipv4_t src;
	ipv4_t dst;
	union {
		struct __attribute__( ( packed ) )
		{
			uint16_t src;
			uint16_t dst;
		} ports;
		uint32_t srcdst_port;
	};
} ip;
for( int j = 0; j < MPF_SIZE_HALF; ++j )
{
	    p[j] = pkts[j];
	    rte_prefetch0( p[j]->hdr + 26 );
}
uint32_t out = 0;
int i = 0;
	for( int j = 0; j < size; ++j )
	{
		pkt            = p[j];
		ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );
		ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );
		ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );
		out = util_hash_ret( &ip, sizeof( ip ) );
		out &= ( (MPF_TBL_SIZE)-1 );
		hashes[j] = out;
		rte_prefetch0(self->tbl + out);
		struct _routing_tbl_entry_t* ent
		    = routing_entry_find( self, ip.dst );
		if( ent )
		{
			self->port_count += ent->port;
			ent->count++;
             }
		self->checksum_count += checksum( pkt->hdr, pkt->size );
             for (int j = 0; j < MPF_SIZE_HALF; ++j) {
                 self->tbl[hashes[j]]++;
                 p[j] = p[j + MPF_SIZE_HALF];
             }
 }
     for( int j = i; j < size; ++j )
     {
     	self->tbl[hashes[j - i]]++;
     }
     element_dispatch( ele, 0, pkts, size );
}
void test_release(struct element_t* ele)
{
struct test_t* self = (struct test_t*)ele;
uint64_t total = 0;
for(size_t i = 0; i < self->tbl_size; ++i)
{
total += self->tbl[i];
}
printf( "Total number of packets processed: %lu\n", total );
if( self->tbl )
{
mem_release( self->tbl );
}
mem_release( ele );
}
void test_report( __attribute__( ( unused ) ) struct element_t* _ )
{
}
