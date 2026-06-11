#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node* next;
};

void linkedListTraversal(struct Node* ptr){
    while(ptr != NULL){
        printf("Element: %d\n", ptr->data);
        ptr = ptr->next;
    }
}

struct Node * insertAtFirst (struct Node *head, int data){
    struct Node * ptr = (struct Node *) malloc(sizeof(struct Node));
    ptr->next = head;
    ptr->data = data;
    return ptr; // returned new head
}

struct Node * insertAtIndex (struct Node *head, int data, int index){
    struct Node * ptr = (struct Node *) malloc(sizeof(struct Node));
    struct Node * p = head;
    int i = 0; // p vo hai jiske baad lake hume insert krna h
    while(i != index-1){ // i starts from 0
        p = p -> next;
        i++;
    }
    ptr-> data = data;
    ptr -> next = p -> next;
    p->next = ptr;


    return head; // returned new head
}

struct Node* insertAtLast(struct Node* head, int data)
{
    struct Node* ptr = (struct Node*) malloc(sizeof(struct Node));
    ptr->data = data;
    ptr->next = NULL;

    // if list is empty
    if (head == NULL)
        return ptr;

    struct Node* p = head;
    while (p->next != NULL) {
        p = p->next;
    }

    p->next = ptr;
    return head;
}


int main() {
    struct Node* head;
    struct Node* second;
    struct Node* third;
    struct Node* fourth;
    head = (struct Node*) malloc(sizeof (struct Node));
    fourth = (struct Node*) malloc(sizeof (struct Node)); // order dont matter malloc deke rakha h.
     second = (struct Node*) malloc(sizeof (struct Node));
      third = (struct Node*) malloc(sizeof (struct Node));
    
    // link first and second node
      head->data = 7;
      head->next = second;

    // link second and third node
      second->data = 11;
      second->next = third;


    // link third and fourth node
      third->data = 66;
      third->next = fourth;

      // link fourth and null node
      fourth -> data = 99;
        fourth -> next = NULL;


        linkedListTraversal(head);

        printf("\n");
    // insert in beginning
        head = insertAtFirst(head,56);

        

        linkedListTraversal(head);
     
    printf("\n");
    // insert in middle
    
    int index;

    head = insertAtIndex(head , 56 , 3); // The third that is index cant be 0 because that would be insert at beginnning


    linkedListTraversal(head);

    printf("\n");

    // insert at last

    head = insertAtLast(head , 77); // The third that is index cant be 0 because that would be insert at beginnning


    linkedListTraversal(head);
     
    printf("\n");
    

    return 0;
}