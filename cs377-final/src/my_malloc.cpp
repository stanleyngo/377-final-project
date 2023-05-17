#include <assert.h>
#include <my_malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>

// a mutex to lock and unlock critical sections
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// A pointer to the head of the free list.
node_t *head = NULL;

// The heap function returns the head pointer to the free list. If the heap
// has not been allocated yet (head is NULL) it will use mmap to allocate
// a page of memory from the OS and initialize the first free node.
node_t *heap() {
  pthread_mutex_lock(&mutex);
  if (head == NULL) {
    // This allocates the heap and initializes the head node.
    head = (node_t *)mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                          MAP_ANON | MAP_PRIVATE, -1, 0);
    head->size = HEAP_SIZE - sizeof(node_t);
    head->next = NULL;
  }
  pthread_mutex_unlock(&mutex);

  return head;
}

// Reallocates the heap.
void reset_heap() {
  if (head != NULL) {
    munmap(head, HEAP_SIZE);
    head = NULL;
    heap();
  }
}

// Returns a pointer to the head of the free list.
node_t *free_list() { return head; }

// Calculates the amount of free memory available in the heap.
size_t available_memory() {
  size_t n = 0;
  node_t *p = heap();
  pthread_mutex_lock(&mutex);
  while (p != NULL) {
    n += p->size;
    p = p->next;
  }
  pthread_mutex_unlock(&mutex);
  return n;
}

// Returns the number of nodes on the free list.
int number_of_free_nodes() {
  int count = 0;
  node_t *p = heap();
  pthread_mutex_lock(&mutex);
  while (p != NULL) {
    count++;
    p = p->next;
  }
  pthread_mutex_unlock(&mutex);
  return count;
}

// Prints the free list. Useful for debugging purposes.
void print_free_list() {
  node_t *p = heap();
  while (p != NULL) {
    printf("Free(%zd)", p->size);
    p = p->next;
    if (p != NULL) {
      printf("->");
    }
  }
  printf("\n");
}

// Allocates a new block if we run out of memory

// PARAMETERS:
// None


void allocate() {
  pthread_mutex_lock(&mutex);
  if (available_memory() == 0) {
    node_t* last_block = heap();
    while (last_block -> next != NULL) {
      last_block = last_block -> next;
    }
    node_t* new_block = (node_t *)mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                          MAP_ANON | MAP_PRIVATE, -1, 0);
    new_block->size = HEAP_SIZE - sizeof(node_t);
    new_block->next = NULL;
    last_block -> next = new_block;  
  }
  pthread_mutex_unlock(&mutex);
}

// Finds a node on the free list that has enough available memory to
// allocate to a calling program. This function uses the "first-fit"
// algorithm to locate a free node.
//
// PARAMETERS:
// size - the number of bytes requested to allocate
//
// RETURNS:
// found - the node found on the free list with enough memory to allocate
// previous - the previous node to the found node
//
void find_free(size_t size, node_t **found, node_t **previous) {
  node_t* p = heap();
  node_t* p2 = NULL;
  size_t actual_size = size + sizeof(header_t);
  pthread_mutex_lock(&mutex);
  while (p != NULL) {
    if (p -> size >= actual_size) {
      *found = p;
      if (p2 != NULL) {
        *previous = p2;
      }
    }
    p2 = p;
    p = p -> next;
  }
  pthread_mutex_unlock(&mutex);
  allocate();
}

// Splits a found free node to accommodate an allocation request.
//
// The job of this function is to take a given free_node found from
// `find_free` and split it according to the number of bytes to allocate.
// In doing so, it will adjust the size and next pointer of the `free_block`
// as well as the `previous` node to properly adjust the free list.
//
// PARAMETERS:
// size - the number of bytes requested to allocate
// previous - the previous node to the free block
// free_block - the node on the free list to allocate from
//
// RETURNS:
// allocated - an allocated block to be returned to the calling program
//
void split(size_t size, node_t **previous, node_t **free_block,
           header_t **allocated) {
  assert(*free_block != NULL);
  node_t *original_block = *free_block;
  size_t actual_size = size + sizeof(header_t);
  size_t original_size = (*free_block) -> size;
  *free_block = (node_t *)(((char *)*free_block) + actual_size);
  (*free_block) -> size = original_size - actual_size;
  if (*previous == NULL) {
    head = *free_block;
  }
  else {
    (*previous) -> next = *free_block;
  }
  *allocated = (header_t*)(void*)original_block;
  (*allocated) -> size = size;
  (*allocated) -> magic = MAGIC;
  // TODO
}

// Returns a pointer to a egion of memory having at least the request `size`
// bytes.
//
// PARAMETERS:
// size - the number of bytes requested to allocate
//
// RETURNS:
// A void pointer to the region of allocated memory
//
void *my_malloc(size_t size) {
  // TODO
  node_t* found = NULL;
  node_t* previous = NULL;
  header_t* allocated = NULL;
  find_free(size, &found, &previous);
  if (found == NULL) {
    return NULL;
  }
  pthread_mutex_lock(&mutex);
  split(size, &previous, &found, &allocated);
  pthread_mutex_unlock(&mutex);
  allocated = (header_t*)(((char*) allocated) + sizeof(header_t));
  return allocated;
}

// Merges adjacent nodes on the free list to reduce external fragmentation.
//
// This function will only coalesce nodes starting with `free_block`. It will
// not handle coalescing of previous nodes (we don't have previous pointers!).
//
// PARAMETERS:
// free_block - the starting node on the free list to coalesce
//

// Modified Coalesce

// This function will coalesce all adjacent nodes starting from the head of the list

//PARAMETERS:
// none - we use the head of the list declared above

void coalesce() {
  // TODO
  node_t* p = head;
  while (p != NULL && p -> next != NULL) {
    size_t block_size = p->size + sizeof(node_t);
    node_t* curr_address = p -> next;
    char* next_address = (char*) p -> next -> next;
    if (((char *)curr_address) + block_size == next_address) {
      p -> size += sizeof(node_t) + sizeof(header_t) + p -> next -> size;
      p -> next = p -> next -> next;
    }
    else {
      p = p -> next;
    }
  }
}

// Frees a given region of memory back to the free list.
//
// PARAMETERS:
// allocated - a pointer to a region of memory previously allocated by my_malloc
//
void my_free(void *allocated) {
  // TODO
  pthread_mutex_lock(&mutex);
  header_t* allocatedHeader = NULL;
  node_t* allocatedNode = NULL;
  allocatedHeader = (header_t*) allocated;
  //size_t actual_size = allocatedHeader -> size - sizeof(header_t);
  allocatedHeader = (header_t *)(((char *)allocatedHeader) - sizeof(header_t));
  assert(allocatedHeader -> magic == MAGIC);
  allocatedNode = (node_t*) allocatedHeader;
  allocatedNode -> size = allocatedHeader -> size;
  allocatedNode -> next = head;
  head = allocatedNode;
  coalesce();
  pthread_mutex_unlock(&mutex);
  
}
