/**
* Malloc Lab
* CS 241 - Fall 2018
*/

#include <string.h>
#include <unistd.h>

// TODO: align
// toimplement:- merge,split
// sbrk some extra space every time we need it.
// This does no bookkeeping and therefore has no ability to free, realloc, etc.


void *nofree_malloc(size_t size) {
    void *p = sbrk(0);
    void *request = sbrk(size);
    if (request == (void *) -1) {
        return NULL; // sbrk failed
    } else {
        return p;
    }
}

typedef struct block {
    size_t size;
    struct block *next;
    struct block *prev;
    int free;
} block;


static size_t blockSize = (size_t)
sizeof(block);
static void *globalBase = NULL;
static block *free_head = NULL;

// Iterate through blocks until we find one that's large enough.
// TODO: split block up if it's larger than necessary
void insert_free_block(block *block_ptr) {
    block_ptr->free = 1;
    block_ptr->next = free_head;
    block_ptr->prev = free_head->prev;
    free_head->prev->next = block_ptr;
    free_head->prev = block_ptr;
}

block *get_next_block(block *ptr) {
    block *x = (block *) ((char *) ptr + ptr->size);
    if (x == sbrk(0)) {
        return NULL;
    } else {
        return x;
    }
}

void remove_free_ptr(block *ptr) {
    if (ptr == free_head) {
        if (ptr->next != free_head)
            free_head = ptr->next;
        else
            free_head = NULL;
    }
    ptr->next->prev = ptr->prev;
    ptr->prev->next = ptr->next;
}

void merge(block *ptr) {
    block *nextBlockPtr = get_next_block(ptr);
    if (nextBlockPtr->free == 1) {
        ptr->size = ptr->size + blockSize + ptr->next->size;
        remove_free_ptr(ptr->next);
    }
}

void split(block *ptr, size_t size) {
    size_t newSize = ptr->size - size - blockSize;
    ptr->size = size;
    block *x = get_next_block(ptr);
    x->size = newSize;
    insert_free_block(x);
}

block *find_free_block(size_t size) {
    block *current = free_head;
    block *bestFit = NULL;
    if (current) {
        do {
            merge(current);
            if (current->size >= size) {
                if (bestFit) {
                    bestFit = (bestFit->size) > (current->size) ? current : bestFit;
                } else {
                    bestFit = current;
                }
            }
            current = current->next;
        } while (current && current->next != free_head);
    }
    if (bestFit) {
        if (bestFit->size > size + blockSize) {
            split(bestFit, size);
        }
        remove_free_ptr(bestFit);
    }
    return bestFit;
}

block *request_space(block *last, size_t size) {
    block *block;
    block = sbrk(0);
    void *request = sbrk(size + blockSize);
    //assert((void*)block == request); // Not thread safe.
    if (request == (void *) -1) {
        return NULL; // sbrk failed.
    }

    if (last) { // NULL on first request.
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->free = 0;
    return block;
}

// If it's the first ever call, i.e., globalBase == NULL, request_space and set globalBase.
// Otherwise, if we can find a free block, use it.
// If not, request_space.
void *malloc(size_t size) {
    block *x;
    // TODO: align size?

    if (size <= 0) {
        return NULL;
    }

    if (!globalBase) { // First call.
        x = request_space(NULL, size);
        if (!x) {
            return NULL;
        }
        globalBase = x;
    } else {
        block *last = globalBase;
        x = find_free_block(size);
        if (!x) { // Failed to find free block.
            x = request_space(last, size);
            if (!x) {
                return NULL;
            }
        } else {      // Found free block
            // TODO: consider splitting block here.
            x->free = 0;
        }
    }

    return (x + 1);
}

void *calloc(size_t nelem, size_t elsize) {
    size_t size = nelem * elsize;
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

block *get_block_ptr(void *ptr) {
    return (block *) ptr - 1;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    // TODO: consider merging blocks once splitting blocks is implemented.
    block *block_ptr = get_block_ptr(ptr);
    if (free_head == NULL) {
        free_head = block_ptr;
        free_head->next = block_ptr;
        free_head->prev = block_ptr;
    } else {
        insert_free_block(block_ptr);
    }
    block_ptr->free = 1;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        // NULL ptr. realloc should act like malloc.
        return malloc(size);
    }

    block *block_ptr = get_block_ptr(ptr);
    if (block_ptr->size >= size) {
        // We have enough space. Could free some once we implement split.
        return ptr;
    }

    // Need to really realloc. Malloc new space and free old space.
    // Then copy old data to new space.
    void *new_ptr;
    new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL; // TODO: set errno on failure.
    }
    memcpy(new_ptr, ptr, block_ptr->size);
    free(ptr);
    return new_ptr;
}