#include "sample_queue.h"
#include <stdio.h>
#include <stdlib.h>


typedef struct HiNode
{
  void *data;
  struct HiNode *next;
} HiNode;

struct HiQueue
{
  HiNode *head;
  HiNode *tail;
  int  current_size;
};

HiQueue *alloc_queue()
{
  HiQueue *q = (HiQueue *) malloc(sizeof(HiQueue));
  if (q)
  {
    q->head = q->tail = NULL;
    q->current_size = 0;
  }
  return q;
}

void free_queue(HiQueue *q)
{
  while (q->current_size)
  {
    pop(q);
  }
  free(q);    //add by liminqi
}

void free_queue_and_qelement(HiQueue *q)
{
  while (q->current_size)
  {
    void *data = pop(q);
    if (data)
      free(data);
  }
}

void *pop(HiQueue *q)
{
  HiNode *temp;
  void *data;

  if (q->current_size == 0)
    return NULL;

  temp = q->head;
  data = temp->data;

  if (q->current_size == 1)
  {
    q->head = q->tail = NULL;
  }
  else
  {
    q->head = q->head->next;
  }

  free(temp);
  q->current_size--;
  return data;
}

int push(HiQueue *q, void * data)
{
  HiNode *new_node = (HiNode *) malloc(sizeof(HiNode));

  if (new_node == NULL)
    return -1;

  new_node->data = data;
  new_node->next = NULL;

  if (q->current_size == 0)
  {
    q->head = new_node;
  }
  else
  {
    q->tail->next = new_node;
  }

  q->tail = new_node;
  q->current_size++;

  return 0;
}

int get_size(HiQueue *q)
{
    if (NULL == q)
        return -1;

    else
        return q->current_size;
}

