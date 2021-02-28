#include <stdio.h>
#include <stdlib.h>


struct ll_node {
    int value;
    struct ll_node *next;
};


// This overly complex code reads integers from stdin and places them in 
// a linked list. Then, it sums the items in the list and prints the result.
int main() {
    int user_inp;
    int sum;

    // Using a dummy head node
    struct ll_node *front = malloc(sizeof(struct ll_node));
    front->value = 0;
    front->next = NULL;
    struct ll_node *current = front;

    while (scanf("%d", &user_inp) != EOF) {
        current->next = malloc(sizeof(struct ll_node));
        current = current->next;

        current->value = user_inp;
        current->next = NULL;
    }

    struct ll_node *temp = front;
    for (sum = 0; front != NULL; front = front->next) {
        sum += front->value;
    }
    printf("The sum of the inputs is %d.\n", sum);

    //bug fix: freeing memeory
    struct ll_node *previous;
    while (temp != NULL) {
        previous = temp;
        temp = temp->next;
        free(previous);
    }


    return 0;
}
