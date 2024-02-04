/*
 *	General-purpose unbalanced binary tree
 *
 *	Copyright (C) 2017-2022 Free Software Foundation, Inc.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#if !defined(__BTREE_H)
#define __BTREE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(__INT_IS_REGSIZE__) && !defined(__INT_IS_WORD__)
#define __INT_IS_REGSIZE__
typedef unsigned int uint_t __attribute__ ((__mode__ (__word__)));
typedef signed   int int_t  __attribute__ ((__mode__ (__word__)));
#endif

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct btree_node_s {
	const char *key;
	void *data;
	struct btree_node_s* left;
	struct btree_node_s* right;
	} btree_node_t;

btree_node_t *btree_insert(btree_node_t *root, const char *key, void *data);
btree_node_t *btree_search(btree_node_t *root, const char *key);
btree_node_t *btree_delete(btree_node_t *root, const char *key, void (*free_node)(btree_node_t*node));
btree_node_t *btree_clear (btree_node_t *root, void (*free_node)(btree_node_t*node));

void btree_in_order  (btree_node_t *root, void(*)(btree_node_t*,void*), void *arg);	// normal
void btree_pre_order (btree_node_t *root, void(*)(btree_node_t*,void*), void *arg);	// for copy
void btree_post_order(btree_node_t *root, void(*)(btree_node_t*,void*), void *arg);	// for delete

size_t	btree_calc_size(btree_node_t *root);
btree_node_t *btree_rebalance(btree_node_t *root);

#if defined (__cplusplus)
	}
#endif

#endif /* __BTREE_H */
// =====================================================================
#if defined(__BTREE_IMPL)
#undef __BTREE_IMPL
#include <string.h>

#define WALK_STACK_ALLOC	0x10000

// creates a node and initialize it.
// returns the new node.
static btree_node_t* btree_create_node(const char *key, void *data) {
	btree_node_t* node = (btree_node_t*) malloc(sizeof(btree_node_t));
	node->key = key;
	node->data = data;
	node->left = NULL;
	node->right = NULL;
	return node;
	}

// inserts a new node to the tree.
// if node already exists updates the 'data' field.
// returns the new root if changed.
btree_node_t *btree_insert(btree_node_t *root, const char *key, void *data) {
	register int_t c;
	register btree_node_t *cur;
		
	if ( root == NULL )
		return btree_create_node(key, data);

	cur = root;
	while ( cur ) {
		if ( (c = strcmp(key, cur->key)) == 0 ) {
			cur->data = data;
			return root; // error, duplicate key
			}
		if ( c > 0 ) {
			if ( cur->right )
				cur = cur->right;
			else {
				cur->right = btree_create_node(key, data);
				return root;
				}
			}
		else {
			if ( cur->left )
				cur = cur->left;				
			else {
				cur->left = btree_create_node(key, data);
				return root;
				}
			}
		}
	return root;
	}

// search the tree for 'key' node.
// returns NULL if not found.
btree_node_t* btree_search(btree_node_t *root, const char *key) {
	register int_t c;
	
	while ( root ) {
		if ( (c = strcmp(key, root->key)) == 0 )
			return root;
		root = ( c > 0 ) ? root->right : root->left;
		}
	return NULL;
	}

// removes all nodes of the tree.
// returns NULL.
btree_node_t *btree_clear(btree_node_t *root, void (*free_node)(btree_node_t*node)) {
	if ( root ) {
		btree_clear(root->left, free_node);
		btree_clear(root->right, free_node);
		if ( free_node )
			free_node(root);
		free(root);
		}
	return NULL;
	}

// Given a non-empty binary search tree, return the node
// with minimum key value found in that tree.
static btree_node_t* btree_min_value_node(btree_node_t *root) {
	btree_node_t *cur = root;
	while ( cur && cur->left )
		cur = cur->left;
	return cur;
	}

// Given a binary search tree and a key, this function
// deletes the key and returns the new root
btree_node_t *btree_delete(btree_node_t *root, const char *key, void (*free_node)(btree_node_t*)) {
	btree_node_t *tmp;
	int_t	c;
	
	if ( root ) {
		c = strcmp(key, root->key);
		if ( c < 0 )
			root->left = btree_delete(root->left, key, free_node);
		else if ( c > 0 )
			root->right = btree_delete(root->right, key, free_node);
		else { // this node
    		if ( root->left == NULL ) {
				tmp = root->right;
				if ( free_node ) free_node(root);
				free(root);
		        return tmp;
		        }
			else if ( root->right == NULL ) {
				tmp = root->left;
				if ( free_node ) free_node(root);
				free(root);
				return tmp;
		        }
		    tmp = btree_min_value_node(root->right);
		    root->key = tmp->key;
		    root->data = tmp->data;
		    root->right = btree_delete(root->right, tmp->key, free_node);
			}
		}
	
	return root;
	}

// traverse in-order (sorted)
void btree_in_order(btree_node_t *root, void(*f)(btree_node_t*,void*), void *arg) {
	if ( root ) {
		btree_in_order(root->left, f, arg);
		f(root, arg);
		btree_in_order(root->right, f, arg);
		}
	}

// traverse pre-order (good for save or copy)
void btree_pre_order(btree_node_t *root, void(*f)(btree_node_t*,void*), void *arg) {
	if ( root ) {
		f(root, arg);
		btree_pre_order(root->left, f, arg);
		btree_pre_order(root->right, f, arg);
		}
	}

// traverse post-order (good for delete)
void btree_post_order(btree_node_t *root, void(*f)(btree_node_t*,void*), void *arg) {
	if ( root ) {
		btree_post_order(root->left, f, arg);
		btree_post_order(root->right, f, arg);
		f(root, arg);
		}
	}

// btree does not keep the number of nodes.
// this function calculates the this number.
// returns the number of nodes in the tree.
size_t btree_calc_size(btree_node_t *root) {
	if ( root ) {
		size_t	stack_alloc = WALK_STACK_ALLOC;
		btree_node_t **stack = (btree_node_t **) malloc(sizeof(btree_node_t*) * stack_alloc);
		btree_node_t *cur;
		register size_t	head, tail;
		size_t	size = 0;
		
		cur = NULL;
		head = tail = 0;
		stack[tail ++] = root;
		while ( head < tail ) {
			cur = stack[head ++];
		    if ( cur->left )	stack[tail ++] = cur->left;
		    if ( cur->right )	stack[tail ++] = cur->right;
			if ( tail == stack_alloc )
				stack = (btree_node_t **) realloc(stack,
					sizeof(btree_node_t*) * (stack_alloc += WALK_STACK_ALLOC));
			size ++;
			}
		free(stack);
		return size;
		}
	return 0;
	}

// part of rebalance, rebuilds the tree by connecting pointers
// in correct order from a sorted array (table).
static btree_node_t* btree_array_to_btree(btree_node_t **table, int_t start, int_t end) {
	btree_node_t *root;
	int_t mid;
	
	if ( start > end ) return NULL;
	mid = (start + end) >> 1;
	root = table[mid];
	root->left  = btree_array_to_btree(table, start, mid - 1);
	root->right = btree_array_to_btree(table, mid + 1, end);
	return root;
	}

// btree is not balanced by default.
// this routine rebuild the tree in perfect balance
btree_node_t *btree_rebalance(btree_node_t *root) {
	if ( root ) {
		size_t	stack_alloc = WALK_STACK_ALLOC;
		btree_node_t **stack = (btree_node_t **) malloc(sizeof(btree_node_t*) * stack_alloc);
		btree_node_t **table, *cur, *new_root = root;
		size_t	head, count, size;
		register size_t tail;

		/* pre order */
		cur = NULL;
		head = tail = size = 0;
		stack[tail ++] = root;
		while ( head < tail ) {
			cur = stack[head ++];
		    if ( cur->left )	stack[tail ++] = cur->left;
		    if ( cur->right )	stack[tail ++] = cur->right;
			if ( tail == stack_alloc )
				stack = (btree_node_t **) realloc(stack,
					sizeof(btree_node_t*) * (stack_alloc += WALK_STACK_ALLOC));
			size ++;
			}

		/* in order */
		cur = root;
		tail = count = 0;
		table = (btree_node_t **) malloc(sizeof(btree_node_t *) * size);
		do {
			if ( cur ) {
				stack[tail ++] = cur;
				cur = cur->left;
				if ( tail == stack_alloc )
					stack = (btree_node_t **) realloc(stack,
						sizeof(btree_node_t*) * (stack_alloc += WALK_STACK_ALLOC));
				}
			else if ( tail ) {
				cur = stack[-- tail];
				table[count ++] = cur;
		        cur = cur->right;
				}
			else
				break;
			} while ( 1 );
	
		free(stack);
		new_root = btree_array_to_btree(table, 0, size - 1);
		free(table);
		return new_root;
		}
	return root;
	}

#ifdef TEST
#undef TEST
#define __STACK_IMPL
#include "dstack.c"
#include "timer.c"

// prints all nodes sorted by key
void btree_print(FILE *out, btree_node_t *root) {
	if ( root ) {
		btree_print(out, root->left);
		fprintf(out, "%s\n", root->key);
		btree_print(out, root->right);
		}
	}

// graphviz, recursive part
void btree_print_dot_r(btree_node_t *root, FILE *fp) {
	fprintf(fp, "\t\"%s\";\n", root->key);

	if ( root->left ) {
		fprintf(fp, "\t\"%s\" -> \"%s\";\n", root->key, root->left->key);
		btree_print_dot_r(root->left, fp);
		}
	
	if ( root->right ) {
		fprintf(fp, "\t\"%s\" -> \"%s\";\n", root->key, root->right->key);
		btree_print_dot_r(root->right, fp);
		}
	}

// create graphviz file
void btree_print_dot(btree_node_t *root, const char *fname) {
	FILE *fp = fopen(fname, "w");
	if ( fp ) {
		fprintf(fp, "digraph BST {\n");
		if ( root ) {
		    if ( root->right || root->left )
				btree_print_dot_r(root, fp);
			else
				fprintf(fp, "\t\"%s\";\n", root->key);
			}
		else
			fprintf(fp, "\n");
		fprintf(fp, "}\n");
		fclose(fp);
		}
	}

// 
int main(int argc, char *argv[]) {
	FILE	*fp;
	stack_t	*stk = stack_create();
	btree_node_t *root = NULL;

	if ( (fp = fopen("words.shuf", "rt")) != NULL ) {
		char buf[128];
		while ( fgets(buf, 128, fp) ) {
			int_t l = strlen(buf) - 1;
			if ( buf[0] != '#' && l > 0 ) {
				if ( buf[l] == '\n' ) buf[l] = '\0';
				stack_push(stk, strdup(buf));
				}
			}
		fclose(fp);

		t_start();
		for ( uint_t i = 0; i < stack_size(stk); i ++ )
			root = btree_insert(root, (const char *) stk->table[i], NULL);
		root = btree_rebalance(root);
		t_stop("insert & rebalance");

//		btree_print_dot(root, "btree.dot");
		
		if ( argc > 1 ) {
			t_start();
			btree_node_t *n = btree_search(root, argv[1]);
			t_stop("search");
			if ( n )
				printf("Found! %s %p\n", n->key, n);
			else
				printf("%s NOT found!\n", argv[1]);
			}
//		else
//			btree_print(stdout, root);
		}
	else
		printf("shuf /usr/share/dict/words > words.shuf, words.shuf not found\n");
	btree_clear(root, NULL);
	stack_enum(stk, free);
	stack_destroy(stk);
	return 0;
	}
#endif /* TEST */
#endif /* __BTREE_IMPL */
