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
    print('#include "elements/measurement.h"',file=f)
    print('#include "elements/routing.h"',file=f)
    for h in headers:
        print('#include "{}"'.format(h),file=f)

def gen_defs(spec):
    pass

# Input: parsed spec file
def gen_include_file(spec,f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    opt_type = 'struct {}_t'.format(opt_name)

    # Create optimization file
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

    print('#include "elements/_{}.h"'.format(opt_name),file=f)

    print('#define MPF_SIZE 16',file=f)
    print('#define MPF_SIZE_HALF ( MPF_SIZE >> 1 )',file=f)
    print('#define MPF_TBL_SIZE ( 1 << 18 )',file=f)

    print('{} {}(uint32_t src, uint32_t dst, uint32_t srcdst_port).'.format(opt_type, fn_name),file=f)
    print("{",file=f)
    print('{}* {} = ({}*) mem_alloc(sizeof({}));'.format(opt_type, opt_name, opt_type,opt_type),file=f)
    print('memset({}, 0, sizeof({}));'.format(opt_name,opt_type),file=f)

    print('{}->element.process    = {}_process   ;'.format(opt_name,opt_name),file=f)
    print('{}->element.release    = {}_release   ;'.format(opt_name,opt_name),file=f)
    print('{}->element.report     = {}_report    ;'.format(opt_name,opt_name),file=f)
    print('{}->element.connect    = element_connect   ;',file=f)
    print('{}->element.disconnect = element_disconnect;',file=f)
    print('{}->element.hop_at     = element_hop_at    ;',file=f)
    print('\n// Set up measurement pool\n',file=f)
    print('{}->tbl_size = ge_pow2_64(MPF_TBL_SIZE);'.format(opt_name),file=f)
    print('{}->tbl = mem_alloc(sizeof(uint32_t) * {}->tbl_size);'.format(opt_name,opt_name),file=f)
    print('memset( {}->tbl, 0, sizeof( uint32_t ) * {}->tbl_size );'.format(opt_name,opt_name),file=f)
    print('{}->a = src;'.format(opt_name),file=f)
    print('{}->b = dst;'.format(opt_name),file=f)
    print('{}->c = srcdst_port;'.format(opt_name),file=f)
    print('{}->_tmp_2 = 0;'.format(opt_name),file=f)
    print('{}->slowpass_count = 0;'.format(opt_name),file=f)
    print('return ' + opt_name + ';',file=f)
    print("}\n",file=f)

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

    print('}\n',file=f)

def gen_report_fn(spec,f):
    opt_name = spec['opt-name'][0].replace("-", "_")
    fn_name = '{}_report'.format(opt_name)
    print('void {}( __attribute__( ( unused ) ) struct element_t* _ )'.format(fn_name),file=f)
    print('{',file=f)
    print('}\n',file=f)

# Main
parsed,headers = parse()

opt_name = parsed['opt-name'][0].replace("-", "_")

# Generate new include file
finclude = open('_{}.h'.format(opt_name),'w')
gen_include_file(parsed,finclude)
finclude.close()

# Create new optimization source file and open it in append mode
open('_gen_{}.c'.format(opt_name),'w').close()
fsrc = open('_gen_{}.c'.format(opt_name), 'a')
gen_headers(headers,fsrc)
gen_create_fn(parsed,fsrc)
#gen_process_fn(parsed,fsrc);
gen_release_fn(parsed,fsrc);
gen_report_fn(parsed,fsrc);