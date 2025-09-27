#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#define BUFF_SIZE 102
#define MAX_SIZE 100 

typedef struct ListNode
{
	char* val; 
	struct ListNode* next; 
} ListNode; 

void PrintStrings (ListNode* head)  
{
	int i = 0;  
	ListNode* node = head; 
	while (node != NULL)
	{
		printf("String %d: ", i);
		fputs(node->val, stdout); 
		printf("\n"); 
		node = node->next;
		i += 1;  
	}
}

void ClearList (ListNode* head)
{
	ListNode* curr_node = head;
	ListNode* last_node; 
	while (curr_node != NULL)
	{
		last_node = curr_node; 
		curr_node = last_node->next;
		free(last_node->val);   
		free(last_node);
	}
}

int main()
{
	char buffer[BUFF_SIZE]; 
	fgets(buffer, BUFF_SIZE, stdin); 
	ListNode* head = NULL; 
	ListNode* last_node; 
	ListNode* curr_node; 
	int null_sig = 0; 
	while (buffer[0] != '.')
	{
		buffer[MAX_SIZE] = '\0';
		if (buffer[strnlen(buffer, MAX_SIZE) - 1] == '\n')
			buffer[strnlen(buffer, MAX_SIZE) - 1] = '\0'; // set '\n' to '\0' 
		if (buffer[0] == '\0')
			strcpy(buffer, "**EMPTY STRING**"); 
		if (head == NULL) 
		{
			head = (ListNode*) malloc (sizeof(ListNode)); 
			if (head == NULL) {
				null_sig = 1; 
				break; 
			} 
			head->val = (char*) malloc (strnlen(buffer, MAX_SIZE) + 1); 
			if (head->val == NULL) {
				null_sig = 1; 
				break; 
			}
			strcpy(head->val, buffer); 
			head->next = NULL; 
			last_node = head; 
		}
		else 
		{
			curr_node = (ListNode*) malloc (sizeof(ListNode)); 
			if (curr_node == NULL) {
				null_sig = 1; 
				break; 
			}
			last_node->next = curr_node; 
			curr_node->val = (char*) malloc (strnlen(buffer, MAX_SIZE) + 1);
			if (curr_node == NULL) {
				null_sig = 1; 
				break; 
			}
			strcpy(curr_node->val, buffer); 
			curr_node->next = NULL; 
			last_node = curr_node; 
		}
		fgets(buffer, MAX_SIZE + 1, stdin);
	}
	if (null_sig) {
		printf("Cannot allocate memory: NULL is returned\n"); 
		ClearList(head); 
		return 0; 
	}
	printf("\n####\n");
	printf("Strings");
	printf("\n####\n");
	PrintStrings(head); 
	printf("####\n"); 
	ClearList(head); 
	printf("\n...Memory has been cleaned...\n");
	return 0; 
}