#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define RB_RED 0
#define RB_BLACK 1
#define RB_DOUBLE_BLACK 2

struct rb_node {
    // parent is a pointer to struct rb_node, and struct rb_node has the same alignment as a pointer (at least 4, 8 on 64-bit)
    // leaving us 2 bits to play with.
    uintptr_t parent_and_color;
    struct rb_node *left, *right;
};

#define get_blackness(node_ptr) ((node_ptr)->parent_and_color & 0x3)
#define set_blackness(node_ptr, blackness)                                                         \
    ((node_ptr)->parent_and_color =                                                                \
         ((node_ptr)->parent_and_color & ~(uintptr_t)0x3) | ((blackness) & 0x3))

#define get_parent(node_ptr) ((struct rb_node *)((node_ptr)->parent_and_color & ~(uintptr_t)0x3))
#define set_parent(node_ptr, parent_ptr)                                                           \
    ((node_ptr)->parent_and_color = ((node_ptr)->parent_and_color & 0x3) | ((uintptr_t)(parent_ptr)))

struct int_rb_node {
    struct rb_node node;
    int value;
};

inline static void get_p_gp(struct rb_node *node, struct rb_node **p, struct rb_node **gp,
                            struct rb_node **s) {
    *p = get_parent(node);

    if (*p) {
        *gp = get_parent(*p);
    } else {
        *gp = NULL;
    }
}

void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **pnode) {
    *pnode = node;
    set_parent(node, parent);
    node->left = node->right = NULL;
}

static void rotate_right(struct rb_node **root, struct rb_node *p) {
    // p is called the parent in the insertion routine.
    // its children are the nodes under operation
    // its parent is the 'grandparent'

    struct rb_node **gp_link; // receives the ptr to the node replacing the grandparent.
    struct rb_node *ggp;

    struct rb_node *n2 = p->right;
    struct rb_node *gp = get_parent(p);

    if (get_parent(gp)) {
        struct rb_node *l = get_parent(gp);
        if (l->left == gp) {
            gp_link = &l->left;
        } else {
            gp_link = &l->right;
        }

        ggp = l;
    } else {
        gp_link = root;
        ggp = NULL;
    }

    // must change ->right for p
    p->right = gp;

    // must change ->left for gp
    gp->left = n2;

    // must change either ->left or ->right for ggp
    *gp_link = p;

    // must change ->parent for p, gp, and n2
    set_parent(p, ggp);
    set_parent(gp, p);

    if (n2)
        set_parent(n2, gp);
}

static void rotate_left(struct rb_node **root, struct rb_node *p) {
    // p is called the parent in the insertion routine.
    // its children are the nodes under operation
    // its parent is the 'grandparent'

    struct rb_node **gp_link; // receives the ptr to the node replacing the grandparent.
    struct rb_node *ggp;

    struct rb_node *n2 = p->left;
    struct rb_node *gp = get_parent(p);

    if (get_parent(gp)) {
        struct rb_node *l = get_parent(gp);
        if (l->left == gp) {
            gp_link = &l->left;
        } else {
            gp_link = &l->right;
        }

        ggp = l;
    } else {
        gp_link = root;
        ggp = NULL;
    }

    // must change ->left for p
    p->left = gp;

    // must change ->right for gp
    gp->right = n2;

    // must change ->left or ->right for ggp
    *gp_link = p;

    // must change ->parent for p, gp, and n2
    set_parent(p, ggp);
    set_parent(gp, p);

    if (n2)
        set_parent(n2, gp);
}

void rb_insert_color(struct rb_node *node, struct rb_node **root) {
    set_blackness(node, RB_RED);

    while (get_parent(node) && get_blackness(get_parent(node)) == RB_RED) {
        // NOTE: node->parent is NOT the root (because it's red).
        struct rb_node *p, *gp, *s;
        get_p_gp(node, &p, &gp, &s);

        int is_p_a_left = p == gp->left;
        int is_node_a_left = node == p->left;

        if (is_p_a_left) {
            if (is_node_a_left) {
                rotate_right(root, p);
                set_blackness(node, RB_BLACK);
            } else {
                rotate_left(root, node);
            }
        } else {
            if (!is_node_a_left) {
                rotate_left(root, p);
                set_blackness(node, RB_BLACK);
            } else {
                rotate_right(root, node);
            }
        }

        node = p;
    }

    if (!get_parent(node)) {
        set_blackness(node, RB_BLACK);
    }
}

static struct rb_node *greatest_lesser(struct rb_node *parent) {
    struct rb_node *node = parent->left;

    if (!node)
        return NULL;

    while (node->right) {
        node = node->right;
    }

    return node;
}

static struct rb_node *least_greater(struct rb_node *parent) {
    struct rb_node *node = parent->right;

    if (!node)
        return NULL;

    while (node->left) {
        node = node->left;
    }

    return node;
}

inline static int is_leaf(struct rb_node *node) { return !node->left && !node->right; }

inline static void fix_self_references(struct rb_node *node, struct rb_node *from,
                                       struct rb_node *to) {
    if (get_parent(node) == from)
        set_parent(node, to);
    if (node->left == from)
        node->left = to;
    if (node->right == from)
        node->right = to;
}

inline static void assign_parent_to_children(struct rb_node *node) {
    if (node->left)
        set_parent(node->left, node);
    if (node->right)
        set_parent(node->right, node);
}

static void swap_nodes(struct rb_node **root, struct rb_node *a, struct rb_node *b) {
    // things needing to be swapped:
    // links from above (->parent->left or ->parent->right OR *root)
    // link to above (->parent)
    // links from below (->left->parent and ->right->parent)
    // links to below (->left and ->right)
    // color

    // things to note: when transfering pointers from a to b,
    // if one of a's pointers points to b, that pointer needs to be made to point to a (from b)

    if (a == b)
        return;

    struct rb_node tmp = *a;

    struct rb_node **a_link, **b_link;

    int is_orig_a_left_child, is_orig_b_left_child;

    is_orig_a_left_child = get_parent(a) && get_parent(a)->left == a;
    is_orig_b_left_child = get_parent(b) && get_parent(b)->left == b;

    *a = *b;
    *b = tmp;

    // Fix any self-references.
    fix_self_references(a, a, b);
    fix_self_references(b, b, a);

    if (get_parent(a)) {
        if (is_orig_b_left_child) {
            a_link = &get_parent(a)->left;
        } else {
            a_link = &get_parent(a)->right;
        }
    } else {
        a_link = root;
    }

    if (get_parent(b)) {
        if (is_orig_a_left_child) {
            b_link = &get_parent(b)->left;
        } else {
            b_link = &get_parent(b)->right;
        }
    } else {
        b_link = root;
    }

    *a_link = a;
    *b_link = b;

    assign_parent_to_children(a);
    assign_parent_to_children(b);
}

void print_node_full(struct rb_node *node) {
    const char *s = "only";

    struct rb_node *pl = NULL, *pr = NULL;

    if (get_parent(node)) {
        s = get_parent(node)->left == node ? "left" : "right";
        pl = get_parent(node)->left;
        pr = get_parent(node)->right;
    }

    printf("\nNode %p is the %s child of %p, and has children %p and %p (left and right)\nThe "
           "parent's children are %p and %p\nThe childrens' parents "
           "are %p and %p\n",
           node, s, get_parent(node), node->left, node->right, pl, pr,
           node->left ? get_parent(node->left) : NULL, node->right ? get_parent(node->right) : NULL);
}

inline static void get_p_s_cl_cr(struct rb_node *node, struct rb_node **p, struct rb_node **s,
                                 struct rb_node **cl, struct rb_node **cr) {
    *p = get_parent(node);

    if (*p) {
        if (node == (*p)->left) {
            *s = (*p)->right;
        } else {
            *s = (*p)->left;
        }
    } else {
        *s = NULL;
    }

    if (*s) {
        *cl = (*s)->left;
        *cr = (*s)->right;
    }
}

void rb_del(struct rb_node *node, struct rb_node **root) {
    while (!is_leaf(node)) {
        struct rb_node *successor = node->right ? least_greater(node) : greatest_lesser(node);

        swap_nodes(root, node, successor);
    }

    if (get_blackness(node) == RB_RED) {
        goto unlink;
    }

    set_blackness(node, RB_DOUBLE_BLACK);

    struct rb_node *current = node;

    // https://medium.com/analytics-vidhya/deletion-in-red-black-rb-tree-92301e1474ea

    while (get_blackness(current) == RB_DOUBLE_BLACK && get_parent(current)) {
        struct rb_node *p, *s, *cl, *cr;
        get_p_s_cl_cr(current, &p, &s, &cl, &cr);

        // NOTE: We're guaranteed the sibling exists, since at some point, in current's subtree,
        // there was a non-nil black node. Hence, there must be a non-nil black node in the
        // sibling's subtree.
        if (get_blackness(s) == RB_BLACK) {
            if ((!cl || get_blackness(cl) == RB_BLACK) && (!cr || get_blackness(cr) == RB_BLACK)) {
                // Both children of sibling are black (either nil or actually black).
                // Case 3.
                set_blackness(p, get_blackness(p) + 1);
                if (s)
                    set_blackness(s, RB_RED);
                set_blackness(current, RB_BLACK);
                current = p;
            }

            if (current == p->left) {
                // Left mirror of 5 and 6.

                if ((!cr || get_blackness(cr) == RB_BLACK) && cl && get_blackness(cl) == RB_RED) {
                    // Case 5.
                    set_blackness(cl, RB_BLACK);
                    set_blackness(s, RB_RED);
                    rotate_right(root, cl);
                    // Case 6 will happen next.
                } else if (cr && get_blackness(cr) == RB_RED) {
                    // Case 6.
                    set_blackness(s, get_blackness(p));
                    set_blackness(p, RB_BLACK);
                    rotate_left(root, s);
                    set_blackness(current, RB_BLACK);
                    set_blackness(cr, RB_BLACK);
                }
            } else {
                // Right mirror of 5 and 6.

                if ((!cl || get_blackness(cl) == RB_BLACK) && cr && get_blackness(cr) == RB_RED) {
                    // Case 5.
                    set_blackness(cr, RB_BLACK);
                    set_blackness(s, RB_RED);
                    rotate_left(root, cr);
                } else if (cl && get_blackness(cl) == RB_RED) {
                    set_blackness(s, get_blackness(p));
                    set_blackness(p, RB_BLACK);
                    rotate_right(root, s);
                    set_blackness(current, RB_BLACK);
                    set_blackness(cl, RB_BLACK);
                }
            }
        } else {
            // Case 4.
            set_blackness(p, RB_RED);
            set_blackness(s, RB_BLACK);

            if (s == p->left) {
                rotate_right(root, s);
            } else {
                rotate_left(root, s);
            }
        }
    }

    if (!get_parent(current)) {
        set_blackness(current, RB_BLACK);
    }

unlink:;
    struct rb_node *parent = get_parent(node);
    struct rb_node **pnode =
        parent ? node == parent->left ? &parent->left : &parent->right
                     : root;
    *pnode = NULL;
}

struct int_rb_node *int_rb_search(struct rb_node *root, int value) {
    struct rb_node *current = root;

    while (current) {
        struct int_rb_node *in = container_of(current, struct int_rb_node, node);
        if (value < in->value) {
            current = current->left;
        } else if (value > in->value) {
            current = current->right;
        } else {
            return container_of(current, struct int_rb_node, node);
        }
    }

    return NULL;
}

int int_rb_del(struct rb_node **root, int value) {
    struct int_rb_node *in = int_rb_search(*root, value);

    if (in) {
        rb_del(&in->node, root);
        // void print_tree(struct rb_node *node);
        // print_tree(*root);
        free(in);
    }

    return in != NULL;
}

int int_rb_add(struct rb_node **root, int value) {
    struct int_rb_node *node = malloc(sizeof(*node));
    node->value = value;

    struct rb_node **current = root, *parent = NULL;

    while (*current) {
        parent = *current;
        struct int_rb_node *in = container_of(*current, struct int_rb_node, node);
        if (value < in->value) {
            current = &(*current)->left;
        } else if (value > in->value) {
            current = &(*current)->right;
        } else {
            return 0;
        }
    }

    rb_link_node(&node->node, parent, current);
    rb_insert_color(&node->node, root);
    return 1;
}

void print_node(struct rb_node *node, int depth) {
    if (!node)
        return;

    print_node(node->left, depth + 1);

    struct int_rb_node *in = container_of(node, struct int_rb_node, node);
    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("%d (%s)\n", in->value,
           get_blackness(node) == RB_RED     ? "RED"
           : get_blackness(node) == RB_BLACK ? "BLACK"
                                             : "???");

    print_node(node->right, depth + 1);
}

void print_tree(struct rb_node *root) { print_node(root, 0); }

void in_order_traversal(struct rb_node *node) {
    if (!node)
        return;

    in_order_traversal(node->left);
    struct int_rb_node *in = container_of(node, struct int_rb_node, node);
    printf("%d ", in->value);
    in_order_traversal(node->right);
}

int main(void) {
    struct rb_node *root = NULL;

    int values[] = {10, 20, 30, 15, 25, 5, 1, 8, 7};
    size_t num_values = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < num_values; i++) {
        printf("Inserting %d...\n", values[i]);
        int_rb_add(&root, values[i]);
    }

    printf("\nIn-order traversal (should be sorted):\n");
    in_order_traversal(root);
    printf("\n\nTree structure with colors:\n");
    print_node(root, 0);

    printf("\n=== BEGIN DELETION TESTS ===\n");

    // Delete a few values one-by-one and print after each deletion
    int to_delete[] = {1, 7, 10, 20, 15}; // includes leaf, internal, and root candidates
    size_t num_deletes = sizeof(to_delete) / sizeof(to_delete[0]);

    for (size_t i = 0; i < num_deletes; i++) {
        int val = to_delete[i];
        printf("\nDeleting %d...\n", val);
        int_rb_del(&root, val);

        printf("\nTree structure:\n");
        print_node(root, 0);
        printf("In-order traversal:\n");
        in_order_traversal(root);
    }

    printf("\n=== END DELETION TESTS ===\n");

    // Insert more values to get wider coverage
    int extra_values[] = {50, 40, 60, 55, 65, 35, 45, 99, 37, 492, 1039, 472, 399};
    size_t extra_count = sizeof(extra_values) / sizeof(extra_values[0]);

    for (size_t i = 0; i < extra_count; i++) {
        printf("Inserting %d...\n", extra_values[i]);
        int_rb_add(&root, extra_values[i]);
    }

    printf("\nTree after extra insertions:\n");
    in_order_traversal(root);
    printf("\n");
    print_node(root, 0);

    // Delete nodes with two children
    int delete_two_children[] = {50, 40}; // likely to have two children
    for (size_t i = 0; i < sizeof(delete_two_children) / sizeof(delete_two_children[0]); i++) {
        int val = delete_two_children[i];
        printf("\nDeleting (two children) %d...\n", val);
        int_rb_del(&root, val);

        printf("In-order traversal:\n");
        in_order_traversal(root);
        printf("\nTree structure:\n");
        print_node(root, 0);
    }

    // Try deleting something not in the tree
    int not_present = 999;
    printf("\nAttempting to delete %d (not present)...\n", not_present);
    int_rb_del(&root, not_present);
    printf("In-order traversal (should be unchanged):\n");
    in_order_traversal(root);
    printf("\nTree structure:\n");
    print_node(root, 0);

    // Reinsert and re-delete
    int reinserts[] = {45, 55};
    for (size_t i = 0; i < sizeof(reinserts) / sizeof(reinserts[0]); i++) {
        printf("\nReinserting %d...\n", reinserts[i]);
        int_rb_add(&root, reinserts[i]);
    }

    printf("Tree after reinsertion:\n");
    in_order_traversal(root);
    printf("\n");
    print_node(root, 0);

    for (size_t i = 0; i < sizeof(reinserts) / sizeof(reinserts[0]); i++) {
        printf("\nDeleting reinserted value %d...\n", reinserts[i]);
        int_rb_del(&root, reinserts[i]);
        in_order_traversal(root);
        printf("\n");
        print_node(root, 0);
    }

    // Delete everything remaining
    printf("\nDeleting all remaining values...\n");
    while (root != NULL) {
        int val = container_of(root, struct int_rb_node, node)->value;
        printf("Deleting %d...\n", val);
        int_rb_del(&root, val);
        in_order_traversal(root);
        printf("\n");
    }

    if (root == NULL) {
        printf("Tree is now empty.\n");
    } else {
        printf("Tree is NOT empty after deletion! Bug?\n");
        print_node(root, 0);
    }
    return 0;
}
