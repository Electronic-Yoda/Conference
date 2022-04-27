#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>



typedef struct ListNode {
    char* str;
    struct ListNode* next;
}ListNode;

ListNode* listNode_init() {
    ListNode* listNode = (ListNode*)malloc(sizeof(ListNode));
    listNode->str = NULL;
    listNode->next = NULL;
    return listNode;
}
ListNode* listNode_initAll(char* str) {
    ListNode* listNode = (ListNode*)malloc(sizeof(ListNode));
    listNode->str = (char*)malloc(strlen(str)+1);
    strncpy(listNode->str, str, strlen(str)+1);
    listNode->next = NULL;
    return listNode;
}



typedef struct List {
    ListNode* head;
}List;



List* list_init() {
    List* list = (List*)malloc(sizeof(List));
    //list->head = listNode_init(); Wrong!
    list->head = NULL;
    return list;
}

int list_insert(List* list, ListNode* node) {
    if (node == NULL) return 0;
    
    node->next = list->head;
    list->head = node;
    return 1;
}


// returns 1 if deleted, 0 if not found or error
int list_deleteNode(List* list, char* name) {
    

    if (list->head == NULL) return 0;
    if (strcmp(list->head->str, name) == 0) {
      if (list->head->next == NULL) {
          free(list->head->str);
          free(list->head);
          list->head = NULL;
          return 1;
      }
      else {
          //found is head but next is not NULL
          ListNode* tmp = list->head;
          list->head = list->head->next;
          free(tmp->str);
          free(tmp);
          tmp = NULL;
          return 1;
      }
    }
    
    ListNode* cur = list->head->next;
    ListNode* prev = list->head;
    for (; cur!= NULL; prev = cur, cur = cur->next) {
        if (strcmp(cur->str, name) == 0) {
            prev->next = cur->next;
			free(cur->str);
            free(cur);
            cur = NULL;
            return 1;
        }
    }
    return 0; // not found in linked list
}

// Returns null if not found, else returns the hashNode
ListNode* list_find(List* list, char* name){

    ListNode* tmp = list->head;
    

    if (tmp == NULL) return tmp;
    int i = 0;
    
    
    while (tmp != NULL) {
		if (strcmp(tmp->str, name) == 0) break;
		tmp = tmp->next;
	}
    return tmp;
}


void list_delete(List** list)
{
    ListNode* temp1; 
    ListNode* tempNext;
    temp1 = (*list)->head;
    tempNext = NULL;

    while( temp1 != NULL )
    {
        tempNext = temp1->next;
        free(temp1->str);
        free(temp1);
        temp1 = tempNext;
    }
    //(*list)->head = NULL;
    free(*list);
    *list = NULL;
}

void list_print(List* list) {
    if (list == NULL) {
        puts("list_print(): list is not initialized");
        return;
    }
    printf("Start\n");
    
	ListNode* tmp = list->head;
	while (tmp != NULL ) {
			
		printf("%s - ", tmp->str);
		
		tmp = tmp->next;
	}
    printf("\nEnd\n");        
}


#endif



