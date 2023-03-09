// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PA2PGREF_ID(p) (((p)-KERNBASE)/PGSIZE)
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP)

int pageref[PGREF_MAX_ENTRIES];
#define PA2PGREF(p) pageref[PA2PGREF_ID((uint64)(p))]


void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct spinlock reflock;


void
incrementRef(uint64 pa)
{
  // acquire(&reflock);
  // pageref[PA2PGREF_ID(pa)]++;
  // release(&reflock);
  acquire(&reflock);
  PA2PGREF(pa)++;
  release(&reflock);
}

void
decrementRef(uint64 pa)
{
  // acquire(&reflock);
  int index = PA2PGREF_ID(pa);
  if(pageref[index]){
    pageref[index]--;
  }
  // release(&reflock);
}

int
getRef(uint64 pa)
{
  return pageref[PA2PGREF_ID(pa)];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&reflock, "pgref"); // 初始化锁
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&reflock);
  
  if(getRef((uint64)pa) > 1){
    decrementRef((uint64)pa);
    release(&reflock);
    return; // 这个page引用 > 1的话,我们不应该释放它,减少引用就可以了;
  } 

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  decrementRef((uint64)pa); // 减少ref

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  
  release(&reflock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    // 这里不会出现竞争,因为上面的链表锁保证这个pa是唯一的,不会再有其他人对这个槽进行写了;
    pageref[PA2PGREF_ID((uint64)r)] = 1; //  Set a page's reference count to one when kalloc() allocates it.
  }
  return (void*)r;
}


void *kcopy_n_deref(void *pa) {
  acquire(&reflock);

  if(pageref[PA2PGREF_ID((uint64)pa)] <= 1) { // 只有 1 个引用，无需复制
    release(&reflock);
    return pa;
  }

  // 分配新的内存页，并复制旧页中的数据到新页
  uint64 newpa = (uint64)kalloc();
  if(newpa == 0) {
    release(&reflock);
    return 0; // out of memory
  }
  memmove((void*)newpa, (void*)pa, PGSIZE);

  // 旧页的引用减 1
  pageref[PA2PGREF_ID((uint64)pa)]--;

  release(&reflock);
  return (void*)newpa;
}

