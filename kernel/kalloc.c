// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmemarr[NCPU];

void getName(int id,char* buf){
  buf[0] = 'k';
  buf[1] = 'm';
  buf[2] = 'e';
  buf[3] = 'm';
  buf[4] = '0' + id;
  buf[5] = 0;
}

void
kinit()
{
  for (int i = 0; i < NCPU; i++){
    char name[6];
    getName(i,name);
    initlock(&kmemarr[i].lock,name);
  }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmemarr[id].lock);
  r->next = kmemarr[id].freelist;
  kmemarr[id].freelist = r;
  release(&kmemarr[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();
  pop_off();  

  acquire(&kmemarr[id].lock);
  r = kmemarr[id].freelist;
  if(r)
    kmemarr[id].freelist = r->next;
  else{
    int hasFound = 0;
    // steal from others
    for (int i = 0; i < NCPU && !hasFound; i++)
    {
      if(i == id) continue;
      acquire(&kmemarr[i].lock);
      if(kmemarr[i].freelist){
        r = kmemarr[i].freelist;
        kmemarr[i].freelist = r->next;
        hasFound = 1;
      }
      release(&kmemarr[i].lock);
    }
  }  
  release(&kmemarr[id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
