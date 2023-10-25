/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

/* Basic constants and macros */
/* sf_header */
/* sf_footer */
/* sf_block */

int total_payload;
int total_allocated_block;

#define GET_PAYLOAD_SIZE(header) ((header >> 32) & 0xFFFFFFFF)
#define GET_BLOCK_SIZE(header)   (header & 0x00000000FFFFFFF0)
#define GET_ALLOC_BIT(header)   (((header) >> 3) & 0x1)
#define GET_PREV_ALLOC(header)  (((header) >> 2) & 0x1)
#define GET_NEXT_BLOCK(bp)      ((sf_block *)bp)
#define LIST_EMPTY(index) (sf_free_list_heads[index].body.links.next == &sf_free_list_heads[index] && sf_free_list_heads[index].body.links.prev == &sf_free_list_heads[index])

#define BLOCK_SIZE_MASK 0xF000000000000000

int find_min_index(size_t header){
    size_t asize = GET_BLOCK_SIZE(header);
    if(asize <= sizeof(sf_block))
        return 0;
    else{
        int index = 1;
        size_t class_size = sizeof(sf_block);
        while(index < NUM_FREE_LISTS - 2 && class_size < asize){
            class_size *= 2;
            index++;
        }
        return index;
    }
}

void init_sentinels() {
    for(int i = 0; i < NUM_FREE_LISTS; i++){            /* Initializing sentinels */
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

int is_wilderness(sf_block *bp){
    size_t size = GET_BLOCK_SIZE(bp->header);
    sf_footer *block_footer = (void *)bp + size;
    return block_footer == sf_mem_end() - 16;
}

void add_new_free_block(sf_block *new_free_block, int index){

    void *sentinel = &sf_free_list_heads[index];
    void *bp = ((sf_block *)sentinel)->body.links.next;


    if(LIST_EMPTY(index)){
        sf_free_list_heads[index].body.links.next = new_free_block;
        sf_free_list_heads[index].body.links.prev = new_free_block;
        new_free_block->body.links.next = sentinel;
        new_free_block->body.links.prev = sentinel;
        return;
    }


    while(bp !=  sentinel){
        if( ((sf_block *)bp)->body.links.prev == sentinel){
            ((sf_block *)bp)->body.links.prev = new_free_block;
            new_free_block->body.links.next = ((sf_block *)bp);
            new_free_block->body.links.prev = ((sf_block *)sentinel);
            ((sf_block *)sentinel)->body.links.next = new_free_block;
            return;
        }
        bp = ((sf_block *)bp)->body.links.next;

    }
}

void remove_free_block(void *bp, int index){
    for(int i = index; i < NUM_FREE_LISTS; i++){
        sf_block *sentinel = &sf_free_list_heads[i];
        sf_block *cursor = sentinel->body.links.next;
        while(cursor != sentinel){
            if(cursor == bp){
                // Found target block pointer
                sf_block *prev_block = ((sf_block *)bp)->body.links.prev;
                sf_block *next_block = ((sf_block *)bp)->body.links.next;
                prev_block->body.links.next = next_block;
                next_block->body.links.prev = prev_block;
                return;
            }
            cursor = cursor->body.links.next;
        }
    }
    //Should not reach the end...
}

void remove_free_block_from_list(sf_block *bp){
    if(GET_ALLOC_BIT(bp->header))
        return;
    sf_block *temp_next = bp->body.links.next;
    sf_block *temp_prev = bp->body.links.prev;

    temp_prev->body.links.next = temp_next;
    temp_next->body.links.prev = temp_prev;

}

void change_size_of_block(sf_header *block_header, size_t asize){
    size_t payload_size = GET_PAYLOAD_SIZE(*block_header);
    size_t alloc = GET_ALLOC_BIT(*block_header);
    size_t prev_alloc = GET_PREV_ALLOC(*block_header);
    sf_header new_header = payload_size << 32;
    new_header |= asize;
    new_header |= alloc ? 0x8 : 0;
    new_header |= prev_alloc ? 0x4 : 0;
    *block_header = new_header;
}

void change_size_of_payload(sf_header *block_header, size_t payload_size){
    size_t block_size = GET_BLOCK_SIZE(*block_header);
    size_t alloc = GET_ALLOC_BIT(*block_header);
    size_t prev_alloc = GET_PREV_ALLOC(*block_header);
    sf_header new_header = block_size;
    new_header |= payload_size << 32;
    new_header |= alloc ? 0x8 : 0;
    new_header |= prev_alloc ? 0x4 : 0;
    *block_header = new_header;
}



void *split(void *target_block, size_t asize, size_t payload_size){
    sf_header *target_block_header_addr = &((sf_block *)target_block)->header;
    sf_header target_block_header = *target_block_header_addr;
    size_t target_block_size = GET_BLOCK_SIZE(target_block_header);
    size_t new_block_size = target_block_size - asize;


    int check_wilderness = (target_block + target_block_size) == sf_mem_end() - 16;

    remove_free_block_from_list(target_block);

    change_size_of_block(target_block_header_addr, asize);
    *target_block_header_addr |= 0x8;
    sf_footer *target_block_footer_addr = target_block + asize;
    *target_block_footer_addr = *target_block_header_addr;

    sf_block *new_block = target_block + asize;           /* Splited new free block  */
    new_block->prev_footer = *target_block_header_addr;
    new_block->header = new_block_size;
    new_block->header |= 0x4;
    sf_footer *new_block_footer = (void *)new_block + new_block_size;
    *new_block_footer = (sf_footer)(new_block->header);

    add_new_free_block(new_block, check_wilderness ? 9 : find_min_index(new_block->header));

    change_size_of_payload(&(((sf_block *)target_block)->header), payload_size);
    return ((sf_block *)target_block)->body.payload;
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC_BIT(((sf_block *)bp)->prev_footer);
    size_t size = GET_BLOCK_SIZE(((sf_block *)bp)->header);
    sf_block *next_block = bp + size;
    size_t next_alloc = GET_ALLOC_BIT(next_block->header);

    size_t prev_block_size = GET_BLOCK_SIZE(((sf_block *)bp)->prev_footer);
    sf_block *prev_block = bp - prev_block_size;

    if(prev_alloc && next_alloc)
        return bp;

    else if(prev_alloc && !next_alloc) {
        remove_free_block_from_list(next_block);
        // sf_show_heap();

        size += GET_BLOCK_SIZE(next_block->header);
        change_size_of_block(&(((sf_block *)bp)->header), size);
        sf_footer *new_footer = bp + size;
        *new_footer = (sf_footer)(((sf_block *)bp)->header);

    }
    else if(!prev_alloc && next_alloc) {
        remove_free_block_from_list(prev_block);

        size += GET_BLOCK_SIZE(prev_block->header);
        sf_footer *bp_footer = bp + GET_BLOCK_SIZE(((sf_block *)bp)->header);
        change_size_of_block(bp_footer, size);
        prev_block->header = *bp_footer;

        bp = prev_block;
    }
    else {
        // remove_free_block_from_list(bp);
        remove_free_block_from_list(prev_block);
        remove_free_block_from_list(next_block);

        size += GET_BLOCK_SIZE(prev_block->header) + GET_BLOCK_SIZE(next_block->header);
        change_size_of_block(&(prev_block->header), size);
        sf_footer *next_block_footer = (sf_footer *)(next_block + GET_BLOCK_SIZE(next_block->header));
        *next_block_footer = prev_block->header;

        bp = prev_block;
    }

    return bp;
}

static void *find_fit(size_t asize, size_t payload_size){

    int index_of_min_size = find_min_index(asize);
    void *bp;

    if(sf_mem_start() == sf_mem_end()){                     /* Check if the heap is empty */
        init_sentinels();

        void *new_page_pt = sf_mem_grow();
        if(new_page_pt == NULL)
            return NULL;

        sf_block *prologue = sf_mem_start();
        prologue->header = 32;
        prologue->header |= 0x8;

        sf_block *first_heap_block = sf_mem_start() + 32;   /* Start from footer of prologue */
        first_heap_block->prev_footer = (sf_footer)(prologue->header);   /* Footer is identical to its header */
        size_t first_block_size = PAGE_SZ - 8 - 32 - 8;
        first_heap_block->header |= first_block_size;
        first_heap_block->header |= 0x4;

        sf_block *epilogue = sf_mem_end() - 16;
        epilogue->prev_footer = (sf_footer)(first_heap_block->header);
        epilogue->header |= 0x8;
        // printf("%s\n", "After first heap initializing");
        // sf_show_heap();         // Show heap right after the intitialization

        int index_of_first_heap_block = NUM_FREE_LISTS-1;                   // Put wilderness block in free list
        sf_free_list_heads[index_of_first_heap_block].body.links.next = first_heap_block;
        sf_free_list_heads[index_of_first_heap_block].body.links.prev = first_heap_block;
        first_heap_block->body.links.next = &sf_free_list_heads[index_of_first_heap_block];
        first_heap_block->body.links.prev = &sf_free_list_heads[index_of_first_heap_block];

        if(first_block_size >= asize){                      /* split */
            if((first_block_size - asize) < 32){            /* Can't split because of splinters */
                // Internal fragment
                remove_free_block_from_list(first_heap_block);
                first_heap_block->header |= 0x8;            /* Set Alloc */
                first_heap_block->header |= payload_size << 32;     /* Set payload_size */
                bp = (void *)(first_heap_block->body.payload);
                // printf("%s\n", "Allocated block with the internal fragment");
                // sf_show_heap();
            }
            else{
                bp = split(first_heap_block, asize, payload_size);        /* split a block and put new splited free block into list and return pp of block */
                // printf("%s\n", "Allocated block after spliting");
                // sf_show_heap();
            }

        }else{
            // required more allocation
            // printf("%s\n", "The required block size is greater than current wilderness block\nAllocate new page");
            goto ALLOCATE_NEW_PAGE;

        }

        return bp;

    }
    else                            /* Heap exists */
    {
        SEARCH_TROUGH_FREE_LIST:
        for(int i = index_of_min_size; i < NUM_FREE_LISTS; i++){
            void *sentinel = &sf_free_list_heads[i];
            void *cursor = ((sf_block *)sentinel)->body.links.next;
            while(cursor != sentinel){
                cursor = ((sf_block *)cursor)->body.links.next;
            }
            if((cursor = ((sf_block *)cursor)->body.links.prev) != sentinel){
                // Non-sential free block is found
                size_t cursor_size = GET_BLOCK_SIZE(((sf_block *)cursor)->header);
                if(cursor_size >= asize){
                    if((cursor_size - asize) >= 32){
                        bp = split(cursor, asize, payload_size);
                        // printf("%s\n", "Allocated block after spliting");
                        return bp;
                    }else{
                        remove_free_block_from_list((sf_block *)cursor);
                        ((sf_block *)cursor)->header |= payload_size << 32;
                        ((sf_block *)cursor)->header |= 0x8;
                        // printf("%s\n", "Allocated block with the internal fragment");
                        return ((sf_block *)cursor)->body.payload;
                    }
                }else{
                    goto ALLOCATE_NEW_PAGE;
                }
            }
        }

        ALLOCATE_NEW_PAGE:
        //allocate new page and coalselate free block and search again
        void *new_page_pt = sf_mem_grow();
        if(new_page_pt == NULL){
            sf_errno = ENOMEM;
            return NULL;
        }

        sf_block *prev_epilogue = new_page_pt - 16;

        size_t prev_alloc = GET_PREV_ALLOC(prev_epilogue->header);
        prev_epilogue->header = 0;
        prev_epilogue->header |= PAGE_SZ;
        prev_epilogue->header |= prev_alloc;

        sf_block *new_epilogue = sf_mem_end() - 16;
        new_epilogue->prev_footer = (sf_footer)prev_epilogue->header;
        new_epilogue->header |= 0x8;
        // sf_show_heap();
        void *new_block = coalesce(prev_epilogue);
        add_new_free_block(new_block, NUM_FREE_LISTS-1);
        // sf_show_heap();


        goto SEARCH_TROUGH_FREE_LIST;
    }


    return NULL; /* No fit */
}

size_t find_required_block_size(size_t size){
    size_t required_block_size;
    if((size + 16) <= sizeof(sf_block))
        required_block_size = sizeof(sf_block);
    else{
        size_t temp = (16 - (size % 16)) == 16 ? 0 : (16 - (size % 16));
        required_block_size = size + 16 + temp;
    }
    return required_block_size;
}


void *sf_malloc(size_t size) {

    size_t required_block_size = find_required_block_size(size);       /* Adjusted block size */
    sf_block *bp;

    /* Ignore spurious requests */
    if(size == 0)   return NULL;

    // printf("Adjusted block size: %ld\n", required_block_size);

    total_payload += (int)size;
    total_allocated_block += (int)required_block_size;

    /* Search the free list for a fit */
    if((bp = find_fit(required_block_size, size)) != NULL) {
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    if((bp = sf_mem_grow()) == NULL)
        return NULL;

    return bp;

}


void sf_free(void *pp) {

    // printf("%s\n", "heap before free");
    // sf_show_heap();
    sf_block *block_pointer = pp - 16;
    sf_header *block_header_addr = &(block_pointer->header);
    size_t payload_size = GET_PAYLOAD_SIZE(*block_header_addr);
    size_t block_size = GET_BLOCK_SIZE(*block_header_addr);
    size_t prev_alloc = GET_PREV_ALLOC(*block_header_addr);
    sf_footer *block_footer_addr = (void *)block_pointer + block_size;

    size_t prev_block_size = GET_BLOCK_SIZE(block_pointer->prev_footer);
    sf_block *prev_block = block_pointer - prev_block_size;


    if(pp == NULL)  abort();
    if((uintptr_t)pp % 16 != 0) abort();
    if(GET_BLOCK_SIZE(block_pointer->header) < 32)  abort();
    if(GET_BLOCK_SIZE(block_pointer->header) % 16 != 0) abort();
    if((void *)block_header_addr < (sf_mem_start()+40) && (void *)block_footer_addr > (sf_mem_end()-8))  abort();
    if(GET_ALLOC_BIT(*block_header_addr) == 0)  abort();
    if(GET_PREV_ALLOC(*block_header_addr) == 0 && GET_ALLOC_BIT(prev_block->header) != 0)   abort();

    block_pointer->header = 0;
    block_pointer->header |= block_size;
    block_pointer->header |= prev_alloc ? 0x4 : 0;

    *block_footer_addr = (sf_footer)(block_pointer->header);

    void *new_bp = coalesce(block_pointer);
    int wflag = is_wilderness(new_bp);

    total_payload -= (int)payload_size;
    total_allocated_block -= (int)block_size;

    add_new_free_block(new_bp, wflag ? NUM_FREE_LISTS-1 : find_min_index(((sf_block *)new_bp)->header));
    // printf("%s\n", "heap after free");

}


void *sf_realloc(void *pp, size_t rsize) {
    // printf("%s\n", "heap before realloc");
    // sf_show_heap();
    sf_block *block_pointer = pp - 16;
    sf_header *block_header_addr = &(block_pointer->header);
    size_t block_size = GET_BLOCK_SIZE(*block_header_addr);
    size_t payload_size = GET_PAYLOAD_SIZE(*block_header_addr);
    sf_footer *block_footer_addr = (void *)block_pointer + block_size;

    size_t prev_block_size = GET_BLOCK_SIZE(block_pointer->prev_footer);
    sf_block *prev_block = block_pointer - prev_block_size;

    if(pp == NULL)  abort();
    if((uintptr_t)pp % 16 != 0) abort();
    if(GET_BLOCK_SIZE(block_pointer->header) < 32)  abort();
    if(GET_BLOCK_SIZE(block_pointer->header) % 16 != 0) abort();
    if((void *)block_header_addr < (sf_mem_start()+40) && (void *)block_footer_addr > (sf_mem_end()-8))  abort();
    if(GET_ALLOC_BIT(*block_header_addr) == 0)  abort();
    if(GET_PREV_ALLOC(*block_header_addr) == 0 && GET_ALLOC_BIT(prev_block->header) != 0)   abort();

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    size_t realloc_block_size = find_required_block_size(rsize);
    if(realloc_block_size == block_size)
        return pp;
    else if(realloc_block_size > block_size){
        void *new_block_payload = sf_malloc(rsize);
        memcpy(new_block_payload,pp,block_size);
        sf_free(pp);

        total_payload += ((int)rsize - (int)payload_size);
        total_allocated_block += ((int)realloc_block_size - (int)block_size);

        return new_block_payload;
    }
    else {
        size_t sub = block_size - realloc_block_size;
        if(sub < sizeof(sf_block)){
            change_size_of_payload(&(block_pointer->header), rsize);
            *block_footer_addr = block_pointer->header;

            total_payload -= (payload_size - rsize);
            return pp;
        }else{
            pp = split(block_pointer, realloc_block_size, rsize);
            void *temp_addr = (void *)((size_t)block_pointer + realloc_block_size);
            coalesce(temp_addr);

            total_allocated_block -= (int)sub;
            total_payload -= ((int)payload_size - (int)rsize);
            return pp;
        }
    }
}


double sf_fragmentation() {
    if(sf_mem_start() == sf_mem_end())
        return 0.0;
    return (double)total_payload/total_allocated_block;
}

double sf_utilization() {
    if(sf_mem_start() == sf_mem_end())
        return 0.0;
    int heap_size = sf_mem_end() - sf_mem_start();
    return (double)total_payload/heap_size;
}
