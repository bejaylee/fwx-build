// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Per-conntrack data storage for fwx, replacing the patched
 * struct nf_conn->fwx_data field with a standalone hash table
 * keyed by conntrack pointer.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rculist.h>
#include <linux/timer.h>
#include <net/netfilter/nf_conntrack.h>
#include "fwx_ct_data.h"

#define FWX_CT_HASH_BITS 10
#define FWX_CT_HASH_SIZE (1 << FWX_CT_HASH_BITS)
#define FWX_CT_TIMEOUT (60 * HZ)

struct fwx_ct_entry {
	struct hlist_node node;
	struct nf_conn *ct;
	struct fwx_ct_data data;
	unsigned long last_used;
};

static struct hlist_head *fwx_ct_hash;
static DEFINE_SPINLOCK(fwx_ct_lock);

static inline u32 fwx_ct_hash_func(struct nf_conn *ct)
{
	unsigned long val = (unsigned long)ct;
	val = (val >> 4) ^ (val >> 12) ^ (val >> 20);
	return val & (FWX_CT_HASH_SIZE - 1);
}

struct fwx_ct_data *fwx_ct_data_get(struct nf_conn *ct)
{
	struct fwx_ct_entry *entry;
	u32 h = fwx_ct_hash_func(ct);

	rcu_read_lock();
	hlist_for_each_entry_rcu(entry, &fwx_ct_hash[h], node) {
		if (entry->ct == ct) {
			rcu_read_unlock();
			return &entry->data;
		}
	}
	rcu_read_unlock();
	return NULL;
}

struct fwx_ct_data *fwx_ct_data_get_or_create(struct nf_conn *ct)
{
	struct fwx_ct_entry *entry, *existing;
	u32 h = fwx_ct_hash_func(ct);

	/* Fast path: RCU lookup */
	rcu_read_lock();
	hlist_for_each_entry_rcu(entry, &fwx_ct_hash[h], node) {
		if (entry->ct == ct) {
			rcu_read_unlock();
			return &entry->data;
		}
	}
	rcu_read_unlock();

	/* Slow path: allocate and insert under lock */
	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry)
		return NULL;
	entry->ct = ct;
	entry->last_used = jiffies;

	spin_lock_bh(&fwx_ct_lock);
	/* Re-check to avoid duplicates */
	hlist_for_each_entry(existing, &fwx_ct_hash[h], node) {
		if (existing->ct == ct) {
			spin_unlock_bh(&fwx_ct_lock);
			kfree(entry);
			return &existing->data;
		}
	}
	hlist_add_head_rcu(&entry->node, &fwx_ct_hash[h]);
	spin_unlock_bh(&fwx_ct_lock);
	return &entry->data;
}

void fwx_ct_data_delete(struct nf_conn *ct)
{
	struct fwx_ct_entry *entry;
	u32 h = fwx_ct_hash_func(ct);

	spin_lock_bh(&fwx_ct_lock);
	hlist_for_each_entry(entry, &fwx_ct_hash[h], node) {
		if (entry->ct == ct) {
			hlist_del_rcu(&entry->node);
			spin_unlock_bh(&fwx_ct_lock);
			synchronize_rcu();
			kfree(entry);
			return;
		}
	}
	spin_unlock_bh(&fwx_ct_lock);
}

void fwx_ct_data_gc(void)
{
	struct fwx_ct_entry *entry;
	struct hlist_node *n;
	int i;
	unsigned long now = jiffies;

	spin_lock_bh(&fwx_ct_lock);
	for (i = 0; i < FWX_CT_HASH_SIZE; i++) {
		hlist_for_each_entry_safe(entry, n, &fwx_ct_hash[i], node) {
			if (time_after(now, entry->last_used + FWX_CT_TIMEOUT)) {
				hlist_del_rcu(&entry->node);
				kfree_rcu(entry, node);
			}
		}
	}
	spin_unlock_bh(&fwx_ct_lock);
}

int fwx_ct_data_init(void)
{
	int i;

	fwx_ct_hash = kzalloc(array_size(sizeof(struct hlist_head),
					 FWX_CT_HASH_SIZE), GFP_KERNEL);
	if (!fwx_ct_hash)
		return -ENOMEM;

	for (i = 0; i < FWX_CT_HASH_SIZE; i++)
		INIT_HLIST_HEAD(&fwx_ct_hash[i]);

	return 0;
}

void fwx_ct_data_fini(void)
{
	struct fwx_ct_entry *entry;
	struct hlist_node *n;
	int i;

	if (!fwx_ct_hash)
		return;

	spin_lock_bh(&fwx_ct_lock);
	for (i = 0; i < FWX_CT_HASH_SIZE; i++) {
		hlist_for_each_entry_safe(entry, n, &fwx_ct_hash[i], node) {
			hlist_del_rcu(&entry->node);
			kfree(entry);
		}
	}
	spin_unlock_bh(&fwx_ct_lock);

	kfree(fwx_ct_hash);
	fwx_ct_hash = NULL;
}
