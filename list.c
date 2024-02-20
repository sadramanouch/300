#include <stdio.h>
#include "list.h"

static List lists[LIST_MAX_NUM_HEADS];
static Node nodes[LIST_MAX_NUM_NODES];
static List* availableListsHead = NULL;
static Node* availableNodesHead = NULL;
static bool first_call_to_List_create = true;

//Push a list to the list of free lists.
static void pushList(List* list)
{
	//reset elements to default values
	list->head = NULL;
	list->last = NULL;
	list->current = NULL;
	list->currentState = LIST_OOB_START;
	list->count = 0;

	//push the list
	list->nextList = availableListsHead;
	availableListsHead = list;
}

//Return a pointer to a fresh list and remove it from the free lists list.
//If no lists are available, return NULL.
static List* popList()
{
	if (!availableListsHead) {
		return NULL;
	}

	List* list = availableListsHead;
	availableListsHead = availableListsHead->nextList;
	list->nextList = NULL;
	return list;
}

//Push a node to the list of free nodes.
static void pushNode(Node* node)
{
	//reset elements to default values
	node->prev = NULL;
	node->item = NULL;

	//push the node
	node->next = availableNodesHead;
	availableNodesHead = node;
}

//Return a pointer to a fresh node and remove it from the free nodes list.
//If no nodes are available, return NULL.
static Node* popNode()
{
	if (!availableNodesHead) {
		return NULL;
	}

	Node* node = availableNodesHead;
	availableNodesHead = availableNodesHead->next;
	node->next = NULL;
	return node;
}

//Initializes the list of free lists and the list of free nodes
//by pushing all of the List*s and Node*s to their respecitve lists.
static void initializeAvailableLists()
{
	for (int i = 0; i < LIST_MAX_NUM_HEADS; i++) {
		List* list = &lists[i];
		pushList(list);
	}
	for (int i = 0; i < LIST_MAX_NUM_NODES; i++) {
		Node* node = &nodes[i];
		pushNode(node);
	}
}

List* List_create() 
{
	if (first_call_to_List_create) {
		initializeAvailableLists();
		first_call_to_List_create = false;
	}

	return popList(); //this will return NULL if there are no lists available
}

int List_count(List* pList)
{
	return pList->count;
}

void* List_first(List* pList) 
{
	if (pList->count == 0) {
		pList->current = NULL;
		pList->currentState = LIST_OOB_START;
		return NULL;
	}

	pList->current = pList->head;
	pList->currentState = IN_LIST;
	return pList->head->item;
}

void* List_last(List* pList) 
{
	if (pList->count == 0) {
		pList->current = NULL;
		pList->currentState = LIST_OOB_END;
		return NULL;
	}

	pList->current = pList->last;
	pList->currentState = IN_LIST;
	return pList->last->item;
}

void* List_next(List* pList)
{
	int state = pList->currentState;

	if (state == IN_LIST && pList->current->next) {
		pList->current = pList->current->next;
	}
	else if (state == LIST_OOB_START && pList->head) {
		pList->current = pList->head;
	}
	else { //state == LIST_OOB_END or next is OOB or pList is empty
		pList->current = NULL;
		pList->currentState = LIST_OOB_END;
		return NULL;
	}
	
	return pList->current->item;
}

void* List_prev(List* pList) 
{
	int state = pList->currentState;

	if (state == IN_LIST && pList->current->prev) {
		pList->current = pList->current->prev;
	}
	else if (state == LIST_OOB_END && pList->last) {
		pList->current = pList->last;
	}
	else { //state == LIST_OOB_START or prev is OOB or pList is empty
		pList->current = NULL;
		pList->currentState = LIST_OOB_START;
		return NULL;
	}

	return pList->current->item;
}

void* List_curr(List* pList) 
{
	if (pList->currentState == IN_LIST){
		return pList->current->item;
	}
	return NULL;
}

int List_insert_after(List* pList, void* pItem) 
{
	int state = pList->currentState;

	//for OOB states we can use the append or prepend functions instead
	if (state == LIST_OOB_END) {
		return List_append(pList, pItem);
	}
	else if (state == LIST_OOB_START) {
		return List_prepend(pList, pItem);
	}

	//get the node, initialize values, insert into pList
	Node* newNode = popNode();
	if (!newNode) { //no nodes are available
		return -1;
	}
	Node* next = pList->current->next;
	newNode->item = pItem;
	newNode->next = next;
	newNode->prev = pList->current;
	pList->current->next = newNode;
	pList->current = newNode;
	pList->count++;

	if (next) { //there is a node in front of newNode in the list
		next->prev = newNode;
	}
	else { //the new node is the last node
		pList->last = newNode;
	}
	
	return 0;
}

int List_insert_before(List* pList, void* pItem) 
{
	int state = pList->currentState;

	//for OOB states we can use the append or prepend functions instead
	if (state == LIST_OOB_END) {
		return List_append(pList, pItem);
	}
	else if (state == LIST_OOB_START) {
		return List_prepend(pList, pItem);
	}

	//get the node, initialize values, insert into pList
	Node* newNode = popNode();
	if (!newNode) { //no nodes are available
		return -1;
	}
	Node* prev = pList->current->prev;
	newNode->item = pItem;
	newNode->next = pList->current;
	newNode->prev = prev;
	pList->current->prev = newNode;
	pList->current = newNode;
	pList->count++;

	if (prev) { //there is a node behind newNode in the list
		prev->next = newNode;
	}
	else { //the new node is the first node
		pList->head = newNode;
	}

	return 0;
}

int List_append(List* pList, void* pItem) 
{
	Node* newNode = popNode();
	if (!newNode) {
		return -1;
	}
	newNode->item = pItem;

	if (pList->count == 0) { //empty list case
		pList->head = newNode;
	}
	else { //list is not empty
		pList->last->next = newNode;
		newNode->prev = pList->last;
	}

	pList->last = newNode;

	//set current and increment the count
	pList->current = newNode;
	pList->currentState = IN_LIST;
	pList->count++;
	return 0;
}

int List_prepend(List* pList, void* pItem) 
{
	Node* newNode = popNode();
	if (!newNode) {
		return -1;
	}
	newNode->item = pItem;

	if (pList->count == 0) { //empty list case
		pList->last = newNode;
	}
	else { //list is not empty
		pList->head->prev = newNode;
		newNode->next = pList->head;
	}

	pList->head = newNode;

	//set current and increment the count
	pList->current = newNode;
	pList->currentState = IN_LIST;
	pList->count++;
	return 0;
}

void* List_remove(List* pList)
{
	if (pList->currentState != IN_LIST) { //current is outside of the list
		return NULL;
	}

	Node* current = pList->current;
	void* item = current->item;

	if (current->prev) {
		current->prev->next = current->next;
	}
	else { // the node we are removing is the head, so update head
		pList->head = current->next;
	}

	if (current->next) {
		current->next->prev = current->prev;
	}
	else { //current->next is NULL, state is OOB, update last
		pList->currentState = LIST_OOB_END;
		pList->last = current->prev;
	}
	pList->current = current->next;

	//free the node
	pushNode(current);
	pList->count--;

	return item;
}

void* List_trim(List* pList)
{
	if(pList->count == 0){
		return NULL;
	}

	pList->current = pList->last;
	pList->currentState = IN_LIST;
	
	Node* current = pList->current;
	void* item = current->item;

	//remove the node from the list
	if (current->prev) {
		current->prev->next = NULL;
		pList->last = current->prev;
	}
	else {
		pList->currentState = LIST_OOB_END;
		pList->head = NULL;
		pList->last = NULL;
	}
	pList->current = current->prev;

	//free the node
	pushNode(current);
	pList->count--;

	return item;
}

void List_concat(List* pList1, List* pList2)
{
	//link pList1->last to pList2->head
	if (pList2->head) {
		if (pList1->head) {
			pList1->last->next = pList2->head;
			pList2->head->prev = pList1->last;
		}
		else {
			pList1->head = pList2->head;
		}
	}
	pList1->last = pList2->last;
	pList1->count += pList2->count;
	
	//free pList2
	pushList(pList2);
}

void List_free(List* pList, FREE_FN pItemFreeFn)
{
	pList->current = pList->head;

	//free every node in the list
	while(pList->current) {
		(*pItemFreeFn)(pList->current->item);
		Node* next = pList->current->next;
		pushNode(pList->current);
		pList->current = next;
	}

	//free the list
	pushList(pList);
}

void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg)
{
	//current is before the start of the list so start at the head
	if (pList->head && pList->currentState == LIST_OOB_START) {
		pList->current = pList->head;
		pList->currentState = IN_LIST;
	}

	//search for a matching item, return a pointer to it if found
	while (pList->current) {
		void* item = pList->current->item;
		if ((*pComparator)(item, pComparisonArg)) {
			return item;
		}

		pList->current = pList->current->next;
	}

	//a match was not found so the end of the list was reached for current
	pList->currentState = LIST_OOB_END;
	return NULL;
}
