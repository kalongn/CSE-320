#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "huff.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the nodes of the Huffman tree and other data have
 * been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

// ----------------------------------- DEBUG METHOD -----------------------------------

/**
 * @brief Debugging method to print the tree in post-order fashion
 *
 * @param root
 *      the root of the tree, it should be arr[0]
 */
void print_huffman_tree_in_post_order(NODE *root) {
    if (root == NULL) {
        return;
    }
    print_huffman_tree_in_post_order(root->left);
    print_huffman_tree_in_post_order(root->right);
    printf("Symbol: %d, Weight: %d\n", root->symbol, root->weight);
}

/**
 * @brief debug function to print the weight of the nodes
 *
 */
void print_nodes_weight() {
    for (int i = 0; i < 2 * MAX_SYMBOLS - 1; i++) {
        printf("%d -> %d\n", i, (nodes + i)->weight);
    }
}

/**
 * @brief debug function to print the entire nodes array
 *
 */
void print_nodes_array() {
    for (int i = 0; i < 2 * MAX_SYMBOLS - 1; i++) {
        printf("%d -> Weight: %d, Symbol: %d, Left: %p, Right: %p, Parent: %p, Self: %p\n", i, (nodes + i)->weight, (nodes + i)->symbol, (nodes + i)->left, (nodes + i)->right, (nodes + i)->parent, (nodes + i));
    }
}

/**
 * @brief debug function to print the weight of the nodes given a length
 *
 * @param length
 *      the amount of NODES the function have
 */
void simplify_print_nodes_weight(int length) {
    printf("Length Input: %d\n", length);
    for (int i = 0; i < length; i++) {
        printf("%d -> %d\n", i, (nodes + i)->weight);
    }
}

// ----------------------------------- END DEBUG METHOD -----------------------------------

// ----------------------------------- HELPER METHOD -----------------------------------

/**
 * @brief determine the block size from global_options.
 *
 * @return int
 *      the block size.
 */
int determine_block_size_from_global() {
    return ((unsigned)global_options >> 16) + 0x00000001;
}

/**
 * @brief return the proper length of a tree base on the amount of leafs.
 *
 * @param leaf_amount
 *      amount of leafs required
 * @return int
 *      the amount of tree node will takes up as an integer.
 */
int total_tree_length_formula(int leaf_amount) {
    return 2 * leaf_amount - 1;
}

/**
 * @brief Empty the nodes_for_symbol by setting every pointer to a NULL value.
 * @details This function empties the nodes_for_symbol array by setting all of them to NULL. Essentially simulating a "local array"
 *
 */
void clear_nodes_for_symbols() {
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        *(node_for_symbol + i) = NULL;
    }
}

/**
 * @brief Swap the content of 2 Node
 *
 * @param i
 *      Node one input
 * @param j
 *      Noe two input
 */
void swap_nodes(int i, int j) {
    NODE temp = *(nodes + i);
    *(nodes + i) = *(nodes + j);
    *(nodes + j) = temp;
}

/**
 * @brief Set the all weight negative one
 * @details This function set the all weight negative one, useful when debugging to visiualize the array nodes()
 */
void set_all_weight_negative() {
    for (int i = 0; i < 2 * MAX_SYMBOLS - 1; i++) {
        (nodes + i)->weight = -1;
    }
}

/**
 * @brief ensure the entire current_block is back to the "uninitialised state" by setting everything within to '\0'.
 *
 */
void current_block_clean() {
    int i = 0;
    while (i < MAX_BLOCK_SIZE && *(current_block + i) != '\0') {
        *(current_block + i) = '\0';
        i++;
    }
}

/**
 * @brief Ensure the entire nodes is back to the "uninitialised state".
 * @details
 *      all short / char value will be set to -1, all pointer variable will be set to NULL.
 *
 */
void clear_nodes() {
    for (int i = 0; i < 2 * MAX_SYMBOLS - 1; i++) {
        (nodes + i)->weight = -1;
        (nodes + i)->symbol = -1;
        (nodes + i)->left = NULL;
        (nodes + i)->right = NULL;
        (nodes + i)->parent = NULL;
    }
}

// ----------------------------------- END HELPER METHOD -----------------------------------

// ----------------------------------- HUFFMAN COMPRESS METHOD -----------------------------------

/**
 * @brief Heapify the array given the beginning and end index.
 *
 * @param min_index
 *      the index that indicate where the current minimum value is at
 * @param start
 *      the beginning index of the array
 * @param end
 *      the end index of the array
 */
void heapify(int min_index, int start, int end) {
    int subtree_min_index = min_index;
    int left_index = 2 * min_index + 1;
    int right_index = 2 * min_index + 2;

    // if the Node in left index is greater then parent, we swap the current subtree min index to left.
    if (left_index < end && left_index >= start && (nodes + left_index)->weight < (nodes + min_index)->weight) {
        subtree_min_index = left_index;
    }

    // if the Node in right index is greater then parent, we swap the current subtree min index to right.
    if (right_index < end && right_index >= start && (nodes + right_index)->weight < (nodes + subtree_min_index)->weight) {
        subtree_min_index = right_index;
    }

    if (subtree_min_index != min_index) {
        swap_nodes(min_index, subtree_min_index);
        heapify(subtree_min_index, start, end);
    }
}

/**
 * @brief Begin heapify base on the amount of leaf (symbol) we have
 *
 * @param leaf_amount
 *      the leaf_amount is the amount of neaf stored in nodes (beginning from index 0)
 */
void construct_binary_heap(int leaf_amount) {
    int start_index = (leaf_amount / 2) - 1;
    for (int i = start_index; i >= 0; i--) {
        heapify(i, 0, leaf_amount);
    }
}

/**
 * @brief Shift the entire array to the left by 1, then reheapify the heap
 *
 * @param length
 *      the length of the current array need to be heapify.
 */
void shift_left_array(int length) {
    for (int i = 0; i < length; i++) {
        swap_nodes(i, i + 1);
    }
    construct_binary_heap(length);
}

/**
 * @brief Pull the minimum Node out of the nodes array, then reheapify the heap with its length subtracted by 1.
 *
 * @param heap_length
 *      the length of the current heap.
 * @param back_of_array
 *      the index of the back of the array (for placing the nodes into huffman tree)
 * @param back_offset
 *      the offset of the back of the array (for placing the nodes into huffman tree)
 * @return NODE*
 *      the pointer to node that were pulled out of the nodes array and placed at the end of nodes.
 */
NODE *pull_min(int *heap_length, int back_of_array, int back_offset) {
    if (*heap_length <= 0) {
        fprintf(stderr, "Error: Back of Array is less than or equal to 0\n");
        return NULL;
    }
    swap_nodes(0, back_of_array - 1 - back_offset);
    (*heap_length)--;
    shift_left_array(*heap_length);
    return (nodes + back_of_array - 1 - back_offset);
}

/**
 * @brief Insert a node into the heap, reheapify it afterward.
 *
 * @param heap_length
 *      the length of the current heap
 * @param weight
 *      the weight of the node to be set
 * @param symbol
 *      the symbol of the node to be set
 * @param left_child_address
 *      the address of the left child of the node to be set
 * @param right_child_address
 *      the address of the right child of the node to be set
 */
void insert_node(int *heap_length, int weight, short symbol, NODE *left_child_address, NODE *right_child_address) {
    if (*heap_length >= 2 * MAX_SYMBOLS - 1) {
        fprintf(stderr, "Error: Heap is full\n");
    }
    (*heap_length)++;
    (nodes + *heap_length - 1)->weight = weight;
    (nodes + *heap_length - 1)->symbol = symbol;
    (nodes + *heap_length - 1)->left = left_child_address;
    (nodes + *heap_length - 1)->right = right_child_address;
    (nodes + *heap_length - 1)->parent = NULL;
    construct_binary_heap(*heap_length);
}

/**
 * @brief Essentially, pulled 2 min-weight nodes out of the heap, then insert a new node with the sum of the 2 weight.
 *
 * @param leaf_amount
 *      amount of leafs in the heap (length of the heap essentially)
 */
void build_huffman_tree_from_heap(int leaf_amount) {
    int total_tree_length = total_tree_length_formula(leaf_amount);
    int back_offset = 0;
    while (leaf_amount > 1) {
        NODE *left_child = pull_min(&leaf_amount, total_tree_length, back_offset);
        back_offset++;
        NODE *right_child = pull_min(&leaf_amount, total_tree_length, back_offset);
        back_offset++;
        // currently make the internal node contains a symbol of negative 1
        insert_node(&leaf_amount, left_child->weight + right_child->weight, -1, left_child, right_child);
        // printf("%d\n", leaf_amount);
    }
}

/**
 * @brief Set the up huffman tree post order object
 *
 * @param root
 *      the root of the tree, it should be arr[0] when call in the beginning
 * @param parent_address
 *      the address of the parent node, it should be NULL when call in the beginning
 */
void set_up_huffman_tree_post_order(NODE *root, NODE *parent_address) {
    if (root == NULL) {
        return;
    }
    set_up_huffman_tree_post_order(root->left, root);
    set_up_huffman_tree_post_order(root->right, root);
    root->parent = parent_address;
}

// ----------------------------------- END HUFFMAN COMPRESS METHOD -----------------------------------

// ----------------------------------- HUFFMAN EMIT_HUFFMAN_TREE METHOD -----------------------------------

/**
 * @brief print the total amount of nodes of the hufferman tree in 2 bytes in big endian order.
 *
 */
void print_num_nodes_in_two_bytes() {
    fputc(((num_nodes & 0x0000ff00)) >> 8, stdout);
    fputc((num_nodes & 0x000000ff), stdout);
}

/**
 * @brief a sequence of n bits, derived from a postorder traversal of the tree, 0 bit indicate a leaf and 1 bit indicate an internal node,
 * the sequence is padded until it reaches a length of 8 bits or a single byte.
 *
 * @param root
 *      the root of the tree
 * @param counter
 *      the counter of the current bits
 * @param buffer
 *      the buffer of the current bits
 */
void post_order_print_huffman_tree_helper(NODE *root, unsigned int *counter, unsigned char *buffer) {
    if (root == NULL) {
        return;
    }
    post_order_print_huffman_tree_helper(root->left, counter, buffer);
    post_order_print_huffman_tree_helper(root->right, counter, buffer);
    *buffer = *buffer << 1;
    if (root->left != NULL && root->right != NULL) {
        *buffer = *buffer | 0x01;
    }
    (*counter)++;
    if (*counter == 8) {
        fputc(*buffer, stdout);
        *counter = 0;
        *buffer = 0;
    }
}

/**
 * @brief set-up for postorder traversal of the tree to derived the sequence of n bits sequence
 *
 * @param root
 *      root of the hufferman tree should be providied
 */
void post_order_print_huffman_tree(NODE *root) {
    if (root == NULL) {
        return;
    }
    unsigned int counter = 0;
    unsigned char buffer = 0;
    post_order_print_huffman_tree_helper(root, &counter, &buffer);
    if (counter < 8 && counter != 0) {
        buffer = buffer << (8 - counter);
        fputc(buffer, stdout);
    }
}

/**
 * @brief Install pointer into nodes_for_symbol and print the symbol of leaf node from left to right.
 *
 * @param root
 *      address of the root of the tree
 */
void identify_leaf_nodes(NODE *root) {
    if (root == NULL) {
        return;
    }
    if (root->left == NULL && root->right == NULL) {
        *(node_for_symbol + root->symbol) = root;
        if (root->symbol == 256) {
            fputc(0xff, stdout);
            fputc(0x00, stdout);
        } else if (root->symbol == 255) {
            fputc(0xff, stdout);
            fputc(0x01, stdout);
        } else {
            fputc(root->symbol, stdout);
        }
    }
    identify_leaf_nodes(root->left);
    identify_leaf_nodes(root->right);
}

/**
 * @brief Emits a description of the Huffman tree used to compress the current block.
 * @details This function emits, to the standard output, a description of the
 * Huffman tree used to compress the current block.  Refer to the assignment handout
 * for a detailed specification of the format of this description.
 */
void emit_huffman_tree() {
    // clean for nodes 
    clear_nodes_for_symbols();
    // printf("Number of Nodes: %d", num_nodes);
    print_num_nodes_in_two_bytes();
    post_order_print_huffman_tree(nodes);
    identify_leaf_nodes(nodes);
}

// ----------------------------------- END HUFFMAN EMIT_HUFFMAN_TREE METHOD -----------------------------------


// ----------------------------------- HUFFMAN READ_HUFFMAN_TREE METHOD -----------------------------------

/**
 * @brief Get the num nodes from two bytes object, update global variable num_nodes to the dimension
 * @details This function get the num nodes from two bytes object, and set the global variable num_nodes.
 *
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem
 */
int get_num_nodes_from_two_bytes() {
    int first_byte = fgetc(stdin);
    if (first_byte == EOF) {
        return 1;
    }
    if (ferror(stdin)) {
        fprintf(stderr, "Error: Attempted to read first byte of amount of nodes from description but failed to");
        return -1;
    }
    int second_byte = fgetc(stdin);
    if (second_byte == EOF || ferror(stdin)) {
        fprintf(stderr, "Error: Attempted to read second byte of amount of nodes from description but failed to");
        return -1;
    }

    num_nodes = (first_byte << 8) | (unsigned char)second_byte;
    // printf("Number of Nodes: %d\n", num_nodes);
    if (num_nodes < 1 || num_nodes >(2 * MAX_SYMBOLS - 1)) {
        fprintf(stderr, "Error: Invalid number of nodes %d, number of nodes must be within [1, %d]", num_nodes, (2 * MAX_SYMBOLS - 1));
        return -1;
    }
    return 0;
}

/**
 * @brief Incrementing the head of the nodes array essentially created a new "node".
 *
 * @param head
 *      the current head index of the stack.
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem.
 */
int push_node_stack(int *head) {
    if (*head < 2 * MAX_SYMBOLS - 1) {
        (*head)++;
        // printf("pushed to stack, head: %d\n", *head);
        return 0;
    }
    fprintf(stderr, "Error: Attempted to push to stack but stack is full when constructing an huffman tree.");
    return -1;
}


/**
 * @brief "pull" 2 nodes from the head of the stack, push them in the back of the array using back_of_array and *back_offset,
 * then add a node back and set the right and left pointer of the new node with the 2 pulled node
 *
 * @param head
 *      the current head index of the stack.
 * @param back_of_array
 *      the back of the array index.
 * @param back_offset
 *      the offset of the back of the array, indicating the amount of "tree" has be made.
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem.
 */
int pull_node_proccess_stack(int *head, int back_of_array, int *back_offset) {
    if (*head > 0) {
        swap_nodes(*head, back_of_array - *back_offset);
        (*head)--;
        swap_nodes(*head, back_of_array - *back_offset - 1);
        (*head)--;
        push_node_stack(head);
        (nodes + *head)->right = (nodes + back_of_array - *back_offset);
        (*back_offset)++;
        (nodes + *head)->left = (nodes + back_of_array - *back_offset);
        (*back_offset)++;
        // printf("pulled from stack, head: %d, back_offset: %d\n", *head, *back_offset);
        return 0;
    } else {
        fprintf(stderr, "Error: Attempted to pull from stack but stack is empty when constructing an huffman tree.");
        return -1;
    }
}

/**
 * @brief Construct the huffman tree from the stack.
 *
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem.
 */
int build_huffman_tree_with_stack() {
    // clean for nodes
    clear_nodes_for_symbols();
    clear_nodes();
    int loop_index_in_bit = num_nodes;
    int top_of_stack = -1, back_of_array = loop_index_in_bit - 1, back_offset = 0;

    int character;
    while (!feof(stdin) && !ferror(stdin) && loop_index_in_bit > 0) {
        character = fgetc(stdin);
        int current_bit_offset = 7;
        while (loop_index_in_bit > 0 && current_bit_offset >= 0) {
            char current_bit = (character >> current_bit_offset) & 0x1;
            // printf("currentBit: %d, loop_index: %d\n", current_bit, loop_index_in_bit);
            if (current_bit == 0) {
                if (push_node_stack(&top_of_stack)) {
                    return -1;
                }
            } else {
                if (pull_node_proccess_stack(&top_of_stack, back_of_array, &back_offset)) {
                    return -1;
                }
            }
            loop_index_in_bit--;
            current_bit_offset--;
        }
    }
    if (feof(stdin) && loop_index_in_bit != 0) {
        fprintf(stderr, "Error: EOF reached before all nodes were read from stdin.");
        return -1;
    }
    if (ferror(stdin)) {
        fprintf(stderr, "Error reading from stdin\n");
        return -1;
    }
    set_up_huffman_tree_post_order(nodes, NULL);
    return 0;
}

/**
 * @brief Reads a description of a Huffman tree and reconstructs the tree from
 * the description.
 * @details  This function reads, from the standard input, the description of a
 * Huffman tree in the format produced by emit_huffman_tree(), and it reconstructs
 * the tree from the description.  Refer to the assignment handout for a specification
 * of the format for this description, and a discussion of how the tree can be
 * reconstructed from it.
 *
 * @return 0 if the tree is read and reconstructed without error, 1 if EOF is
 * encountered at the start of a block, otherwise -1 if an error occurs.
 */
int read_huffman_tree() {
    int return_code = get_num_nodes_from_two_bytes();
    if (return_code == -1) {
        return -1;
    }
    if (return_code == 1) {
        return 1;
    }

    if (build_huffman_tree_with_stack()) {
        return -1;
    }
    return 0;
}

// ----------------------------------- END HUFFMAN READ_HUFFMAN_TREE METHOD -----------------------------------

// ----------------------------------- HUFFMAN COMPRESS_BLOCKS METHOD -----------------------------------

/**
 * @brief traverse from a leaf node to the root node, and set the weight of the node to 0 if the node is the left child of its parent, 1 if the node is the right child of its parent.
 * the value is stored into its parent node.
 *
 * @param leaf
 *      the address of the leaf node
 */
void traverse_upward(NODE *leaf) {
    if (leaf == NULL || leaf->parent == NULL) {
        return;
    }
    if (leaf->parent->left == leaf) {
        leaf->parent->weight = 0;
    } else if (leaf->parent->right == leaf) {
        leaf->parent->weight = 1;
    }
    // printf("%d", leaf->parent->weight);
    traverse_upward(leaf->parent);
}

/**
 * @brief traverse downward till we reach the node leaf with the direction given in the weight section.
 * if reached 8 bits, print the buffer and reset the buffer and counter.
 *
 * @param root
 *      the root of the tree
 * @param counter
 *      the counter of the current bits
 * @param buffer
 *      the buffer of the current bits
 */
void traverse_downward(NODE *root, unsigned char *counter, unsigned char *buffer) {
    if (root == NULL || (root->left == NULL && root->right == NULL)) {
        return;
    }
    *buffer = *buffer << 1;
    (*counter)++;
    if (root->weight) {
        *buffer = *buffer | 0x01;
    }
    if (*counter == 8) {
        fputc(*buffer, stdout);
        if (ferror(stdout)) {
            fprintf(stderr, "Error: Standard output is faulty and cannot be write at this moment, Please verify output file.");
        }
        *counter = 0;
        *buffer = 0;
    }
    if (root->weight == 0) {
        traverse_downward(root->left, counter, buffer);
    } else if (root->weight == 1) {
        traverse_downward(root->right, counter, buffer);
    }
}

/**
 * @brief Reads one block of data from standard input and emits corresponding
 * compressed data to standard output.
 * @details This function reads raw binary data bytes from the standard input
 * until the specified block size has been read or until EOF is reached.
 * It then applies a data compression algorithm to the block and outputs the
 * compressed block to the standard output.  The block size parameter is
 * obtained from the global_options variable.
 *
 * @return 0 if compression completes without error, -1 if an error occurs.
 */
int compress_block() {
    int block_size = determine_block_size_from_global();
    int character;
    int loopCounter = 0;
    do {
        character = fgetc(stdin);
        if (character == EOF) {
            if (ferror(stdin)) {
                fprintf(stderr, "Error: Invalid Read toward stdin or encounter read error.");
                return -1;
            }
        } else {
            *(current_block + loopCounter) = character;
            loopCounter++;
        }
    } while (!feof(stdin) && loopCounter < block_size);

    if (ferror(stdin)) {
        fprintf(stderr, "Error reading from stdin\n");
        return -1;
    }

    if (loopCounter == 0) {
        return 0;
    }

    // Clear the pointers Array
    clear_nodes_for_symbols();
    // not needed but will kept for debugging visual purpose
    set_all_weight_negative();
    int index = 0;
    int index_of_node_array = 0;
    while (index < loopCounter) {
        if (*(node_for_symbol + *(current_block + index)) == NULL) {
            (nodes + index_of_node_array)->weight = 1;
            (nodes + index_of_node_array)->symbol = *(current_block + index);
            *(node_for_symbol + *(current_block + index)) = (nodes + index_of_node_array);
            index_of_node_array++;
        } else {
            (**(node_for_symbol + *(current_block + index))).weight++;
        }
        index++;
    }
    (nodes + index_of_node_array)->weight = 0;
    (nodes + index_of_node_array)->symbol = 256;
    *(node_for_symbol + 256) = (nodes + index_of_node_array);
    index_of_node_array++;

    // int totalWeight = 0;
    // for (int i = 0; i < index_of_node_array; i++) {
    //     totalWeight += (nodes + i)->weight;
    //     printf("Weight: %d, Symbol: %d\n", (nodes + i)->weight, (nodes + i)->symbol);
    // }
    // printf("Total weight: %d\n", totalWeight);

    construct_binary_heap(index_of_node_array);
    // simplify_print_nodes_weight(index_of_node_array);
    num_nodes = total_tree_length_formula(index_of_node_array);
    build_huffman_tree_from_heap(index_of_node_array);
    // print_nodes_array();
    // printf("\n-----END----\n");
    set_up_huffman_tree_post_order(nodes, NULL);
    // // at this point, the huffman tree is constructed
    // print_huffman_tree_in_post_order(nodes);
    // // print_nodes_weight();
    if (ferror(stdout)) {
        fprintf(stderr, "Error: Standard output is faulty and cannot be write at this moment, Please verify output file.");
        return -1;
    }
    emit_huffman_tree(); // emit the description of the tree


    // content of the current_block in the proper format after the description.
    // counter
    unsigned char counter = 0;
    unsigned char buffer = 0;

    // all characters in current block
    for (int i = 0; i < loopCounter; i++) {
        traverse_upward(*(node_for_symbol + *(current_block + i)));
        traverse_downward(nodes, &counter, &buffer);
        if (ferror(stdout)) {
            return -1;
        }
    }

    // the END marker
    traverse_upward(*(node_for_symbol + 256));
    traverse_downward(nodes, &counter, &buffer);
    if (ferror(stdout)) {
        return -1;
    }

    // just in case
    if (counter < 8 && counter) {
        buffer = buffer << (8 - counter);
        fputc(buffer, stdout);
        if (ferror(stdout)) {
            fprintf(stderr, "Error: Standard output is faulty and cannot be write at this moment, Please verify output file.");
            return -1;
        }
    }

    current_block_clean();
    // print_huffman_tree_in_post_order(nodes);
    // print_nodes_array();
    clear_nodes();
    return 0;
}

// ----------------------------------- END HUFFMAN COMPRESS_BLOCKS METHOD -----------------------------------

// ----------------------------------- HUFFMAN DECOMPRESS_BLOCKS METHOD -----------------------------------

/**
 * @brief Identify the leaf nodes of the huffman tree by traversing the huffman tree in pre-order.
 *
 * @param root
 *      the root of the huffman tree.
 * @param current_index_nodes_symbol
 *      the current index of the nodes_symbol array.
 */
void identify_leaf_nodes_stack(NODE *root, int *current_index_nodes_symbol) {
    if (root == NULL) {
        return;
    }
    if (root->left == NULL && root->right == NULL) {
        *(node_for_symbol + *current_index_nodes_symbol) = root;
        (*current_index_nodes_symbol)++;
    }
    identify_leaf_nodes_stack(root->left, current_index_nodes_symbol);
    identify_leaf_nodes_stack(root->right, current_index_nodes_symbol);
}

/**
 * @brief Retrieve all the leafs symbol for all leaf nodes
 *
 * @param amount_of_leafs
 *      the amount of leaf nodes.
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem.
 */
int read_leaf_nodes_symbol(int amount_of_leafs) {
    int character;
    int loop_counter = 0;
    int ff_before = 0;
    while (!feof(stdin) && !ferror(stdin) && loop_counter < amount_of_leafs) {
        character = fgetc(stdin);
        // printf("character: %d, loop_index: %d\n", (unsigned char)character, loop_counter);
        // printf("character: %d\n", (unsigned char)character == 0xff);
        if ((unsigned char)character == 0xff) {
            // printf("ff symbol: %d\n", (*(node_for_symbol + loop_counter))->symbol);
            ff_before = 1;
            continue;
        }
        if (ff_before && (unsigned char)character == 0x00) {
            (*(node_for_symbol + loop_counter))->symbol = 256;
            // printf("256 symbol: %d\n", (*(node_for_symbol + loop_counter))->symbol);
            ff_before = 0;
        } else if (ff_before) {
            (*(node_for_symbol + loop_counter))->symbol = 255;
            // printf("255 symbol: %d\n", (*(node_for_symbol + loop_counter))->symbol);
            ff_before = 0;
        } else {
            (*(node_for_symbol + loop_counter))->symbol = (unsigned char)character;
            // printf("regular character: %d\n", (*(node_for_symbol + loop_counter))->symbol);
        }
        loop_counter++;
    }
    if (feof(stdin) && loop_counter != amount_of_leafs) {
        fprintf(stderr, "Error: Encountered EOF before all leaf nodes were read.");
        return -1;
    }
    if (ferror(stdin)) {
        fprintf(stderr, "Error reading from stdin\n");
        return -1;
    }

    if (ff_before) {
        fprintf(stderr, "Error: Encountered 0xff without 0x00 or 0x(XX) after it. Meaning the file was not compressed probably.");
        return -1;
    }
    return 0;
}
/**
 * @brief determine which direction to travel to from root given the direction provided (should be either 0 or 1).
 *
 * @param direction
 *      a char that should have a value of 0x0 or 0x1
 * @param root
 *      the address of the pointer toward the root of the huffman tree.
 * @return int
 *      -3 indicated a failure, -2 indicated the end of the file, -1 indicated the end of the huffman tree, 0 indicating successful run.
 */
int node_address_for_traversal(char direction, NODE **root) {
    if (*root == NULL) {
        return -3;
    }

    if (direction == 0) {
        (*root) = (*root)->left;
    }
    if (direction == 1) {
        (*root) = (*root)->right;
    }
    if ((*root)->left == NULL && (*root)->right == NULL) {
        if ((*root)->symbol == 256) {
            return -2;
        }
        fputc((*root)->symbol, stdout);
        return -1;
    }
    return 0;
}

/**
 * @brief Traverse the huffman tree from the root to the leafs with the bit options.
 *
 * @return int
 *      -1 indicated a point of failure (error message is printed to stderr). 0 meaning function executed without problem.
 */
int traverse_from_bit_with_huffman() {
    NODE *pointer_to_node = nodes;
    int character;
    do {
        character = fgetc(stdin);
        if (character == EOF) {
            if (ferror(stdin)) {
                fprintf(stderr, "Error: Invalid Read toward stdin or encounter read error.");
                return -1;
            }
            break;
        }
        int current_bit_offset = 7;
        while (current_bit_offset >= 0) {
            // printf("CURRENT pointer_address: %p \n", pointer_to_node);
            char current_bit = (character >> current_bit_offset) & 0x1;
            // printf("%d -> %p\n", current_bit, pointer_to_node);
            switch (node_address_for_traversal(current_bit, &pointer_to_node)) {
            case -3:
                fprintf(stderr, "Error: Attempted to traverse the huffman tree but the root is null.");
                return -1;
            case -2:
                return 0;
            case -1:
                pointer_to_node = nodes;
                break;
            case 0:
                break;
            }
            current_bit_offset--;
        }
    } while (!feof(stdin) && !ferror(stdout) && !ferror(stdin));
    if (ferror(stdin)) {
        fprintf(stderr, "Error reading from stdin\n");
        return -1;
    }
    if (ferror(stdout)) {
        fprintf(stderr, "Error writing to stdout\n");
        return -1;
    }
    return 0;
}

// if 0 then just increment the index of the stack array by 1
// if 1 then subtract the index of the stack array by 1 then set the head of that stack to contains the address of the back of the array 2 Nodes,
// left being n - 1, right being n - 2.
// if stack is left with a single node and there's no more to pull supossingly. We're done constructing the tree 
// If given amount of nodes, stored the pulled nodes from the stack to the back of the array like heap one
// doesn't require shifting as the stack head always be at the optimal place

/**
 * @brief Reads one block of compressed data from standard input and writes
 * the corresponding uncompressed data to standard output.
 * @details This function reads one block of compressed data from the standard
 * input, it decompresses the block, and it outputs the uncompressed data to
 * the standard output.  The input data blocks are assumed to be in the format
 * produced by compress().  If EOF is encountered before a complete block has
 * been read, it is an error.
 *
 * @return 0 if decompression completes without error, -1 if an error occurs.
 */
int decompress_block() {
    int return_code = read_huffman_tree();
    if (return_code == -1) {
        return -1;
    }
    if (return_code == 1) {
        return 0;
    }

    int current_index_nodes_symbol = 0;
    identify_leaf_nodes_stack(nodes, &current_index_nodes_symbol);

    if (read_leaf_nodes_symbol(current_index_nodes_symbol)) {
        return -1;
    }

    if (traverse_from_bit_with_huffman()) {
        return -1;
    }

    return 0;
}

// ----------------------------------- END HUFFMAN DECOMPRESS_BLOCKS METHOD -----------------------------------

/**
 * @brief Reads raw data from standard input, writes compressed data to
 * standard output.
 * @details This function reads raw binary data bytes from the standard input in
 * blocks of up to a specified maximum number of bytes or until EOF is reached,
 * it applies a data compression algorithm to each block, and it outputs the
 * compressed blocks to standard output.  The block size parameter is obtained
 * from the global_options variable.
 *
 * @return 0 if compression completes without error, -1 if an error occurs.
 */
int compress() {
    // obtain the block_size from global_options
    int block_size = determine_block_size_from_global();
    // check just in case
    if (block_size < MIN_BLOCK_SIZE || block_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Error: Invalid block size %d, block size must be within bytes (range [1024, 65536])", block_size);
        return -1;
    }
    // Loop until we read EOF
    while (!feof(stdin) && !ferror(stdin)) {
        if (compress_block() == -1) {
            return -1;
        }
    }
    if (ferror(stdin)) {
        fprintf(stderr, "Error reading from stdin\n");
        return -1;
    }
    fflush(stdout);
    return 0;
}

/**
 * @brief Reads compressed data from standard input, writes uncompressed
 * data to standard output.
 * @details This function reads blocks of compressed data from the standard
 * input until EOF is reached, it decompresses each block, and it outputs
 * the uncompressed data to the standard output.  The input data blocks
 * are assumed to be in the format produced by compress().
 *
 * @return 0 if decompression completes without error, -1 if an error occurs.
 */
int decompress() {
    // Loop until we read EOF
    while (!feof(stdin) && !ferror(stdin)) {
        if (decompress_block() == -1) {
            return -1;
        }
    }
    fflush(stdout);
    return 0;
}

// ----------------------------------- VALIDARGS METHODS -----------------------------------

/**
 * @brief determine if the go value is unmodified (aka the state of global_options).
 *
 * @param go
 *      integer value of global_option, shortened to go.
 * @return int
 *      return 0 if go is modified, return 1 otherwise.
 */
int validargs_valid_positional_arguments(int go) {
    return (go & 0xffffffff) != 0x00000000;
}

/**
 * @brief determine if the go value is modified (aka the state of global_options).
 *
 * @param go
 *      integer value of global_option, shortened to go.
 * @return int
 *      return 0 if go is unmodified or already have detected a -b flag before, return 1 otherwise.
 */
int validargs_valid_optional_arguments(int go) {
    return (go & 0xffffffff) != 0xffff0002;
}

/**
 * @brief Convert a string to an integer.
 *
 * @param input
 *      the string (in the format of a char*) input to be converted into a integer
 * @return int
 *      -1 if the string input contains non numerical value or greater then the range which is specified in this hw
 *      [1024, 65536]
 */
int stringToInt(char *input) {
    int output = 0;
    while (*input != '\0') {
        if (*input < '0' || *input > '9') {
            return -1;
        }
        output = output * 10 + (*input - '0');
        input++;
    }
    if (output < MIN_BLOCK_SIZE || output > MAX_BLOCK_SIZE) {
        return -1;
    }
    output -= 1;
    return output;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    // if no flags, return -1
    if (argc < 2) {
        return -1;
    }
    int position_check = 0;
    int command_detected = 0;
    char **args = argv + 1;
    while (*args) {
        char *arg = *args;
        if (*arg == '-') {
            arg++;
            if (*arg == '\0' || *(arg + 1) != '\0') {
                return -1;
            }
            switch (*arg) {
            case 'h':
                // if the flag read some other content prior to the h flag, return -1
                if (validargs_valid_positional_arguments(global_options) || position_check) {
                    return -1;
                }
                global_options |= 0x00000001;
                return 0;
            case 'c':
                // if the flag read some other content prior to the c flag, return -1
                // or if the flag read more then 4 arguments for -c (program-name, -c, -b, [BLOCKSIZE]), means there's more argument then needed hence return -1.
                if ((validargs_valid_positional_arguments(global_options) || argc > 4) || position_check) {
                    return -1;
                }
                global_options |= 0xffff0002;
                command_detected = 1;
                break;
            case 'd':
                // if the flag read some other content prior to the d flag, return -1
                // or if the flag read more then 2 arguments for -d (program-name, -d), means there's more argument then needed hence return -1.
                if ((validargs_valid_positional_arguments(global_options) || argc > 2) || position_check) {
                    return -1;
                }
                global_options |= 0xffff0004;
                command_detected = 1;
                break;
            case 'b':
                // if the b flag came before any of the previous flags, return -1
                // don't need to verified length here as it should've been verified with -d and -c prior. And if -b is the first flag, it will be detected now.
                if (validargs_valid_optional_arguments(global_options)) {
                    return -1;
                }
                args++;
                // if there's not item after -b flag, return -1
                if (*args == NULL) {
                    return -1;
                }
                // assuming argv now pointing to the supposingly BLOCKSIZE
                int temp = stringToInt(*args);
                // if the BLOCKSIZE is not a valid integer, return -1
                if (temp == -1) {
                    return -1;
                }
                // removed the ffff padding in the front by shifting 16 bit to the left then shift it back down
                global_options = global_options << 16 >> 16;
                // add the BLOCKSIZE to the global_options_replace
                global_options |= (temp << 16);
                break;
            default:
                // string that is not a valid flag, ignored.
                position_check = 1;
                break;
            }
        } else {
            position_check = 1;
            if (command_detected) {
                return -1;
            }
        }
        args++;
    }
    return 0;
}

// ----------------------------------- END VALIDARGS METHODS -----------------------------------
