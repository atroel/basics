#include "b6/heap.h"
#include "b6/allocator.h"

static void b6_heap_xchg(struct b6_heap *self, void **buf,
			 unsigned long int i, unsigned long int j)
{
	void *temp = buf[i];
	buf[i] = buf[j];
	buf[j] = temp;
}

static void b6_heap_xchg_cb(struct b6_heap *self, void **buf,
			    unsigned long int i, unsigned long int j)
{
	b6_heap_xchg(self, buf, i, j);
	self->set_index(buf[i], i);
	self->set_index(buf[j], j);
}

static unsigned long int b6_heap_rise(struct b6_heap *self, void **buf,
				      unsigned long int i)
{
	unsigned long int j = (i - 1) / 2;
	return self->compare(buf[i], buf[j]) < 0 ? j : i;
}

void b6_heap_do_push(struct b6_heap *self, void **buf, unsigned long int i)
{
	unsigned long int j;
	if (self->set_index)
		for (; i && i != (j = b6_heap_rise(self, buf, i)); i = j)
			b6_heap_xchg_cb(self, buf, i, j);
	else
		for (; i && i != (j = b6_heap_rise(self, buf, i)); i = j)
			b6_heap_xchg(self, buf, i, j);
}

void b6_heap_do_boost(struct b6_heap *self, void **buf, unsigned long int i)
{
	if (self->set_index)
		while (i) {
			unsigned long int j = (i - 1) / 2;
			b6_heap_xchg_cb(self, buf, i, j);
			i = j;
		}
	else
		while (i) {
			unsigned long int j = (i - 1) / 2;
			b6_heap_xchg(self, buf, i, j);
			i = j;
		}
}

static unsigned long int b6_heap_dive(const struct b6_heap *self,
				      void **buf, unsigned long int len,
				      unsigned long int i)
{
	unsigned long int m = i;
	unsigned long int l = m * 2 + 1;
	unsigned long int r = l + 1;
	if (l >= len)
		goto bail_out;
	if (self->compare(buf[m], buf[l]) > 0)
		m = l;
	if (r < len && self->compare(buf[m], buf[r]) > 0)
		m = r;
bail_out:
	return m;
}

void b6_heap_do_pop(struct b6_heap *self)
{
	unsigned long int len = b6_array_length(self->array) - 1;
	void **buf = b6_array_get(self->array, 0);
	unsigned long int i, j;
	if (self->set_index) {
		b6_heap_xchg_cb(self, buf, 0, len);
		for (i = 0; i != (j = b6_heap_dive(self, buf, len, i)); i = j)
			b6_heap_xchg_cb(self, buf, i, j);
	} else {
		b6_heap_xchg(self, buf, 0, len);
		for (i = 0; i != (j = b6_heap_dive(self, buf, len, i)); i = j)
			b6_heap_xchg(self, buf, i, j);
	}
}

void b6_heap_do_make(struct b6_heap *self)
{
	unsigned long int len = b6_array_length(self->array);
	void **buf = b6_array_get(self->array, 0);
	unsigned long int i, j, k = len / 2;
	if (self->set_index)
		do
			for (i = k; i != (j = b6_heap_dive(self, buf, len, i));
			     i = j)
				b6_heap_xchg_cb(self, buf, i, j);
		while (k--);
	else
		do
			for (i = k; i != (j = b6_heap_dive(self, buf, len, i));
			     i = j)
				b6_heap_xchg(self, buf, i, j);
		while (k--);
}
