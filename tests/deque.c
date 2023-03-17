#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct node {
    int data;
    struct node* prev;
    struct node* next;

} node_t;

typedef struct deque {
    node_t* front;
    node_t* back;
    int size;

} deque_t;
 
 deque_t* create_deque() {
    deque_t* deque = malloc(sizeof(deque_t));
    deque->front = NULL;
    deque->back = NULL;
    deque->size = 0;
    return deque;
}

bool is_empty(deque_t* deque) {
    if (deque->size == 0) {
        return true;
    }
    return false;
}

int size(deque_t* deque) {
    return deque->size;
}

void push_front(deque_t* deque, int data){

     node_t* node = malloc(sizeof(node_t));

    if (data == NULL) {
        printf("Error with data ");
    }
    if (node == NULL) {
        printf("Error with node ");
    }
    node->data = data;
    node->prev = NULL;
    node->next = deque->front;
    
    if (deque->front != NULL) {
        deque->front->prev = node;

    }
    deque->front = node;
    if (deque->back == NULL) {
        deque->back = node;
    }

    deque->size++;

}
void push_back(deque_t* deque, int data){

     node_t* node = malloc(sizeof(node_t));

    if (data == NULL) {
        return -1;
    }
    if (node == NULL) {
        return -1;
        }
    node->data = data;
    node->prev = deque->back;
    node->next = NULL;
    deque->back = node;
    
    if (deque->front == NULL) {
        deque->front = node;
    }

    deque->size++;

}

int front_element(deque_t* deque) {

    if (deque == NULL || deque->front == NULL) {
        return -1;
    }
    return (deque->front->data);

}

 int rear_element(deque_t* deque) {
    if (deque == NULL || deque->back == NULL) {
        return -1;
    }
    return (deque->back->data);
}

int remove_front(deque_t* deque){
    if (deque->front == NULL) {
        return -1;

    }
     node_t* node = deque->front->next;
   int num;
   num = deque->front->data;
   if (node != NULL) {
        node->prev = NULL;

    }
    free(deque->front);
    deque->size--;
    return num;


}
int remove_back(deque_t* deque) {

    if (deque->back == NULL) {
        return -1;

    }
   int num;
    num = deque->back->data;
  
    if (deque->back->prev != NULL) {
        deque->back = deque->back->prev;
        free(deque->back->next);
        deque->back->next = NULL;
    }
    else {
        free(deque->back);
        deque->back = NULL;
        deque->front = NULL;
    }
    
    deque->size--;
    return num;


}

void free_deque(deque_t* deque) {

    if (deque == NULL) {
        return -1;

    }

    while (deque->front != NULL) {
        node_t* node = deque->front;
        deque->front = deque->front->next;
        free(node);
    }
    free(deque);
}


// void clear(deque_t* deque);
// void free_queue(deque_t* deque);




//DONE: 
// bool is_empty(deque_t* deque);
// deque_t* make_deque();
// void push_front(deque_t* deque, int data);
// void push_back(deque_t* deque, int data);
// int remove_front(deque_t* deque);
// int size (deque_t* deque);
// int front_element(deque_t* deque);
// int rear_element(deque_t* deque);
// int remove_back(deque_t* deque);