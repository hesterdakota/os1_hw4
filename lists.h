/*  Header file containing the c_list_T and h_list_T types
    as well as functions pertaining to operations on the 
    linked lists. Most of this code was either taken from 
    or inspired by code included in the chapter 29 slides
*/

#ifndef LIST_HEADER
#define LIST_HEADER

#include <stdlib.h>
#include <pthread.h> // Using POSIX mutex lock

// This would be a lot easier in c++

// Standard concurrent linked list typedefs and functions
// List only has one mutex, placed in the c_list_T typedef

typedef struct __c_node_T {
    int key;
    struct __c_node_T* next;
} c_node_T;

typedef struct c_list_T {
    c_node_T* head;
    pthread_mutex_t lock; // Lock protects entire list
} c_list_T;

// init function for basic locking list
void cListInit(c_list_T* c_list)  {
    c_list->head = NULL;
    pthread_mutex_init(&c_list->lock, NULL);
}

// insert function for basic locking list
int cListInsert(c_list_T* c_list, int key)   {
    c_node_T* new_c_node = (c_node_T*) malloc(sizeof(c_node_T));
    if (new_c_node == NULL) {
        return -1; // proper error output should be done in main code
    }
    new_c_node->key = key;

    // lock list, set new node's next to current head, set head pointer to new node
    pthread_mutex_lock(&(c_list->lock)); // lock
    new_c_node->next = c_list->head;
    c_list->head = new_c_node;
    pthread_mutex_unlock(&c_list->lock); // unlock

    return 0; // success
}

// lookup function for basic locking list
int cListLookup(c_list_T* c_list, int key)   {
    int return_val = -1;

    pthread_mutex_lock(&c_list->lock); // lock
    if (!c_list->head)  {
        // empty list
        pthread_mutex_unlock(&c_list->lock);
        return return_val;
    }
    c_node_T* curr = c_list->head;
    
    while (curr)    {
        if (key == curr->key)   {
            return_val = 0;
            break;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&c_list->lock);
    return return_val;
}

// Hand over hand linked list typededs and functions
// List has one mutex per node

typedef struct __h_node_T   {
    int key;
    struct __h_node_T* next;
    pthread_mutex_t lock; // One lock per node
} h_node_T;

typedef struct __h_list_T   {
    h_node_T* head;
    pthread_mutex_t head_lock; // From slides
} h_list_T;

// init function for hand over hand locking list
void hListInit(h_list_T* h_list)  {
    h_list->head = NULL; // lock will be iniitilaized upon 1st insert call
    pthread_mutex_init(&h_list->head_lock, NULL);
}

// insert function for hand over hand locking list
int hListInsert(h_list_T* h_list, int key)   {
    h_node_T* new_h_node = (h_node_T*) malloc(sizeof(h_node_T));
    if (!new_h_node) {
        return -1; // proper error output should be done in main code
    }

    // initialize lock
    pthread_mutex_init(&new_h_node->lock, NULL);

    new_h_node->key = key;

    pthread_mutex_lock(&h_list->head_lock); // lock head
    new_h_node->next = h_list->head;
    pthread_mutex_unlock(&h_list->head_lock); // unlock

    h_list->head = new_h_node; // update head of list

    return 0; // success
}

// lookup function for basic locking list
int hListLookup(h_list_T* h_list, int key)   {
    int return_val = -1;

    if (!h_list->head) return return_val; // Empty list
;
    h_node_T* curr = h_list->head;
    h_node_T* next;

    pthread_mutex_lock(&curr->lock); // lock head node

    while (curr)    {
        if (curr->key == key)   { // ciritcial operation
            return_val = 0;
            pthread_mutex_unlock(&curr->lock); // unlock
            break;
        }
        next = curr->next;

        // check if next node exitst
        if (next) {
            pthread_mutex_lock(&next->lock);
        }
    
        pthread_mutex_unlock(&curr->lock);
        curr = next;
    }
    return return_val;
}

#endif