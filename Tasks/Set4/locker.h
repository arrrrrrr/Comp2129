#ifndef LOCKER_H
#define LOCKER_H
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <mqueue.h>
#include <semaphore.h>
#include <time.h>

// max message on the message queue
#define MQ_MAX_MESSAGE 10

#define LOCKER_PER_RECORD 8
#define LOCKER_RECORD_MAX 1024
#define LOCKER_RECORDS (LOCKER_RECORD_MAX / LOCKER_PER_RECORD)

// special parameter for shm_msg_t.param
#define MSG_PARAM_BROADCAST 0xFFFF

// flags for shm_msg_t.flags
#define MSG_FLAG_QUERY   0x0000
#define MSG_FLAG_ATTACH  0x0001
#define MSG_FLAG_DETACH  0x0002
#define MSG_FLAG_LOCK    0x0004
#define MSG_FLAG_UNLOCK  0x0008
#define MSG_FLAG_DELETE  0x0010

// typdefs
typedef struct locker_t locker_t;
typedef struct queue_t queue_t;
typedef struct queue_item_t queue_item_t;
typedef struct shm_msg_t shm_msg_t;
typedef struct shm_control_t shm_control_t;

// shared memory control structure
struct shm_control_t {
    uint64_t msgid;
    int available; // is a msg ready to be processed
    int response;  // used for terminating slaves, signals termination in process
};

// shared memory message structure
struct shm_msg_t {
    uint16_t param;
    uint16_t flags;
    uint16_t data;
};

typedef struct {
    sem_t response_lock;
} shm_sem_t;

#define SHM_SEM_SIZE (sizeof(shm_sem_t))
#define SHM_CTL_SIZE (sizeof(shm_control_t))
#define SHM_MSG_SIZE (sizeof(shm_msg_t))
#define SHM_DATA_SIZE (SHM_CTL_SIZE+SHM_MSG_SIZE)

#define SHM_SIZE (SHM_SEM_SIZE + SHM_DATA_SIZE)

#define SHM_SEM_OFFSET 0
#define SHM_CTL_OFFSET SHM_SEM_SIZE
#define SHM_MSG_OFFSET (SHM_CTL_OFFSET + SHM_CTL_SIZE)

struct locker_t {
    uint16_t id;
    uint16_t user_id;
    uint8_t locked;
    uint8_t owned;
};

struct queue_item_t {
    void         *item;
    queue_item_t *next;
};

struct queue_t {
    queue_item_t *head;
    queue_item_t *tail;

    size_t size;
};

typedef struct {
    unsigned b0: 1;
    unsigned b1: 1;
    unsigned b2: 1;
    unsigned b3: 1;
    unsigned b4: 1;
    unsigned b5: 1;
    unsigned b6: 1;
    unsigned b7: 1;
} bits_t;

#define MQ_RCV_BUFSIZE (sizeof(struct locker_t))

void     remove_item(queue_t *q, queue_item_t *qitem);
void     enqueue_item(queue_t *q, void *item, size_t size);
int      dequeue_item(queue_t *q, void *item, size_t size);
queue_t *create_queue();
void     destroy_queue(queue_t *q);
void     set_active_locker(bits_t *store, uint16_t id, int value);
int      is_active_locker(bits_t *store, uint16_t id);
void     sig_handler(int sig, siginfo_t *siginfo, void *context);
int      cmp_locker_id(const void *a, const void *b);

#endif