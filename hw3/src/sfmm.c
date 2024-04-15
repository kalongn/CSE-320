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

// header is 8 bytes.
#define BOUNDRY_TAG_SIZE 8
// align blocks in 16 bytes fashion.
#define ALLIGNMENT 16
// header 8 bytes, prev pointer 8 bytes, next pointer 8 bytes, footer 8 bytes.
#define MIN_BLOCK_SIZE 32
// Maximum size for quick_list 32 + 9 * 16
#define MAX_QUICK_LIST_BLOCK_SIZE 32 + (NUM_QUICK_LISTS - 1) * ALLIGNMENT
// lower 4 bits on
#define BINARY_LOWER_FOUR_ON 0xf

size_t real_heap_usage = 0, max = 0;

// --------------------------- START OF HELPER FUNCTIONS --------------------------- //

/**
 * @brief it xor the size_t number with the MAGIC number
 *
 * @param number
 *      the header / footer content most likely
 * @return size_t
 *      the xor with magic number version of the content
 */
static size_t xor_magic(size_t number) {
    return number ^ MAGIC;
}

/**
 * @brief round up to the nearest multiple of ALLIGNMENT MACRO (16 in this HW) given a size_t size input.
 * If the input size is less then 32 (MIN_BLOCK_SIZE) however, it will round up to 32 (MIN_BLOCK_SIZE) automatically
 *
 * @param size
 *      the size input to be round up.
 * @return size_t
 *      the rounded up size (alligned with the ALLIGNMENT MACRO (16 in this HW))
 */
size_t r_u_m16(size_t size) {
    if (size < MIN_BLOCK_SIZE) {
        return MIN_BLOCK_SIZE;
    }
    size_t remainder = size % ALLIGNMENT;
    if (remainder == 0) {
        return size;
    }
    size = size + ALLIGNMENT - remainder;
    return size;
}

/**
 * @brief Validate a boundary tag with our boundary tag structures.
 *
 * @param boundary_tag
 *      the pointer to a boundry tag.
 * @return int
 *      0 -> a "good" boundary tag, -1 -> boundary tag went wrong.
 */
int is_invalid_bt(size_t *boundary_tag) {
    *boundary_tag = xor_magic(*boundary_tag);
    size_t size = *boundary_tag & (~BINARY_LOWER_FOUR_ON);
    // means the boundry_tag size is smaller then 32, which is impossible.
    if (size < MIN_BLOCK_SIZE) {
        return -1;
    }
    // means the boundry_tag size is not a multiple of 16, which is impossible
    if (size % ALLIGNMENT != 0) {
        return -1;
    }
    *boundary_tag = xor_magic(*boundary_tag);
    return 0;
}

/**
 * @brief Update one of the boundry tag given all the param that can alter the content.
 *
 * @param update_bt
 *      the pointer to a update_bt
 * @param size
 *      the size of the block which the update_bt belongs to
 * @param prev_alloc
 *      0 -> prev block not allocated, 1 -> prev block allocated
 * @param alloc
 *      0 -> current block not allocated, 1 -> current block allocated
 * @param in_quick
 *      0 -> not in quick list, 1 -> in quick list
 */
void update_bt(size_t *update_bt, size_t size, char prev_alloc, char alloc, char in_quick) {
    *update_bt = xor_magic(*update_bt);
    *update_bt &= BINARY_LOWER_FOUR_ON;
    if (prev_alloc) {
        *update_bt |= PREV_BLOCK_ALLOCATED;
    } else {
        *update_bt &= (~PREV_BLOCK_ALLOCATED);
    }
    if (alloc) {
        *update_bt |= THIS_BLOCK_ALLOCATED;
    } else {
        *update_bt &= (~THIS_BLOCK_ALLOCATED);
    }
    if (in_quick) {
        *update_bt |= IN_QUICK_LIST;
    } else {
        *update_bt &= (~IN_QUICK_LIST);
    }
    *update_bt |= size;
    *update_bt = xor_magic(*update_bt);
}

/**
 * @brief return the size field of a header / footer field
 *
 * @param boundary_tag
 *      the value which is stored within a header / footer field
 * @return size_t
 *      the size stored within those value.
 */
static size_t block_size_bt(size_t boundary_tag) {
    return xor_magic(boundary_tag) & (~BINARY_LOWER_FOUR_ON);
}

/**
 * @brief return the status of PREV_BLOCK_ALLOCATED BIT
 *
 * @param boundary_tag
 *      the content in a boundary tag.
 * @return char
 *      0 -> no, any other number = yes.
 */
char is_prev_alloc(size_t boundary_tag) {
    return xor_magic(boundary_tag) & PREV_BLOCK_ALLOCATED;
}

/**
 * @brief return the status of THIS_BLOCK_ALLOCATED BIT
 *
 * @param boundary_tag
 *      the content in a boundary tag.
 * @return char
 *      0 -> no, any other number = yes.
 */
char is_current_alloc(size_t boundary_tag) {
    return xor_magic(boundary_tag) & THIS_BLOCK_ALLOCATED;
}

/**
 * @brief return the status of IN_QUICK_LIST BIT
 *
 * @param boundary_tag
 *      the content in a boundary tag.
 * @return char
 *      0 -> no, any other number = yes.
 */
char is_in_quick(size_t boundary_tag) {
    return xor_magic(boundary_tag) & IN_QUICK_LIST;
}

/**
 * @brief Init the free list
 *
 */
void init_free_list() {
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
    }
}

/**
 * @brief Init the quick list
 *
 */
void init_quick_list() {
    for (int i = 0; i < NUM_QUICK_LISTS; i++) {
        sf_quick_lists[i].length = 0;
    }
}

/**
 * @brief Validate a free block given a footer. (for block that should be coel)
 *
 * @param footer
 *      the address of the footer.
 * @return int
 *      0 -> free block is valid, -1 -> free block is invalid
 */
int is_invalid_free_list_free_block_footer(sf_footer *footer) {
    sf_footer content = xor_magic(*footer);
    size_t size = content & (~BINARY_LOWER_FOUR_ON);
    // means the boundry_tag size is smaller then 32, which is impossible.
    if (size < MIN_BLOCK_SIZE) {
        return -1;
    }
    // means the boundry_tag size is not a multiple of 16, which is impossible
    if (size % ALLIGNMENT != 0) {
        return -1;
    }
    // if is allocated
    if (content & THIS_BLOCK_ALLOCATED) {
        return -1;
    }
    // if it is a free_block, 
    sf_header *header = (void *)footer - size + BOUNDRY_TAG_SIZE;
    if (content != xor_magic(*header)) {
        return -1;
    }
    return 0;
}

int is_invalid_pointer_address(void *address) {
    if (address == NULL) {
        fprintf(stderr, "ERROR: pointer provide is invalid as it is NULL\n");
        return -1;
    }
    // the pp is the payload address, -8 to get to the header
    sf_header *header_of_block = address - BOUNDRY_TAG_SIZE;
    // if address of the header of the block we're attemping to free is below our heap starting address
    if ((void *)header_of_block < sf_mem_start() + BOUNDRY_TAG_SIZE + MIN_BLOCK_SIZE) {
        fprintf(stderr, "ERROR: pointer provide is Invalid as its address is smaller then heap beginning address.\n");
        return -1;
    }
    // if address of the header is larger then the heap address space
    if ((void *)header_of_block > sf_mem_end() - BOUNDRY_TAG_SIZE - MIN_BLOCK_SIZE) {
        fprintf(stderr, "ERROR: pointer provide is Invalid as its address is larger then heap ending address.\n");
        return -1;
    }
    // address multiple of 16, size >= 32
    if (is_invalid_bt(header_of_block)) {
        fprintf(stderr, "ERROR: pointer value indicated either it is size less then 32, not a multiple of 16.\n");
        return -1;
    }
    // the block is not allocated.
    if (!is_current_alloc(*header_of_block)) {
        fprintf(stderr, "ERROR: pointer provide is Invalid as it is not an allocated block.\n");
        return -1;
    }
    if (is_current_alloc(*header_of_block) && is_in_quick(*header_of_block)) {
        fprintf(stderr, "ERROR: pointer provide is Invalid as it is in the quicklist.\n");
        return -1;
    }
    // the prev_block indicate by current header is free, but reading prev_block footer it is not free.
    if (!(xor_magic(*header_of_block) & PREV_BLOCK_ALLOCATED)) {
        sf_footer *footer_of_prev_block = (void *)header_of_block - BOUNDRY_TAG_SIZE;
        if (is_invalid_free_list_free_block_footer(footer_of_prev_block)) {
            fprintf(stderr, "ERROR: current block header indiciated prev block is free but it is not.\n");
            return -1;
        }
    }
    return 0;
}

// ---------------------------  END OF HELPER FUNCTIONS  --------------------------- //

// ---------------------------  START OF MALLOC HELPER   --------------------------- //

/**
 * @brief Determine the index of the main_list we should start searching from.
 *
 * @param size
 *      the size of the element that we want to allocate for.
 * @return int
 *      an number from [0,9] to indicate where we need to start searching from.
 */
int determine_free_list_index(size_t size) {
    int main_list_index = 0, main_list_size = MIN_BLOCK_SIZE;
    while (size > main_list_size && main_list_index < NUM_FREE_LISTS - 1) {
        main_list_size *= 2;
        main_list_index++;
    }
    return main_list_index;
}

/**
 * @brief remove the current block from the free_list (ASSUMED THE BLOCK IS IN FREE_LIST).s
 *
 * @param block
 *      the pointer address to the block
 */
void remove_block_from_free_list(sf_block *block) {
    block->body.links.prev->body.links.next = block->body.links.next;
    block->body.links.next->body.links.prev = block->body.links.prev;
}

/**
 * @brief Insert a block into free_list (assuming the header and footer is done correctly)
 *
 * @param new_block
 *      the pointer to the new block.
 * @param new_block_size
 *      the block_Size of this block is supposed to be.
 */
void insert_block_to_free_list(sf_block *new_block, size_t new_block_size) {
    int insert_index = determine_free_list_index(new_block_size);
    new_block->body.links.next = sf_free_list_heads[insert_index].body.links.next;
    sf_free_list_heads[insert_index].body.links.next->body.links.prev = new_block;
    sf_free_list_heads[insert_index].body.links.next = new_block;
    new_block->body.links.prev = &(sf_free_list_heads[insert_index]);
}

/**
 * @brief Update next block prev_alloc bit setup.
 *
 * @param current_header
 *      the current_block header.
 */
void update_prev_alloc_next_block(sf_footer *current_footer) {
    sf_header *header = (void *)current_footer + BOUNDRY_TAG_SIZE;
    *header = xor_magic(xor_magic(*header) & (~PREV_BLOCK_ALLOCATED));
    if (!is_current_alloc(*header)) {
        sf_footer *footer = (void *)header + block_size_bt(*header) - BOUNDRY_TAG_SIZE;
        *footer = *header;
    }
}

/**
 * @brief Allocate a block if not splitting is required, occured when the block size is
 * [exactly, exactly + MIN_BLOCK_SIZE <32 in this hw>) given "exactly" is the size requested.
 *
 * @param block
 *      the pointer to the block we're working with.
 * @return void*
 *      the pointer to the space avaliable to use (skipping over the header).
 */
void *allocate_block_no_split(sf_block *block) {
    size_t size = block_size_bt(block->header);
    real_heap_usage += (size - BOUNDRY_TAG_SIZE);
    debug("no split allocation addition: prev_real_size: %lu, addition: %lu, now_real: %lu\n", real_heap_usage - (size - BOUNDRY_TAG_SIZE), (size - BOUNDRY_TAG_SIZE), real_heap_usage);
    max = real_heap_usage > max ? real_heap_usage : max;
    sf_header *header_of_block_to_allocate = &(block->header);
    char is_prev = is_prev_alloc(*header_of_block_to_allocate);
    update_bt(header_of_block_to_allocate, size, is_prev, 1, 0);
    sf_header *header_of_next_block = (void *)&(block->header) + size;
    size_t size_of_next_block = block_size_bt(*header_of_next_block);
    *header_of_next_block = xor_magic(xor_magic(*header_of_next_block) | PREV_BLOCK_ALLOCATED);
    if (is_current_alloc(*header_of_next_block)) {
        sf_footer *footer_of_next_block = (void *)header_of_next_block + size_of_next_block - BOUNDRY_TAG_SIZE;
        *footer_of_next_block = *header_of_next_block;
    }
    return (void *)header_of_block_to_allocate + BOUNDRY_TAG_SIZE;
}

/**s
 * @brief Allocate a block if splitting is required.
 *
 * @param block
 *      the pointer to the block we're going to split
 * @param required_size
 *      the size required by the current allocation request
 * @return void*
 *      the pointer to the space avaliable to use (skipping over the header). NULL = failure.
 */
void *allocate_block_with_splitting(sf_block *block, size_t required_size) {
    // setting up the gona be allocated block.
    size_t size = block_size_bt(block->header);
    sf_header *header_of_block_to_allocate = &(block->header);
    char is_prev = is_prev_alloc(*header_of_block_to_allocate);
    real_heap_usage += (required_size - BOUNDRY_TAG_SIZE);
    debug("split allocation addition: prev_real_size: %lu, addition: %lu, now_real: %lu\n", real_heap_usage - (required_size - BOUNDRY_TAG_SIZE), (required_size - BOUNDRY_TAG_SIZE), real_heap_usage);
    max = real_heap_usage > max ? real_heap_usage : max;
    update_bt(header_of_block_to_allocate, required_size, is_prev, 1, 0);

    // set-up the split block
    sf_block *splitted_block = (void *)header_of_block_to_allocate + required_size - BOUNDRY_TAG_SIZE;
    sf_header splitted_block_header = 0x0;
    size_t splitted_block_size = size - required_size;
    update_bt(&splitted_block_header, splitted_block_size, 1, 0, 0);
    splitted_block->header = splitted_block_header;
    sf_footer *footer_of_splitted_block = (void *)&(splitted_block->header) + splitted_block_size - BOUNDRY_TAG_SIZE;
    update_bt(footer_of_splitted_block, splitted_block_size, 1, 0, 0);

    // remove the block from free_list
    remove_block_from_free_list(block);

    // insert the split block back to free_list
    insert_block_to_free_list(splitted_block, splitted_block_size);
    return (void *)header_of_block_to_allocate + BOUNDRY_TAG_SIZE;
}

/**
 * @brief Search main free_list for memory and attempt to allocate if possible.
 *
 * @param pointer
 *      the pointer to the address of the pointer we should update if a allocation is possible.
 * @param isAllocated
 *      the pointer to a status check to indicate whether the program should continue.
 * @param size
 *      the size of the thing we're trying to allocate
 * @return int
 *      0 = success, -1 = fail to find a space
 */
int search_heap_and_allocate(void **pointer, char *isAllocated, size_t size) {
    int main_list_index = determine_free_list_index(size);
    for (int i = main_list_index; i < NUM_FREE_LISTS && !*isAllocated; i++) {
        if (sf_free_list_heads[i].body.links.next == &sf_free_list_heads[i]) {
            continue;
        }
        sf_block *dummy_header = &sf_free_list_heads[i];
        sf_block *traverse = sf_free_list_heads[i].body.links.next;
        while (!*isAllocated && traverse != dummy_header) {
            size_t traverse_block_size = block_size_bt(traverse->header);
            if (traverse_block_size - MIN_BLOCK_SIZE >= size) {
                remove_block_from_free_list(traverse);
                *pointer = allocate_block_with_splitting(traverse, size);
                *isAllocated = 1;
                return 0;
            } else if (traverse_block_size >= size) {
                remove_block_from_free_list(traverse);
                *isAllocated = 1;
                *pointer = allocate_block_no_split(traverse);
                return 0;
            } else {
                traverse = traverse->body.links.next;
            }
        }
    }
    return -1;
}

/**
 * @brief a special coalesce used when we need more memory.
 *
 * @param prev_block_header
 *      the pointer to the prev_block_header
 * @param current_block_footer
 *      the pointer to the current_block_footer
 */
void special_coalesce_new_mem(sf_header *prev_block_header, sf_footer *current_block_footer) {
    char is_prev_prev = is_prev_alloc(*prev_block_header);
    size_t size_of_prev = block_size_bt(*prev_block_header);
    size_t new_size = size_of_prev + block_size_bt(*current_block_footer);
    update_bt(prev_block_header, new_size, is_prev_prev, 0, 0);
    update_bt(current_block_footer, new_size, is_prev_prev, 0, 0);

    sf_block *block = (void *)prev_block_header - BOUNDRY_TAG_SIZE;
    remove_block_from_free_list(block);
    insert_block_to_free_list(block, new_size);
}

/**
 * @brief Attempt to coal the curent block given the header with the prev and next block if possible
 *
 * @param current_block_header
 *      the header of the current block
 */
void coalesce_current(sf_header *current_block_header) {
    size_t current_block_size = block_size_bt(*current_block_header);
    sf_footer *current_block_footer = (void *)current_block_header + current_block_size - BOUNDRY_TAG_SIZE;
    // check for next_block allocation status
    sf_header *next_block_header = (void *)current_block_footer + BOUNDRY_TAG_SIZE;
    if (!is_current_alloc(*next_block_header) && (void *)next_block_header < sf_mem_end() - BOUNDRY_TAG_SIZE) {
        size_t next_block_size = block_size_bt(*next_block_header);
        sf_footer *next_block_footer = (void *)next_block_header + next_block_size - BOUNDRY_TAG_SIZE;
        // update the header and footer of the "new" coel block.
        size_t new_block_size = current_block_size + next_block_size;
        update_bt(current_block_header, new_block_size, is_prev_alloc(*current_block_header), 0, 0);
        *next_block_footer = *current_block_header;
        // remove both blocks from free_list
        sf_block *block = (void *)current_block_header - BOUNDRY_TAG_SIZE;
        remove_block_from_free_list(block);

        block = (void *)next_block_header - BOUNDRY_TAG_SIZE;
        remove_block_from_free_list(block);

        block = (void *)current_block_header - BOUNDRY_TAG_SIZE;
        // add the new block to the free_list
        insert_block_to_free_list(block, new_block_size);

        // added such that current_block is the new coal block if an attempt to coal with prev block is made
        current_block_size = new_block_size;
        current_block_footer = next_block_footer;
    }
    // prev_block coal
    if (!is_prev_alloc(*current_block_header) && (void *)current_block_header > sf_mem_start() + BOUNDRY_TAG_SIZE + MIN_BLOCK_SIZE) {
        //verify block structure
        sf_footer *prev_block_footer = (void *)current_block_header - BOUNDRY_TAG_SIZE;
        size_t prev_block_size = block_size_bt(*prev_block_footer);
        sf_header *prev_block_header = (void *)prev_block_footer - prev_block_size + BOUNDRY_TAG_SIZE;
        // update the header and footer of the "new" coel block.
        size_t new_block_size = current_block_size + prev_block_size;
        update_bt(prev_block_header, new_block_size, is_prev_alloc(*prev_block_header), 0, 0);
        *current_block_footer = *prev_block_header;

        // remove both blocks from free_list
        sf_block *block = (void *)current_block_header - BOUNDRY_TAG_SIZE;
        remove_block_from_free_list(block);

        block = (void *)prev_block_header - BOUNDRY_TAG_SIZE;
        remove_block_from_free_list(block);

        // add the new block to the free_list
        block = (void *)prev_block_header - BOUNDRY_TAG_SIZE;
        insert_block_to_free_list(block, new_block_size);
    }
    // so no coal attempt made or coal attempts sucess.
}

// ---------------------------   END OF MALLOC HELPER    --------------------------- //

// ---------------------------  START OF sf_malloc CODE  --------------------------- //

void *sf_malloc(size_t size) {
    // the return address variable
    void *new_begin_mem_pointer = 0x0;
    // means the heap is nost created yet
    if (sf_mem_start() == sf_mem_end()) {
        new_begin_mem_pointer = sf_mem_grow();
        if (new_begin_mem_pointer == NULL) {
            fprintf(stderr, "ERROR: Ask for more memory from kernel but does not recieve any.\n");
            sf_errno = ENOMEM;
            return NULL;
        }
        // get pass Unused block;
        new_begin_mem_pointer += BOUNDRY_TAG_SIZE;
        // Provide correct bit information to prologue;
        sf_header *prologue = new_begin_mem_pointer;
        update_bt(prologue, MIN_BLOCK_SIZE, 1, 1, 0);
        // get pass the prologue block;
        new_begin_mem_pointer += MIN_BLOCK_SIZE;
        init_free_list();
        init_quick_list();
        // setting up the first block
        sf_block *first_block = new_begin_mem_pointer - BOUNDRY_TAG_SIZE; // mins 8 as it needs to include the prev_footer
        // setup the header
        sf_header header = 0x0;
        size_t size_of_first = PAGE_SZ - MIN_BLOCK_SIZE - ALLIGNMENT;
        update_bt(&header, size_of_first, 1, 0, 0);
        first_block->header = header;
        // add this giant block at the 8th index of the list as it is (128M, 256M]
        insert_block_to_free_list(first_block, size_of_first);
        // footer is the current end of the heap - 8 (for the epilogue) then -8 again (for the beginning of footer)
        sf_footer *footer = sf_mem_end() - ALLIGNMENT;
        update_bt(footer, size_of_first, 1, 0, 0);
        // set up epilogue
        sf_footer *epilogue = sf_mem_end() - 8;
        update_bt(epilogue, 0, 0, 1, 0);
    }

    // size 0, return null and don't set anything
    if (size == 0) {
        return NULL;
    }

    // add the header to the size since we need to allocated for the header as well.
    size += BOUNDRY_TAG_SIZE;
    size = r_u_m16(size);

    char isAllocated = 0;

    // search through quicklist
    if (size <= MAX_QUICK_LIST_BLOCK_SIZE && (size - MIN_BLOCK_SIZE) % ALLIGNMENT == 0) {
        int quicklist_index = (size - MIN_BLOCK_SIZE) >> 4;
        if (quicklist_index < NUM_QUICK_LISTS && sf_quick_lists[quicklist_index].length != 0) {
            // obtain the first block in the quick list
            sf_block *block_to_allocate = sf_quick_lists[quicklist_index].first;
            sf_quick_lists[quicklist_index].first = block_to_allocate->body.links.next;
            sf_quick_lists[quicklist_index].length--;
            new_begin_mem_pointer = allocate_block_no_split(block_to_allocate);
            // updating the link from the dummy node to "remove" the now allocated block
            isAllocated = 1;
        }
    }

    // allocate if page has enough memory, if not continue to allocate for page space.
    while (!isAllocated && search_heap_and_allocate(&new_begin_mem_pointer, &isAllocated, size) == -1) {
        sf_footer *prev_epilogue = sf_mem_end() - BOUNDRY_TAG_SIZE;
        void *additional_memory = sf_mem_grow();
        if (additional_memory == NULL) {
            fprintf(stderr, "ERROR: Ask for more memory from kernel but does not recieve any.\n");
            sf_errno = ENOMEM;
            return NULL;
        }

        char is_prev_taken = is_prev_alloc(*prev_epilogue);
        update_bt(prev_epilogue, PAGE_SZ, is_prev_taken, 0, 0);
        sf_footer *footer_of_new_block = (void *)prev_epilogue + PAGE_SZ - BOUNDRY_TAG_SIZE;
        update_bt(footer_of_new_block, PAGE_SZ, is_prev_taken, 0, 0);
        sf_footer *epilogue = sf_mem_end() - BOUNDRY_TAG_SIZE;
        update_bt(epilogue, 0, 0, 1, 0);

        sf_header *prev_block_header = 0x0;
        if (!is_prev_taken) {
            sf_footer *prev_block_footer = (void *)prev_epilogue - BOUNDRY_TAG_SIZE;
            if (is_invalid_bt(prev_block_footer) || is_current_alloc(*prev_block_footer)) {
                fprintf(stderr, "ERROR: previous block footer has been corrupted when attempt to coalescing\n");
                sf_errno = ENOMEM;
                return NULL;
            }
            size_t size_of_prev = block_size_bt(*prev_block_footer);
            prev_block_header = (void *)prev_epilogue - size_of_prev;
            if (xor_magic(*prev_block_header) != xor_magic(*prev_block_footer)) {
                fprintf(stderr, "ERROR: Coalescing has failed due to prev_block header does not contain appropriate value.\n");
                sf_errno = ENOMEM;
                return NULL;
            }
            special_coalesce_new_mem(prev_block_header, footer_of_new_block);
        } else {
            insert_block_to_free_list((void *)prev_epilogue - BOUNDRY_TAG_SIZE, PAGE_SZ);
        }
    }
    // sf_show_heap();
    return new_begin_mem_pointer;
}
// ---------------------------   END OF sf_malloc CODE   --------------------------- //

// ------------------------ START OF sf_malloc HELPER CODE ------------------------- //

/**
 * @brief Insert the block into quick_list (DOES NOT CHECK CONDITION)
 *
 * @param header
 *      header of the block.
 * @param index
 *      the index of the quick_list determined prior.
 * @param current_block_size
 *      the size of the current quick_list[index] (should be determine before calling this).
 */
void insert_block_to_quick_list(sf_header *header, int index, size_t current_block_size) {
    *header = xor_magic(xor_magic(*header) | IN_QUICK_LIST);
    real_heap_usage -= (current_block_size - BOUNDRY_TAG_SIZE);
    debug("insert_block_to_quick_list prev_real_size: %lu, subtraction: %lu, now_real: %lu\n", real_heap_usage + (current_block_size - BOUNDRY_TAG_SIZE), (current_block_size - BOUNDRY_TAG_SIZE), real_heap_usage);
    sf_block *block = (void *)header - BOUNDRY_TAG_SIZE;
    block->body.links.next = sf_quick_lists[index].first;
    sf_quick_lists[index].first = block;
    sf_quick_lists[index].length++;
}

// ------------------------- END OF sf_malloc HELPER CODE -------------------------- //

// ---------------------------   START OF sf_free CODE   --------------------------- //

void sf_free(void *pp) {
    if (is_invalid_pointer_address(pp)) {
        abort();
    }
    // the pp is the payload address, -8 to get to the header
    sf_header *header_of_block = pp - BOUNDRY_TAG_SIZE;

    // at this point address is valid.
    // we also know that if the bit of PREV_BLOCK_ALLOCATED is on this block
    // the previous block is also free

    // quick list insertion
    size_t current_block_size = block_size_bt(*header_of_block);
    int quick_list_index = -1;
    if (current_block_size <= MAX_QUICK_LIST_BLOCK_SIZE && (current_block_size - MIN_BLOCK_SIZE) % ALLIGNMENT == 0) {
        // flush condition
        quick_list_index = (current_block_size - MIN_BLOCK_SIZE) >> 4;
        if (sf_quick_lists[quick_list_index].length == QUICK_LIST_MAX) {
            for (int i = 0; i < QUICK_LIST_MAX; i++) {
                sf_block *cursor = sf_quick_lists[quick_list_index].first;
                sf_header *header = &(cursor->header);
                sf_quick_lists[quick_list_index].first = cursor->body.links.next;
                sf_footer *footer = (void *)header + current_block_size - BOUNDRY_TAG_SIZE;
                // make the bit that say they're in quick_list no longer in quick_list
                update_bt(header, current_block_size, is_prev_alloc(*header), 0, 0);
                *footer = *header;

                // update next block prev_allocate bit
                update_prev_alloc_next_block(footer);

                // add to the free_list
                sf_block *block = (void *)header - BOUNDRY_TAG_SIZE;
                insert_block_to_free_list(block, current_block_size);

                // coalesces
                coalesce_current(header);
            }
            sf_quick_lists[quick_list_index].first = NULL;
        }
        insert_block_to_quick_list(header_of_block, quick_list_index, current_block_size);
        return;
    }

    // get to this point means the free is not a quick_list dimension
    update_bt(header_of_block, current_block_size, is_prev_alloc(*header_of_block), 0, 0);
    sf_footer *footer_of_block = (void *)header_of_block + current_block_size - BOUNDRY_TAG_SIZE;
    *footer_of_block = *header_of_block;
    update_prev_alloc_next_block(footer_of_block);

    real_heap_usage -= (current_block_size - BOUNDRY_TAG_SIZE);
    debug("free normal subtraction: prev_real_size: %lu, subtraction: %lu, now_real: %lu\n", real_heap_usage + (current_block_size - BOUNDRY_TAG_SIZE), (current_block_size - BOUNDRY_TAG_SIZE), real_heap_usage);


    sf_block *block = (void *)header_of_block - BOUNDRY_TAG_SIZE;
    insert_block_to_free_list(block, current_block_size);
    coalesce_current(header_of_block);
    return;
}

// ---------------------------    END OF sf_free CODE    --------------------------- //

// --------------------------- START OF sf_realloc CODE  --------------------------- //

void *sf_realloc(void *pp, size_t rsize) {
    // sf_show_heap();
    if (is_invalid_pointer_address(pp)) {
        sf_errno = EINVAL;
        return NULL;
    }
    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    }
    // the pp is the payload address, -8 to get to the header
    sf_header *header_of_block = pp - BOUNDRY_TAG_SIZE;
    size_t actual_rsize = r_u_m16(rsize + BOUNDRY_TAG_SIZE);
    size_t size = block_size_bt(*header_of_block);
    if (actual_rsize > size) {
        void *new_address = sf_malloc(rsize);
        if (new_address == NULL) {
            fprintf(stderr, "ERROR: sf_realloc failed as sf_malloc failed to allocate memory for the new size.\n");
            return NULL;
        }
        memcpy(new_address, (void *)header_of_block + BOUNDRY_TAG_SIZE, size - BOUNDRY_TAG_SIZE);
        sf_free(pp);
        return new_address;
    }
    if (actual_rsize < size) {
        // since we can't split anyways
        if (actual_rsize + MIN_BLOCK_SIZE > size) {
            return pp;
        }
        update_bt(header_of_block, actual_rsize, is_prev_alloc(*header_of_block), 1, 0);
        sf_footer *new_footer_of_block = (void *)header_of_block + actual_rsize - BOUNDRY_TAG_SIZE;
        *new_footer_of_block = *header_of_block;
        sf_header *new_split_header = (void *)new_footer_of_block + BOUNDRY_TAG_SIZE;
        update_bt(new_split_header, size - actual_rsize, 1, 0, 0);
        sf_footer *new_split_footer = (void *)new_split_header + (size - actual_rsize) - BOUNDRY_TAG_SIZE;
        *new_split_footer = *new_split_header;
        real_heap_usage -= (size - BOUNDRY_TAG_SIZE);
        real_heap_usage += (actual_rsize - BOUNDRY_TAG_SIZE);
        debug("realloc subtraction: prev_real_size: %lu, subtraction: %lu, addition: %lu, now_real: %lu\n", real_heap_usage + (size - BOUNDRY_TAG_SIZE) - (actual_rsize - BOUNDRY_TAG_SIZE), (size - BOUNDRY_TAG_SIZE), (actual_rsize - BOUNDRY_TAG_SIZE), real_heap_usage);

        sf_block *block = (void *)new_split_header - BOUNDRY_TAG_SIZE;
        insert_block_to_free_list(block, size - actual_rsize);
        update_prev_alloc_next_block(new_split_footer);

        coalesce_current(new_split_header);
        return pp;
    }
    // if size = rsize
    return pp;
}
// ---------------------------   END OF sf_realloc CODE  --------------------------- //

double sf_fragmentation() {
    fprintf(stderr, "ERROR: This function doesn't work as there's not way to track internal payload size.");
    return 0.0;
}

double sf_utilization() {
    if (sf_mem_end() - sf_mem_start() == 0) {
        return 0.0;
    }
    debug("max when util is call:%lu \n", max);
    return (double)max / (double)(sf_mem_end() - sf_mem_start());
}
