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

#define PUT(payload_size, block_size, alloc, prvAlloc) \
    ((((size_t)(payload_size) & 0xFFFFFFFF) << 32) | \
     (((size_t)(block_size) & 0xFFFFFF) << 4) | \
     (((size_t)(alloc) & 0x1) << 3) | \
     ((size_t)(prvAlloc) & 0x1))

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



void *split(void *target_block, size_t asize){
    sf_header *target_block_header_addr = &((sf_block *)target_block)->header;
    sf_header target_block_header = *target_block_header_addr;
    if(GET_ALLOC_BIT(target_block_header) == 1) return NULL;                   /* If target block is not freed return NULL */
    size_t target_block_size = GET_BLOCK_SIZE(target_block_header);
    size_t new_block_size = target_block_size - asize;

    int check_wilderness = (target_block + target_block_size) == sf_mem_end() - 16;
    // void *epilogue = sf_mem_end() -16;
    // printf("%p\n", epilogue);
    remove_free_block_from_list(target_block);

    change_size_of_block(target_block_header_addr, asize);
    *target_block_header_addr |= 0x8;

    sf_block *new_block = target_block + asize;           /* Splited new free block  */
    new_block->prev_footer = (sf_footer)target_block_header;
    new_block->header = new_block_size;
    new_block->header |= 0x4;
    sf_footer *new_block_footer = (void *)new_block + new_block_size;
    *new_block_footer = (sf_footer)(new_block->header);

    add_new_free_block(new_block, check_wilderness ? 9 : find_min_index(new_block->header));

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
        remove_free_block_from_list(bp);
        remove_free_block_from_list(next_block);

        size += GET_BLOCK_SIZE(next_block->header);
        change_size_of_block(&(((sf_block *)bp)->header), size);
        sf_footer *new_footer = bp + size;
        *new_footer = (sf_footer)(((sf_block *)bp)->header);

        add_new_free_block(bp, find_min_index(((sf_block *)bp)->header));
    }
    else if(!prev_alloc && next_alloc) {
        remove_free_block_from_list(bp);
        remove_free_block_from_list(prev_block);

        size += GET_BLOCK_SIZE(prev_block->header);
        sf_footer *bp_footer = bp + GET_BLOCK_SIZE(((sf_block *)bp)->header);
        change_size_of_block(bp_footer, size);
        prev_block->header = *bp_footer;

        bp = prev_block;
    }
    else {
        remove_free_block_from_list(bp);
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
        // first_heap_block->header |= (uint64_t)payload_size << 32;    /* Page(4096) - unused row(8) - prologue(32) - epilogue(8) */
        size_t first_block_size = PAGE_SZ - 8 - 32 - 8;
        first_heap_block->header |= first_block_size;
        first_heap_block->header |= 0x4;

        sf_block *epilogue = sf_mem_end() - 16;
        epilogue->prev_footer = (sf_footer)(first_heap_block->header);
        epilogue->header |= 0x8;

        // sf_show_heap();         // Show heap right after the intitialization

        int index_of_first_heap_block = NUM_FREE_LISTS-1;
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
            }
            else{
                // sf_show_heap();
                bp = split(first_heap_block, asize);        /* split a block and put new splited free block into list and return pp of block */
                // sf_show_heap();
            }

        }else{
            // required more allocation
            goto ALLOCATE_NEW_PAGE;

        }

        return bp;

    }
    else                            /* Heap exists */
    {
        SEARCH_TROUGH_FREE_LIST:
        // sf_show_heap();
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
                        bp = split(cursor, asize);
                        return bp;
                    }else{
                        remove_free_block_from_list((sf_block *)cursor);

                        ((sf_block *)cursor)->header |= 0x8;
                        sf_show_heap();
                        return ((sf_block *)cursor)->body.payload;
                    }
                }else{
                    goto ALLOCATE_NEW_PAGE;
                }
                // return ((sf_block *)cursor)->body.payload;
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

        add_new_free_block(prev_epilogue, find_min_index(prev_epilogue->header));

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


void *sf_malloc(size_t size) {

    size_t required_block_size;       /* Adjusted block size */
    // size_t extendsize;  /* Amount to extend heap if no fit */
    sf_block *bp;

    /* Ignore spurious requests */
    if(size == 0)   return NULL;

    /* Adjust block size to include overhead and alignment reqs */
    if((size + 16) <= sizeof(sf_block))
        required_block_size = sizeof(sf_block);
    else{
        size_t temp = (16 - (size % 16)) == 16 ? 0 : (16 - (size % 16));
        required_block_size = size + 16 + temp;

    }

    printf("Adjusted block size: %ld\n", required_block_size);

    /* Search the free list for a fit */
    if((bp = find_fit(required_block_size, size)) != NULL) {
        // place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    if((bp = sf_mem_grow()) == NULL)
        return NULL;
    // place(bp, asize);
    return bp;

}


// static void *coalesce(void *pp){
//     size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(pp)));
//     size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(pp)));
//     size_t size = GET_SIZE(HDRP(pp));

//     if(prev_alloc && next_alloc) {                  /* Case 1*/
//         return pp;
//     }

//     else if(prev_alloc && !next_alloc) {            /* Case 2 */
//         size += GET_SIZE(HDRP(NEXT_BLKP(pp)));
//         PUT(HDRP(pp), PACK(size, 0));
//         PUT(FTRP(pp), PACK(size, 0));
//     }

//     else if(!prev_alloc && next_alloc) {            /* Case 3 */
//         size += GET_SIZE(HDRP(NEXT_BLKP(pp)));
//         PUT(FTRP(pp), PACK(size, 0));
//         PUT(HDRP(PREV_BLKP(pp)), PACK(size, 0));
//         pp = PREV_BLKP(pp);
//     }

//     else {                                          /* Case 4 */
//         size += GET_SIZE(HDRP(PREV_BLKP(pp))) + GET_SIZE(FTRP(NEXT_BLKP(pp)));
//         PUT(HDRP(PREV_BLKP(pp)), PACK(size, 0));
//         PUT(FTRP(NEXT_BLKP(pp)), PACK(size, 0));
//         pp = PREV_BLKP(pp);
//     }
//     return pp;
// }

void sf_free(void *pp) {
    // size_t size = GET_SIZE(HDRP(pp));

    // PUT(HDRP(pp), PACK(size, 0));
    // PUT(FTRP(pp), PACK(size, 0));
    // coalesce(pp);
    abort();
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented.
    abort();
}

double sf_fragmentation() {
    // To be implemented.
    abort();
}

double sf_utilization() {
    // To be implemented.
    abort();
}
