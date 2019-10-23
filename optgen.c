// Given optimization specification file
// generate a benchmark
//

void parse_spec();

void gen_create_fn()
{
	struct merged_fastpass_t* merged = (struct merged_fastpass_t*)mem_alloc(
	    sizeof( struct merged_fastpass_t ) );
	memset( merged, 0, sizeof( struct merged_fastpass_t ) );

	merged->element.process    = merged_fastpass_process;
	merged->element.release    = merged_fastpass_release;
	merged->element.report     = merged_fastpass_report;
	merged->element.connect    = element_connect;
	merged->element.disconnect = element_disconnect;
	merged->element.hop_at     = element_hop_at;

	// Setup routing pool
	// nop;

	// Setup measurement pool
	merged->tbl_size = ge_pow2_64( MPF_TBL_SIZE );
	merged->tbl      = mem_alloc( sizeof( uint32_t ) * merged->tbl_size );
	memset( merged->tbl, 0, sizeof( uint32_t ) * merged->tbl_size );

	// Setup checksum
	// nop;

	merged->a              = a; // src?
	merged->b              = b; // dst?
	merged->c              = c; // srcdst_port?
	merged->_tmp_2         = 0;
	merged->slowpass_count = 0;

	return merged;
}

void gen_create_fn();
void gen_el_create_process();
void gen_el_create_release();
void gen_el_create_report();
