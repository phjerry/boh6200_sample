#ifndef SAMPLE_QUEUE_H
#define SAMPLE_QUEUE_H

typedef struct HiQueue HiQueue;

HiQueue *alloc_queue();
void free_queue(HiQueue *q);
void free_queue_and_qelement(HiQueue *q);
void *pop(HiQueue *q);
int push(HiQueue *q, void * data);
int get_size(HiQueue *q);

#endif
