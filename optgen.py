# Parse a spec that describes optimizations to be generated (by default "gen.spec.template")
# Then generate optimizations file

from datetime import datetime

def parse(spec_path='gen.spec.template'):
    sections = {}
    headers = []

    with open(spec_path) as fp:
        line = fp.readline()
        section_name = ''
        while line:
            line = line.strip()
            if line.startswith('[') and line.endswith(']'):
                section_name = line[1:len(line) - 1]
                sections[section_name] = []
            else:
                if len(line):
                    sections[section_name].append(line)

            line = fp.readline()

    if 'opt-name' not in sections:
        print('Failed to parse {}. No "opt-name" section found.'.format(spec_path))
        exit(1)

    return (sections,headers)

# headers: list of header names
# f: filehandle
def gen_headers(headers, f):
    print('// This file was automatically generated at {}'.format(datetime.now().strftime("%d/%m/%Y %H:%M:%S")),file=f)
    print('#include "memory.h"',file=f)
    print('#include "element.h"',file=f)
    print('#include "packets.h"',file=f)
    print('#include "util.h"',file=f)
    print('#include "checksum.h"',file=f)
    print('#include "rte_prefetch.h"',file=f)
    for h in headers:
        print('#include "{}"'.format(h),file=f)

def gen_defs(spec):
    pass

# Input: parsed spec file
def gen_include_file(spec,f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)

    print('// This file was automatically generated at {}'.format(datetime.now().strftime("%d/%m/%Y %H:%M:%S")),file=f)

    # Include guard
    print('#ifndef {}_IN_H'.format(opt_name.upper()),file=f)
    print('#define {}_IN_H'.format(opt_name.upper()),file=f)

    print('#include "elements/routing.h"',file=f)
    print('#include "element.h"',file=f)

    # Forward declare packet type
    print('struct packet_t;',file=f)

    # Declaration of optimization specific type
    print('struct {}_t {{'.format(opt_name),file=f)

    # Routing NF
    print('/* This block should match routing_t\'s structure */',file=f)
    print('struct element_t element;',file=f)
    print('uint64_t _tmp_2;',file=f)
    print('struct _routing_tbl_256_t fst_tbl;',file=f)
    print('/* until here */',file=f)

    # Measurement NF
    print('// Measurement',file=f)
    print('size_t tbl_size;',file=f)
    print('uint32_t _tmp[MEASUREMENT_BUFFER_SIZE];',file=f)
    print('uint32_t* tbl;',file=f)

    print('// Checksum',file=f)
    print('uint64_t checksum_count;',file=f)
    print('uint64_t port_count;',file=f)

    print('// Misc.',file=f)
    print('uint32_t a, b, c;',file=f)
    print('uint64_t slowpass_count;',file=f)

    # Close declaration
    print('};',file=f)

    # Prototypes for functions to generate
    print( '{}* {}_create(uint32_t, uint32_t, uint32_t);'.format(opt_type, opt_name),file=f)
    print( 'void {}_process(struct element_t*, struct packet_t**, packet_index_t);'.format(opt_name),file=f)
    print( 'void {}_release(struct element_t*);'.format(opt_name),file=f)
    print( 'void {}_report(struct element_t*);'.format(opt_name),file=f)

    # Close include guard
    print('#endif // {}_IN_H'.format(opt_name.upper()),file=f)

def gen_create_fn(spec, f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)
    fn_name = '{}_create'.format(opt_name)

    print('#include "elements/{}.h"'.format(opt_name),file=f)

    print('#define MPF_SIZE 16',file=f)
    print('#define MPF_SIZE_HALF ( MPF_SIZE >> 1 )',file=f)
    print('#define MPF_TBL_SIZE ( 1 << 18 )',file=f)

    print('{}* {}(uint32_t src, uint32_t dst, uint32_t srcdst_port)'.format(opt_type, fn_name),file=f)
    print("{",file=f)
    print('{}* {} = ({}*) mem_alloc(sizeof({}));'.format(opt_type, opt_name, opt_type,opt_type),file=f)
    print('memset({}, 0, sizeof({}));'.format(opt_name,opt_type),file=f)

    print('{}->element.process    = {}_process   ;'.format(opt_name,opt_name),file=f)
    print('{}->element.release    = {}_release   ;'.format(opt_name,opt_name),file=f)
    print('{}->element.report     = {}_report    ;'.format(opt_name,opt_name),file=f)
    print('{}->element.connect    = element_connect   ;'.format(opt_name),file=f)
    print('{}->element.disconnect = element_disconnect;'.format(opt_name),file=f)
    print('{}->element.hop_at     = element_hop_at    ;'.format(opt_name),file=f)
    print('// Set up measurement pool',file=f)
    print('{}->tbl_size = ge_pow2_64(MPF_TBL_SIZE);'.format(opt_name),file=f)
    print('{}->tbl = mem_alloc(sizeof(uint32_t) * {}->tbl_size);'.format(opt_name,opt_name),file=f)
    print('memset( {}->tbl, 0, sizeof( uint32_t ) * {}->tbl_size );'.format(opt_name,opt_name),file=f)
    print('{}->a = src;'.format(opt_name),file=f)
    print('{}->b = dst;'.format(opt_name),file=f)
    print('{}->c = srcdst_port;'.format(opt_name),file=f)
    print('{}->_tmp_2 = 0;'.format(opt_name),file=f)
    print('{}->slowpass_count = 0;'.format(opt_name),file=f)
    print('return ' + opt_name + ';',file=f)
    print("}",file=f)

def gen_release_fn(spec, f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)
    fn_name = '{}_release'.format(opt_name)

    print('void {}(struct element_t* ele)'.format(fn_name),file=f)
    print('{',file=f)

    print('{}* self = ({}*)ele;'.format(opt_type,opt_type),file=f)
    print('uint64_t total = 0;'.format(opt_type),file=f)
    print('for(size_t i = 0; i < self->tbl_size; ++i)'.format(opt_type),file=f)
    print('{',file=f)
    print('total += self->tbl[i];', file=f)
    print('}',file=f)

    print('printf( "Total number of packets processed: %lu\\n", total );',file=f)
    print('printf( "Total number of fastpass packets: %lu\\n", self->_tmp_2 );',file=f)
    print('printf( "Total number of slowpass packets: %lu\\n", self->slowpass_count );',file=f)

    print('if( self->tbl )',file=f)
    print('{',file=f)
    print('mem_release( self->tbl );',file=f)
    print('}',file=f)
    print('mem_release( ele );',file=f)

    print('}',file=f)

def gen_report_fn(spec,f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    fn_name = '{}_report'.format(opt_name)
    print('void {}( __attribute__( ( unused ) ) struct element_t* _ )'.format(fn_name),file=f)
    print('{',file=f)
    print('}',file=f)

def gen_process_fn(parsed,f):
    opt_name = parsed['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)
    fn_name = '{}_process'.format(opt_name)

    print('void {}(struct element_t* ele, struct packet_t** pkts, packet_index_t size)'.format(fn_name),file=f)
    print('{',file=f)
    print('{}* self = ({}*) ele;'.format(opt_type,opt_type),file=f)

    # Prefetching pattern
    print('struct packet_t* pkt;',file=f);
    print('struct packet_t* p[MPF_SIZE];',file=f);
    print('uint32_t hashes[MPF_SIZE_HALF];',file=f);
    print('struct __attribute__( ( packed ) )',file=f)
    print('{',file=f)
    print('	ipv4_t src;',file=f)
    print('	ipv4_t dst;',file=f)
    print('	union {',file=f)
    print('		struct __attribute__( ( packed ) )',file=f)
    print('		{',file=f)
    print('			uint16_t src;',file=f)
    print('			uint16_t dst;',file=f)
    print('		} ports;',file=f)
    print('		uint32_t srcdst_port;',file=f)
    print('	};',file=f)
    print('} ip;',file=f)
    print('for( int j = 0; j < MPF_SIZE_HALF; ++j )',file=f)
    print('{',file=f)
    print('	p[j] = pkts[j];',file=f)
    print('	rte_prefetch0( p[j]->hdr + 26 );',file=f)
    print('}',file=f)

    # Main processing loop
    print('uint32_t src, dst, srcdst_port;',file=f)
    print('uint32_t fastpass_count = 0;',file=f)
    print('src = self->a;',file=f)
    print('dst = self->b;',file=f)
    print('srcdst_port = self->c;',file=f)
    print('struct packet_t* slow[MPF_SIZE];', file=f)
    print('uint32_t out = 0;', file=f)
    print('int i = MPF_SIZE_HALF;', file=f)
    print('for( ; i < size - MPF_SIZE_HALF; i += MPF_SIZE_HALF )', file=f)
    print('{', file=f)
    print('	for( int j = 0; j < MPF_SIZE_HALF && i + j < size; ++j )', file=f)
    print('	{', file=f)
    print('		p[j + MPF_SIZE_HALF] = pkts[i + j];', file=f)

    # Prefetching
    print('		// Prefetch the next set of packets', file=f)
    print('		rte_prefetch0( p[j + MPF_SIZE_HALF]->hdr + 26 );', file=f)
    print('	}', file=f)

    # Loop splitting
    print('	uint32_t sidx = 0;',file=f)
    print('	for( int j = 0; j < MPF_SIZE_HALF; ++j )',file=f)
    print('	{',file=f)
    print('		pkt            = p[j];',file=f)
    print('		ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );',file=f)
    print('		ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );',file=f)
    print('		ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );',file=f)
    print('		slow[sidx++] = pkt;',file=f)
    print('		uint32_t is_fast',file=f)
    print('		    = ( ip.src == src ) && ( ip.dst == dst ) && ( ip.srcdst_port == srcdst_port );',file=f)
    print('		sidx -= ( is_fast );',file=f)
    print('		fastpass_count += is_fast;',file=f)
    print('		self->checksum_count += checksum( pkt->hdr, pkt->size );',file=f)
    print('	}',file=f)

    print('	for( uint32_t j = 0; j < sidx; ++j )',file=f)
    print('	{',file=f)
    print('		pkt            = slow[j];',file=f)
    print('		ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );',file=f)
    print('		ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );',file=f)
    print('		ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );',file=f)
    print('		out = util_hash_ret( &ip, sizeof( ip ) );',file=f)
    print('		out &= ( (MPF_TBL_SIZE)-1 );',file=f)
    print('		hashes[j] = out;',file=f)
    print('		struct _routing_tbl_entry_t* ent',file=f)
    print('		    = routing_entry_find( self, ip.dst );',file=f)
    print('		if( ent )',file=f)
    print('		{',file=f)
    print('			self->port_count += ent->port;',file=f)
    print('			ent->count++;',file=f)
    print('		}',file=f)
    print('	}',file=f)

    print('	for( uint32_t j = 0; j < sidx; ++j )',file=f)
    print('	{',file=f)
    print('		self->tbl[hashes[j]]++;',file=f)
    print('	}',file=f)

    # Loop splitting
    print('	self->slowpass_count += sidx;',file=f)
    print('	for( int j = 0; j < MPF_SIZE_HALF; ++j )',file=f)
    print('	{',file=f)
    print('		p[j] = p[j + MPF_SIZE_HALF];',file=f)
    print('	}',file=f)
    print('}',file=f)

    # Checksum
    print('i -= MPF_SIZE_HALF;',file=f)
    print('uint32_t sidx = 0;',file=f)
    print('for( int j = i; j < size; ++j )',file=f)
    print('{',file=f)
    print('	pkt            = pkts[j];',file=f)
    print('	ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );',file=f)
    print('	ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );',file=f)
    print('	ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );',file=f)
    print('	slow[sidx++] = pkt;',file=f)
    print('	uint32_t is_fast',file=f)
    print('	    = ( ip.src == src ) && ( ip.dst == dst ) && ( ip.srcdst_port == srcdst_port );',file=f)
    print('	sidx -= ( is_fast );',file=f)
    print('	fastpass_count += is_fast;',file=f)
    print('	self->checksum_count += checksum( pkt->hdr, pkt->size );',file=f)
    print('}',file=f)

    # Routing
    print('for( uint32_t j = 0; j < sidx; ++j )',file=f)
    print('{',file=f)
    print('	pkt            = slow[j];',file=f)
    print('	ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );',file=f)
    print('	ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );',file=f)
    print('	ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );',file=f)
    print('	out = util_hash_ret( &ip, sizeof( ip ) );',file=f)
    print('	out &= ( (MPF_TBL_SIZE)-1 );',file=f)
    print('	hashes[j] = out;',file=f)
    print('	struct _routing_tbl_entry_t* ent = routing_entry_find( self, ip.dst );',file=f)
    print('	if( ent )',file=f)
    print('	{',file=f)
    print('		self->port_count += ent->port;',file=f)
    print('		ent->count++;',file=f)
    print('	}',file=f)
    print('}',file=f)

    print('for( uint32_t j = 0; j < sidx; ++j )',file=f)
    print('{',file=f)
    print('	self->tbl[hashes[j]]++;',file=f)
    print('}',file=f)
    print('self->slowpass_count += sidx;',file=f)
    print('self->_tmp_2 += fastpass_count;',file=f)
    print('element_dispatch( ele, 0, pkts, size );',file=f)
    print('}',file=f)

# Main
parsed,headers = parse()

opt_name = parsed['opt-name'][0].replace("-", "_")

if 'include-dir' in parsed:
	include_path = parsed['include-dir'][0][:-1]
else:
	include_path = '.'

if 'src-dir' in parsed:
	src_path = parsed['src-dir'][0][:-1]
else:
	src_path = '.'

# Generate new include file
finclude = open('{}/{}.h'.format(include_path,opt_name),'w')
gen_include_file(parsed,finclude)
finclude.close()

# Create new optimization source file and open it in append mode
open('{}/{}.c'.format(src_path,opt_name),'w').close()
fsrc = open('{}/{}.c'.format(src_path,opt_name), 'a')
gen_headers(headers,fsrc)
gen_create_fn(parsed,fsrc)
gen_process_fn(parsed,fsrc);
gen_release_fn(parsed,fsrc);
gen_report_fn(parsed,fsrc);
