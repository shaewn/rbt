#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define container_of(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))

enum {
    RB_NONE = -1,
    RB_RED,
    RB_BLACK,
    RB_DOUBLE_BLACK,
};

struct rb_node {
    struct rb_node *parent;
    struct rb_node *left, *right;
    int color;
};

struct int_rb_node {
    struct rb_node node;
    int value;
};

void get_p_gp_s(struct rb_node *node, struct rb_node **p, struct rb_node **gp, struct rb_node **s) {
    *p = node->parent;

    if (*p) {
        *gp = (*p)->parent;
    } else {
        *gp = NULL;
    }

    if (*gp) {
        if (*p == (*gp)->left) {
            *s = (*gp)->right;
        } else {
            *s = (*gp)->left;
        }
    } else {
        *s = NULL;
    }
}

void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **pnode) {
    *pnode = node;
    node->parent = parent;
    node->left = node->right = NULL;
}

void rotate_right(struct rb_node **root, struct rb_node *p) {
    // p is called the parent in the insertion routine.
    // its children are the nodes under operation
    // its parent is the 'grandparent'

    struct rb_node **gp_link; // receives the ptr to the node replacing the grandparent.
    struct rb_node *ggp;

    struct rb_node *n2 = p->right;
    struct rb_node *gp = p->parent;

    if (gp->parent) {
        struct rb_node *l = gp->parent;
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
    p->parent = ggp;
    gp->parent = p;

    if (n2)
        n2->parent = gp;
}

void rotate_left(struct rb_node **root, struct rb_node *p) {
    // p is called the parent in the insertion routine.
    // its children are the nodes under operation
    // its parent is the 'grandparent'

    struct rb_node **gp_link; // receives the ptr to the node replacing the grandparent.
    struct rb_node *ggp;

    struct rb_node *n2 = p->left;
    struct rb_node *gp = p->parent;

    if (gp->parent) {
        struct rb_node *l = gp->parent;
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
    p->parent = ggp;
    gp->parent = p;

    if (n2)
        n2->parent = gp;
}

void rb_insert_color(struct rb_node *node, struct rb_node **root) {
    node->color = RB_RED;

    while (node->parent && node->parent->color == RB_RED) {
        // NOTE: node->parent is NOT the root (because it's red).
        struct rb_node *p, *gp, *s;
        get_p_gp_s(node, &p, &gp, &s);

        int is_p_a_left = p == gp->left;
        int is_node_a_left = node == p->left;

        if (is_p_a_left) {
            if (is_node_a_left) {
                rotate_right(root, p);
                node->color = RB_BLACK;
            } else {
                rotate_left(root, node);
            }
        } else {
            if (!is_node_a_left) {
                rotate_left(root, p);
                node->color = RB_BLACK;
            } else {
                rotate_right(root, node);
            }
        }

        node = p;
    }

    if (!node->parent) {
        node->color = RB_BLACK;
    }
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
           node->color == RB_RED     ? "RED"
           : node->color == RB_BLACK ? "BLACK"
                                     : "???");

    print_node(node->right, depth + 1);
}

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

    return 0;
}
