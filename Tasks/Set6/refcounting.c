#include "refcounting.h"

ref_table_t g_reftable = {0};

ref_entry_t *find_ref(ref_entry_t **entry, ref_entry_t ***parent, void *ref) {
    ref_entry_t **root;

    if (entry == NULL)
        root = &g_reftable.head;
    else
        root = entry;

    *parent = root;

    for (ref_entry_t *head = *root; head; head = head->next, *parent = root) {
        if (head->alloc == ref)
            return head;

        if (head->child != NULL) {
            ref_entry_t *child = find_ref(&head->child, parent, ref);
            if (child != NULL)
                return child;
        }
    }

    return NULL;
}

void* new_ref(size_t size, void* dep) {
    if (size == 0)
        return NULL;

    if (dep != NULL) {
        if (g_reftable.count == 0)
            return NULL;

        ref_entry_t **parent, *root = find_ref(NULL, &parent, dep);

        if (root == NULL)
            return NULL;

        return new_ref_priv(root, size);
    }

    return new_ref_priv(NULL, size);
}

void *new_ref_priv(ref_entry_t *root, size_t size) {
    // create a new reference
    ref_entry_t *ref = malloc(sizeof(*ref));
    ref->alloc = malloc(size);
    ref->ctr = 1;
    ref->child = NULL;
    ref->next = NULL;

    if (root == NULL) {
        if (g_reftable.count == 0) {
            g_reftable.head = ref;
        } else {
            ref->next = g_reftable.head;
            g_reftable.head = ref;
        }

        g_reftable.count++;
    } else {
        if (root->child == NULL) {
            root->child = ref;
        } else {
            ref->next = root->child;
            root->child = ref;
        }
    }

    return ref->alloc;
}

void* assign_ref(void* ref) {
    ref_entry_t **parent, *root = find_ref(NULL, &parent, ref);

    if (root == NULL || root->ctr == 0)
        return NULL;

    return assign_ref_priv(root);
}

void *assign_ref_priv(ref_entry_t *root) {
    if (root == NULL)
        return NULL;

    //printf("assigning ref to root: %p, root->alloc: %p\n",root, root->alloc);
    if (root->ctr != 0) {
        root->ctr++;
        for (ref_entry_t *head = root->child; head; head = head->next)
            assign_ref_priv(head);
    }

    return root->alloc;
}

void *del_ref(void *ref) {
    ref_entry_t **parent, *root = find_ref(NULL, &parent, ref);

    if (root == NULL)
        return NULL;

    //printf("calling del_ref_priv with %p, parent: %p, *parent: %p\n",root,parent,*parent);
    root = del_ref_priv(*parent, root);
    *parent = root;

    if (parent == &g_reftable.head) {
        int count = 0;
        //printf("root: %p\n",root);
        void *found = NULL;

        for (ref_entry_t *head = root; head; head = head->next) {
            if (head->alloc == ref && head->ctr != 0) {
                //printf("g_root => head->alloc: %p, head->child: %p, head->ctr: %d\n",head->alloc,head->child,head->ctr);
                found = ref;
            }

            count++;
        }

        g_reftable.count = count;
        //printf("del_ref => g_reftable.count: %d, found: %p\n",count, found);
        return found;
    } else {
        for (ref_entry_t *head = root; head; head = head->next) {
            if (head->alloc == ref && head->ctr != 0)
                return ref;
        }
        // cleanup the root linked list
        cleanup_reftable_root();

        return NULL;
    }
}

ref_entry_t *del_ref_priv(ref_entry_t *parent, ref_entry_t *root) {
    if (root == NULL)
        return NULL;

    for (ref_entry_t *head = parent, *prev = NULL; head;
         prev = head, head = head->next) {
        if (head != root)
            continue;

        ref_entry_t *next = head->next;
        //printf("head: %p, prev: %p, next: %p\n",head,prev,next);

        //printf("head: %p, head->child: %p, head->ctr: %d, head->alloc: %p\n",head,head->child,head->ctr,head->alloc);
        if (head->child != NULL)
            head->child = del_ref_priv(head->child, head->child);

        // decrement the ref count, and free the allocation if refcnt = 0
        if (head->ctr > 0) {
            head->ctr--;

            if (head->ctr == 0)
                free(head->alloc);
        }

        //printf("head->ctr: %d\n",head->ctr);

        if (head->ctr == 0) {
            if (head->child == NULL) {
                free(head);

                if (prev == NULL)
                    parent = next;
                else if (next == NULL)
                    prev->next = NULL;
                else
                    prev->next = next;
            }

            break;
        }
    }

    //printf("end of del_ref_priv => parent: %p\n",parent);

    return parent;
}

void cleanup_reftable_root() {
    for (ref_entry_t *head = g_reftable.head, *prev = NULL; head;
         prev = head, head = head->next) {
        ref_entry_t *next = head->next;

        if (head->ctr == 0 && head->child == NULL) {
            free(head);

            if (prev == NULL)
                g_reftable.head = next;
            else if (next == NULL)
                prev->next = NULL;
            else
                prev->next = next;

            g_reftable.count--;
            break;
        }
    }
}

void *refcount_ref(void *ref, int *count) {
    void *ret = NULL;

    if (count == NULL || ref == NULL)
        return ret;

    *count = 0;

    ref_entry_t **parent, *root = find_ref(NULL, &parent, ref);
    //printf("refcount_ref => parent: %p, root: %p\n",parent,root);

    if (root == NULL)
        return ret;

    *count = root->ctr;
    return root->alloc;
}

void cleanup() {
    cleanup_priv(NULL);
}

void cleanup_priv(ref_entry_t *root) {
    ref_entry_t *head = root;

    if (root == NULL)
        head = g_reftable.head;

    for (ref_entry_t *tmp = head; head; head = tmp) {
        if (head->child != NULL)
            cleanup_priv(head->child);

        if (head->ctr > 0)
            free(head->alloc);

        tmp = head->next;
        free(head);
    }
}
