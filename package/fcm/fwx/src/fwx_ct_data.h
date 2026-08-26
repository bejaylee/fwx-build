/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Per-conntrack data storage for fwx, replacing the patched
 * struct nf_conn->fwx_data field with a standalone hash table.
 */
#ifndef __FWX_CT_DATA_H__
#define __FWX_CT_DATA_H__

#include <linux/types.h>

struct nf_conn;

struct fwx_ct_data {
	u32 app_id;
	u32 action;
	u32 match_status;
};

/* Look up per-conntrack data. Returns NULL if not yet allocated. */
struct fwx_ct_data *fwx_ct_data_get(struct nf_conn *ct);

/* Look up or create per-conntrack data. Returns NULL on allocation failure. */
struct fwx_ct_data *fwx_ct_data_get_or_create(struct nf_conn *ct);

/* Delete data for a conntrack entry. */
void fwx_ct_data_delete(struct nf_conn *ct);

/* Clean up stale entries (call from timer). */
void fwx_ct_data_gc(void);

int  fwx_ct_data_init(void);
void fwx_ct_data_fini(void);

#endif /* __FWX_CT_DATA_H__ */
