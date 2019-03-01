#include <stdio.h>  // needed for size_t
#include <sys/mman.h> // needed for mmap
#include <assert.h> // needed for asserts
#include "dmm.h"
//
/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
  /* size_t is the return type of the sizeof operator. Since the size of an
   * object depends on the architecture and its implementation, size_t is used
   * to represent the maximum size of any object in the particular
   * implementation. size contains the size of the data object or the number of
   * free bytes
   */
  size_t size;
  struct metadata* next;
  struct metadata* prev; 
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * sorted to improve coalescing efficiency 
 */

static metadata_t* freelist = NULL;

void coalesce(metadata_t* addr)
{
        if(addr -> next != NULL)
        {
                        if(addr -> next == ((void*)addr) + addr -> size)
                        {
                                if(addr -> next -> next != NULL)
                                {
                                        addr -> next -> next -> prev = addr;
                                }
                                addr -> size += addr -> next -> size;
                                addr -> next = addr -> next -> next;
                                coalesce(addr);

                        }
                        else
                        {
                                coalesce(addr -> next);
                        }
        }
}
void* checkblock(size_t numbytes, metadata_t * current)
{
        if(current == NULL)
        {
                return NULL;
        }
        if((current -> size > numbytes + sizeof(metadata_t)) /*&& (current -> inuse == 0)*/)
        {
                metadata_t* newblock = ((void*)current) + numbytes + sizeof(metadata_t);
                /*current -> inuse = 1;*/
                newblock -> size = current -> size - ((int)(newblock) - (int)(current));
                current -> size = (int)(newblock) - (int)(current);
                newblock -> prev = current -> prev;
                newblock -> next = current -> next;
                if (current ->next != NULL)
                {
                        current ->next -> prev = newblock;
                }
                if(current -> prev != NULL)
                {
                        current -> prev -> next = newblock;
                }
                else
                {
                        freelist = newblock;
                }
                //print_freelist();
                //printf("\nsize:%zd\n", current -> size);
                //printf("\naddress allocated:%p\n",((void*)current)+sizeof(metadata_t));
                //if(current -> size > 4507756864)
                //{
                //        while(1)
                //        {
                //                int x = 0;
                //        }
                //}
                return ((void*)current)+sizeof(metadata_t);
        }
        else
        {
                return checkblock(numbytes, current -> next);
        }
}

void* dmalloc(size_t numbytes) {
  /* initialize through mmap call first time */
  if(freelist == NULL) { 			
    if(!dmalloc_init())
      return NULL;
  }

  assert(numbytes > 0);
  //printf("PROGRAM HAS CALLED DMALLOC(%zx)!!!!!!!!!!!!\n",numbytes);
  //print_freelist();
  if(numbytes % sizeof(metadata_t) != 0)
  {
        numbytes =  sizeof(metadata_t)*(numbytes/sizeof(metadata_t) + 1);
  }
  /* your code here */
  return checkblock(numbytes, freelist); //+ sizeof(metadata_t);
	
}

void dfree(void* ptr) {
  /* your code here */
        //printf("PROGRAM HAS CALLED DFREE(%p)!!!!!!!!!!!!\n",ptr);
        //printf("BEGINING");
        //print_freelist();
        metadata_t* tofree = ptr - sizeof(metadata_t);
        //printf("\n SIZE:%zd\n",tofree -> size);
        //tofree -> inuse = 0;
        if((int)tofree < (int)freelist)
        {
                tofree ->next = freelist;
                tofree ->next -> prev = tofree;
                freelist = tofree;
        }
        else
        {
                metadata_t* freelist_head = freelist;
                while(freelist_head < tofree)
                {
                        freelist_head = freelist_head -> next;
                }
                tofree -> next = freelist_head;
                tofree ->prev = tofree ->next -> prev;
                if(tofree ->prev != NULL)
                {
                        tofree ->prev -> next = tofree;
                }
                if(tofree -> next != NULL)
                {
                        tofree ->next -> prev = tofree;
                }
        }
        //printf("ABOUT TO COALESCE");
        coalesce(freelist);

}

bool dmalloc_init() {

  /* Two choices: 
   * 1. Append prologue and epilogue blocks to the start and the
   * end of the freelist 
   *
   * 2. Initialize freelist pointers to NULL
   *
   * Note: We provide the code for 2. Using 1 will help you to tackle the 
   * corner cases succinctly.
   */

  size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
  /* returns heap_region, which is initialized to freelist */
  freelist = (metadata_t*) mmap(NULL, max_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  /* Q: Why casting is used? i.e., why (void*)-1? */
  if (freelist == (void *)-1)
    return false;
  freelist->next = NULL;
  freelist->prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  //for(int i = sizeof(metadata_t); i < freelist -> size; i++)
  //{
  //        ((int *) freelist)[i] = 0;
  //}
  return true;
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    printf("\nFreelist Size:%zx, Head:%p, Prev:%p, Next:%p\t",
	  freelist_head->size,
          //freelist_head->inuse,
	  freelist_head,
	  freelist_head->prev,
	  freelist_head->next);
    freelist_head = freelist_head->next;
  }
  printf("\n");
}
