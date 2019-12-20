# Parse a spec that describes optimizations to be generated (by default "gen.spec.template")
# Then generate optimizations file

from datetime import datetime
from enum import Enum

class Opt:
    def __init__(self, name: str):
        self.name = name
        self.filehandle = None

    def codegen(self):
        raise NotImplementedError

    def set_filehandle(self, filehandle):
        self.filehandle = filehandle

    def print_opt(self, msg):
        print(msg, file=self.filehandle)

class PrefetchOpt(Opt):
    def codegen(self, idx_var="i", pkt_var="p", offset=26, deref_var="hdr"):
        print_opt('rte_prefetch0( {}[{}]->{} + {} );'.format(pkt_var, idx_var, deref_var, offset),file=f)

class LoopSplitOpt(Opt):
    def codegen(self):
        pass

class BatchOpt(Opt):
    def codegen(self):
        pass

    def gen_batch_loop(self):
        pass

class NFType(Enum):
    MEASUREMENT = 1
    CHECKSUM = 2
    ROUTING = 3

class NF:
    def __init__(self, typ: NFType):
        self.type = typ
        self.opts = []

    def add_opt(self, opt: Opt):
        self.opts.append(opt)

    def codegen(self):
        raise NotImplementedError

def MeasurementNF(NF):
    def __init__(self):
        NF.__init__(self, NFType.MEASUREMENT)

    def codegen(self):
        pass

class Generator:
    def __init__(self, spec, filehandle):
        self.q_preamble = None
        self.q_loop = None
        self.q_main = None
        self.q_postamble = None
        self.f = filehandle
        self.parsed = spec

    # Generate functions call each other in a chain
    # in order to properly close all blocks

    def generate(self):
        self.preamble()

    # Before main loop starts: currently this is where prefetching and measurement/checksum setup happens
    def preamble(self):
        opt_name = parsed['opt-name'][0].replace("-", "_")
        opt_type = 'struct {}_t'.format(opt_name)
        fn_name = '{}_process'.format(opt_name)
        print('void {}(struct element_t* ele, struct packet_t** pkts, packet_index_t size)'.format(fn_name),file=self.f)
        print('{',file=self.f)
        print('{}* self = ({}*) ele;'.format(opt_type,opt_type),file=self.f)
        # Prefetching pattern
        print('struct packet_t* pkt;',file=self.f);
        print('struct packet_t* p[BATCH_SIZE];',file=self.f);
        if 'measurement' in parsed['nf-pipeline']:
            print('uint32_t hashes[SUB_BATCH_SIZE];',file=self.f);
        print('struct __attribute__( ( packed ) )',file=self.f)
        print('{',file=self.f)
        print('	ipv4_t src;',file=self.f)
        print('	ipv4_t dst;',file=self.f)
        print('	union {',file=self.f)
        print('		struct __attribute__( ( packed ) )',file=self.f)
        print('		{',file=self.f)
        print('			uint16_t src;',file=self.f)
        print('			uint16_t dst;',file=self.f)
        print('		} ports;',file=self.f)
        print('		uint32_t srcdst_port;',file=self.f)
        print('	};',file=self.f)
        print('} ip;',file=self.f)
        if 'prefetch' in parsed['optimizations']:
            print('for( int j = 0; j < SUB_BATCH_SIZE; ++j )',file=self.f)
            print('{',file=self.f)
            print('	    p[j] = pkts[j];',file=self.f)
            print('	    rte_prefetch0( p[j]->hdr + 26 );',file=self.f)
            print('}',file=self.f)

        self.loop()

    # This is the main loop processing packets
    # The loop will either process in batches
    # or will be split into smaller loops within
    # its body
    def loop(self):
        print('uint32_t out = 0;', file=self.f)
        if 'loop-split' in parsed['optimizations']:
            print('int i = SUB_BATCH_SIZE;', file=self.f)
            print('for( ; i < size - SUB_BATCH_SIZE; i += SUB_BATCH_SIZE )', file=self.f)
            print('{', file=self.f)

            # Prefetching
            if 'prefetch' in parsed['optimizations']:
                print(' // Prefetch the next set of packets', file=self.f)
                print('	for( int j = 0; j < SUB_BATCH_SIZE && i + j < size; ++j )', file=self.f)
                print('	{', file=self.f)
                print('		p[j + SUB_BATCH_SIZE] = pkts[i + j];', file=self.f)
                print('		rte_prefetch0( p[j + SUB_BATCH_SIZE]->hdr + 26 );', file=self.f)
                print('	}', file=self.f)
        else:
            print('int i = 0;', file=self.f)

        # Will split?
            # Continue with next gen. step
        # Determine batch size
        # Continue with next gen. step
        self.main()

    # Generates main processing logic
    # Consists of 1 or more loops
    # Body of loop depends pipelines specified
    def main(self):
        def inner_loop(loop_bound, first_loop = False):
            # Loop splitting: Loop 1
            print('	for( int j = 0; j < {}; ++j )'.format(loop_bound),file=self.f)
            print('	{',file=self.f)
            print('		pkt            = p[j];',file=self.f)
            print('		ip.src         = *( (ipv4_t*)( pkt->hdr + 14 + 12 ) );',file=self.f)
            print('		ip.dst         = *( (ipv4_t*)( pkt->hdr + 14 + 12 + 4 ) );',file=self.f)
            print('		ip.srcdst_port = *( (uint32_t*)( pkt->hdr + 14 + 12 + 8 ) );',file=self.f)
            # Measurement
            if 'measurement' in parsed['nf-pipeline']:
                print('		out = util_hash_ret( &ip, sizeof( ip ) );',file=self.f)
                print('		out &= ( (MPF_TBL_SIZE)-1 );',file=self.f)
                # TODO: handle j vs j-i between loops here (also for batch size)
                if first_loop:
                    print('		hashes[j] = out;',file=self.f)
                else:
                    print('		hashes[j - i] = out;',file=self.f)

            # Opt: prefetching (random access data; this is measurement module specific); probably not needed in this position
            # TODO: should give option to prefetch hash table accesses in both loops
            if first_loop:
                if 'prefetch' in parsed['optimizations'] and 'measurement' in parsed['nf-pipeline']:
                    print('		rte_prefetch0(self->tbl + out);',file=self.f)

            # Routing
            if 'routing' in parsed['nf-pipeline']:
                print('		struct _routing_tbl_entry_t* ent',file=self.f)
                print('		    = routing_entry_find( self, ip.dst );',file=self.f)
                print('		if( ent )',file=self.f)
                print('		{',file=self.f)
                print('			self->port_count += ent->port;',file=self.f)
                print('			ent->count++;',file=self.f)
                print('             }',file=self.f)

            # Checksum
            if 'checksum' in parsed['nf-pipeline']:
                print('		self->checksum_count += checksum( pkt->hdr, pkt->size );',file=self.f)
            # Measurement: fill hashes
            # TODO: only done in first loop
            if first_loop:
                if 'measurement' in parsed['nf-pipeline']:
                    print('             for (int j = 0; j < BATCH_SIZE_HALF; ++j) {',file=self.f)
                    print('                 self->tbl[hashes[j]]++;',file=self.f)
                    print('                 p[j] = p[j + BATCH_SIZE_HALF];',file=self.f)
                    print('             }',file=self.f)
            print(' }',file=self.f)

        # TODO: do this dependent on batch size
        if 'loop-split' in parsed['optimizations']:
            # TODO: use SUB_BATCH_SIZE instead
            inner_loop("BATCH_SIZE_HALF", first_loop = True)
            for i in range(1, int(parsed['params']['proc-loops'])):
                print('     i -= BATCH_SIZE_HALF;',file=self.f)
                inner_loop("size")
        else:
            inner_loop("size", first_loop = True)

        self.postamble()

    # End of NF
    def postamble(self):
        # Measurement: record hashes that were potentially prefetched ealier
        # TODO: parameterize size, j - i
        if 'measurement' in parsed['nf-pipeline']:
            print('     for( int j = i; j < size; ++j )',file=self.f)
            print('     {',file=self.f)
            print('     	self->tbl[hashes[j - i]]++;',file=self.f)
            print('     }',file=self.f)
        print('     element_dispatch( ele, 0, pkts, size );',file=self.f)

        if 'loop-split' in parsed['optimizations']:
            for i in range(1,int(parsed['params']['proc-loops'])):
                print('}',file=self.f)

        # Close process function
        print('}',file=self.f)

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

    if 'optimizations' not in sections:
        print('Failed to parse {}. No "optimizations" section found.'.format(spec_path))
        exit(1)

    if 'prefetch' in sections['optimizations']:
        headers.append("rte_prefetch.h")

    if 'params' in sections:
        param_dict = {}
        for kv in sections['params']:
            [key, value] = kv.split(':')
            param_dict[key.strip()] = value.strip()
        sections['params'] = param_dict

        if 'loop-split' in sections['optimizations']:
            sections['params']['proc-loops'] = sections['params'].get('proc-loops', 2)

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
    print( '{}* {}_create(void);'.format(opt_type, opt_name),file=f)
    print( 'void {}_process(struct element_t*, struct packet_t**, packet_index_t);'.format(opt_name),file=f)
    print( 'void {}_release(struct element_t*);'.format(opt_name),file=f)
    print( 'void {}_report(struct element_t*);'.format(opt_name),file=f)

    # Close include guard
    print('#endif // {}_IN_H'.format(opt_name.upper()),file=f)

def gen_create_fn(spec, f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)
    fn_name = '{}_create'.format(opt_name)

    if 'params' in spec:
        params = spec['params']
    else:
        params = {}

    print('#include "elements/{}.h"'.format(opt_name),file=f)

    print('#define BATCH_SIZE {} // How many packets per batch'.format(params.get('batch', 32)),file=f)
    print('#define SPLITS {}'.format(params['proc-loops']),file=f)
    print('#define SUB_BATCH_SIZE (BATCH_SIZE / SPLITS)',file=f)
    print('#define BATCH_SIZE_HALF (BATCH_SIZE >> 1)',file=f)
    print('#define MPF_TBL_SIZE ( 1 << 24 )',file=f)

    print('{}* {}(void)'.format(opt_type, fn_name),file=f)
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

    print('// Setup checksum: nop', file=f)
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

def gen_process_fn(spec,f):
    gen = Generator(spec, f)
    gen.generate()

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
