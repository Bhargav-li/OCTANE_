#include <stdio.h>
#include <stdlib.h>

struct Node  {
int data;
struct Node *next;
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
struct Node *head = NULL, *temp = NULL, *newNode , *ptr = NULL;
int n , i , value;

printf("Enter number of nodes:  ");
scanf("%d" , &n);

for(i = 0 ; i < n ; i ++ ){

newNode = (struct Node*) malloc(sizeof(struct Node));

printf("The data of %d Node : \n"  , i + 1 );
scanf("%d" , &value);

newNode -> data = value;
newNode -> next = NULL;

if(head == NULL){
head = newNode;
temp = head;
}

else {
temp->next = newNode ;
temp = newNode ;
}
}
    printf("\nLinked List: ");
    temp = head;
while(temp != NULL){
printf("%d -> " , temp->data);
temp = temp->next;
}

printf("NULL\n");

// Insertion in this 
int x , y;

printf("Enter the position after which element is to be added : ");
scanf("%d" , &x);

if(x==0){
    printf("Enter The element to be entered :");
    scanf("%d" , &y);
    head = insertAtFirst(head,y);

    //
linkedListTraversal(head);

//
    printf("\n");
    
}
int index;
if(x==n){
    printf("Enter The element to be entered : ");
    scanf("%d" , &y);
    head = insertAtLast(head,y);

//
    linkedListTraversal(head);
    printf("\n");
    //
}


if((1)<=(x<=n-1)){

    printf("Enter The element to be entered : ");
    scanf("%d" , &y);
    head = insertAtIndex(head , y , x);
    
    //
    linkedListTraversal(head);
//
    printf("\n");

}


return 0;

}