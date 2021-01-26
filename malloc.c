#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


#define ALIGN4(s) (((((s)-1) >> 2) << 2) + 4)
#define BLOCK_DATA(b) ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr)-1)

static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_splits = 0;
static int num_coalesces = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics(void)
{
printf("\nheap management statistics\n");
printf("mallocs:\t%d\n", num_mallocs);
printf("frees:\t\t%d\n", num_frees);
printf("reuses:\t\t%d\n", num_reuses);
printf("grows:\t\t%d\n", num_grows);
printf("splits:\t\t%d\n", num_splits);
printf("coalesces:\t%d\n", num_coalesces);
printf("blocks:\t\t%d\n", num_blocks);
printf("requested:\t%d\n", num_requested);
printf("max heap:\t%d\n", max_heap);
}

struct _block
{
size_t size;         /* Size of the allocated _block of memory in bytes */
struct _block *next; /* Pointer to the next _block of allcated memory   */
bool free;           /* Is this _block free?                     */
char padding[3];
};

struct _block *freeList = NULL; /* Free list to track the _blocks available */
struct _block *previous = NULL; // to use for next fit implementation

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
struct _block *curr = freeList;

#if defined FIT && FIT == 0
/* First fit */
while (curr && !(curr->free && curr->size >= size))
{
 *last = curr;
 curr = curr->next;
}
#endif

#if defined BEST && BEST == 0



   struct _block *best = NULL;
   int best_size = -1;
   int block_size_dif;
   while (curr != NULL)
   {
      *last = curr;
      if (curr->free && curr->size >= size)
      {
   block_size_dif = (curr->size) - size;
   
   if(best_size == -1 || block_size_dif < best_size)
   {
  best = curr;
  //best_size = curr;
                  best_size=curr->size;
   }
      }
      curr = curr->next;
   }
   curr = best;
#endif

#if defined WORST && WORST == 0


   struct _block *worst = NULL;
   int worst_size = 0;
   int worst_size_dif;
   while (curr != NULL)
   {
      *last = curr;
      if ((curr->free && curr->size >= size) && curr->size > worst_size)
      {
   worst_size_dif = (curr->size) - size;
   
   if(worst_size == 0 || worst_size_dif >= worst_size)
   {
         worst = curr;
         worst_size = curr->size;
   }
      }
      curr = curr->next;
   }
   curr = worst;


#endif

#if defined NEXT && NEXT == 0

curr = previous;
 if (curr == NULL)
{
 curr = freeList;
 while (curr && !(curr->free && curr->size >= size))
 {
  *last = curr;
  curr = curr->next;
 }
}
while (curr && !(curr->free && curr->size >= size))
{
 *last = curr;
 curr = curr->next;
}

previous= curr;
#endif

/*  Increment num_reuses*/

//if (curr!=NULL)
      if(curr)
{
 num_reuses++;
}


/* Return pointer */
return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
/* Request more space from OS */
struct _block *curr = (struct _block *)sbrk(0);
struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

assert(curr == prev);

/* OS allocation failed */
if (curr == (struct _block *) - 1)
{
 return NULL;
}

/* Update freeList if not set */
if (freeList == NULL)
{
 freeList = curr;
}

/* Attach new _block to prev _block */
if (last)
{
 last->next = curr;
}

/* Update _block metadata */
curr->size = size;
curr->next = NULL;
curr->free = false;

/* Statistics */
      num_blocks++;
num_grows++;
max_heap += size;
return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
/* Statistics */
num_requested += size;

if (atexit_registered == 0)
{
 atexit_registered = 1;
 atexit(printStatistics);
}

/* Align to multiple of 4 */
size = ALIGN4(size);

/* Handle 0 size */
if (size == 0)
{
 return NULL;
}

/* Look for free _block */
struct _block *last = freeList;
struct _block *next = findFreeBlock(&last, size);

/* Could not find free _block, so grow heap */
if (next == NULL)
{
 next = growHeap(last, size);
}
else
{
 /* Split free _block if possible */
 if (next->size >= size + sizeof(struct _block) * 2)
 {
  struct _block *newblock = (struct _block *)((void*)(next + 1) + size);

  newblock->free = true;
  newblock->size = next->size - size - sizeof(struct _block);
  newblock->next = next->next;

  next->size = size;
  next->next = newblock;

  /* Statistics */
                  num_splits++;
                  num_blocks++;
 }
}

/* Could not find free _block or grow heap, so just return NULL */
if (next == NULL)
{
 return NULL;
}

/* Mark _block as in use */
next->free = false;

/* Statistics */
num_mallocs++;

/* Return data address associated with _block */
return BLOCK_DATA(next);
}

void *realloc(void *ptr, size_t size)
{
if (ptr == NULL)
{
 return malloc(size);
}
else if (size == 0)
{
 free(ptr);
 return NULL;
}

size_t old_size = BLOCK_HEADER(ptr)->size;
void *ptr2 = malloc(size);

if (size > old_size)
 memcpy(ptr2, ptr, old_size);
else
 memcpy(ptr2, ptr, size);

free(ptr);
return ptr2;
}

void *calloc(size_t nmemb, size_t size)
{
size_t t_Size = nmemb * size;

void *ptr = malloc(t_Size);
if (ptr != NULL)
 memset(ptr, 0, t_Size);
return ptr;
}

/*
 * \brief Coalesce
 *
 * Coalesce free _blocks if needed
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
 void coalesce()
{
struct _block *current = freeList;

while (current != NULL)
{
 if (current->free && current->next && current->next->free)
 {
  current->size += current->next->size + sizeof(struct _block);
  current->next = current->next->next;

  /* Statistics */
                  num_coalesces++;
                  num_blocks--;
 
    }

 current = current->next;
}
 }

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
if (ptr == NULL)
{
 return;
}

/* Make _block as free */
struct _block *curr = BLOCK_HEADER(ptr);
assert(curr->free == 0);
curr->free = true;

/* Coalesce free _blocks if applicable */
coalesce();

/* Statistics */
num_frees++;
}



/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
