/*
 * (Re)allocate non-volatile write set entries.
 */
static inline 
void 
mtm_allocate_ws_entries_nv(mtm_tx_t *tx, mode_data_t *data, int extend)
{
	if (extend) {
		/* Extend write set */
		data->w_set_nv.size *= 2;
		PRINT_DEBUG("==> reallocate write set (%p[%lu-%lu],%d)\n", tx, 
		            (unsigned long)data->start, (unsigned long)data->end, data->w_set_nv.size);
		if ((data->w_set_nv.entries = 
		     (w_entry_t *)realloc(data->w_set_nv.entries, 
		                          data->w_set_nv.size * sizeof(w_entry_t))) == NULL) 
		{
			perror("realloc");
			exit(1);
		}
	} else {
		/* Allocate write set */
#if ALIGNMENT == 1 /* no alignment requirement */
		if ((data->w_set_nv.entries = 
		     (w_entry_t *)malloc(data->w_set_nv.size * sizeof(w_entry_t))) == NULL)
		{
			perror("malloc");
			exit(1);
		}
#else
		if (posix_memalign((void **)&data->w_set_nv.entries, 
		                   ALIGNMENT, 
		                   data->w_set_nv.size * sizeof(w_entry_t)) != 0) 
		{
			fprintf(stderr, "Error: cannot allocate aligned memory\n");
			exit(1);
		}
#endif
	}
}


