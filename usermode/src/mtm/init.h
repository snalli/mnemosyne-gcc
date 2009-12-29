#ifndef _INIT_H
#define _INIT_H

int mtm_init_global(void);
mtm_tx_t *mtm_init_thread(void);
void mtm_fini_global(void);
void mtm_fini_thread(void);

#endif
