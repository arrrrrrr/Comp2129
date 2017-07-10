/*
 * Copyright (c) 2017 Michael Zammit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "locker.h"

// locker data
locker_t locker_data = {0};
uint64_t msg_ctr = 0;
/*
CREATE - Creates a locker
DELETE <id : locker id> - Decommissions a locker
QUERY <id : locker id> - Queries a locker and retrieves information
QUERYALL - Queries all lockers and retrieve their information
LOCK <id : locker id> - Locks a locker
UNLOCK <id : locker id> - Unlocks a locker
ATTACH <owner> - Adds an owner to a locker, gets locker at head of the queue
DETACH <id : locker id> - Removes an owner from a locker
QUIT - Deletes all lockers and quits the program
*/

void send_message(char *shm_base, uint16_t param, uint16_t flags, uint16_t data,
                  struct timespec *tm, long nsec) {
    shm_msg_t msg_data      = { param, flags, data };
    shm_control_t ctl_data  = { msg_ctr++, 1, 0 };

    // first set up the message
    memcpy(shm_base + SHM_MSG_OFFSET, &msg_data, sizeof(msg_data));
    // now signal the slaves
    memcpy(shm_base + SHM_CTL_OFFSET, &ctl_data, sizeof(ctl_data));

    if (tm != NULL) {
        clock_gettime(CLOCK_REALTIME, tm);
        tm->tv_nsec += nsec;
    }
}

void remove_item(queue_t *q, queue_item_t *qitem) {
    queue_item_t *curr = q->head;
    queue_item_t *prev = NULL;

    while (curr != NULL) {
        if (curr == qitem) {
            queue_item_t *next = curr->next;
            q->size--;

            // in the tail position
            if (next == NULL) {
                q->tail = prev;

                if (q->tail != NULL)
                    q->tail->next = NULL;
                if (q->size == 1)
                    q->head = q->tail;
                if (q->size == 0)
                    q->head = q->tail = NULL;

                free(curr->item);
                free(curr);
                // done
                break;
            }

            // in the head position
            if (prev == NULL) {
                if (curr->next == NULL)
                    q->head = q->tail = prev;
                else
                    q->head = curr->next;

                free(curr->item);
                free(curr);
                // done
                break;
            }

            // removing a node between two others
            prev->next = next;
            // free the current node
            free(curr->item);
            free(curr);
            break;
        }

        prev = curr;
        curr = curr->next;
    }
}

void enqueue_item(queue_t *q, void *item, size_t size) {
    // create a queue item
    queue_item_t *tmp = malloc(sizeof(*tmp));
    tmp->item = malloc(size);
    // copy the item
    memcpy(tmp->item, item, size);

    tmp->next = NULL;

    // append it to the end of the queue
    if (q->size == 0) {
        q->head = q->tail = tmp;
    } else {
        q->tail->next = tmp;
        q->tail = tmp;
    }

    // enqueued an item
    q->size++;
}

int dequeue_item(queue_t *q, void *item, size_t size) {
    if (q->size == 0)
        return 0;

    queue_item_t *tmp = q->head;
    q->size--;
    q->head = tmp->next;

    // copy the item
    memcpy(item, tmp->item, size);
    // free the item
    free(tmp->item);
    // free the queue item
    free(tmp);

    // queue is now empty
    if (q->size == 0)
        q->head = q->tail = NULL;

    return 1;
}

queue_t *create_queue() {
    queue_t *q = malloc(sizeof(*q));
    q->head = q->tail = NULL;
    q->size = 0;

    return q;
}

void destroy_queue(queue_t *q) {
    if (q != NULL) {
        queue_item_t *curr = q->head;

        while (curr != NULL) {
            queue_item_t *tmp = curr->next;

            if (curr->item != NULL)
                free(curr->item);

            free(curr);
            curr = tmp;
        }

        free(q);
    }
}

int cmp_locker_id(const void *a, const void *b) {
    locker_t *pa = (locker_t *)a;
    locker_t *pb = (locker_t *)b;

    return pa->id - pb->id;
}

void sig_handler(int sig, siginfo_t *siginfo, void *context) {
    if (sig == SIGUSR1) {
        locker_data.locked = 1;
    } else if (sig == SIGUSR2) {
        locker_data.locked = 0;
    }
}

int is_active_locker(bits_t *store, uint16_t id) {
    int idx = id / LOCKER_PER_RECORD;
    int mod = id % LOCKER_PER_RECORD;

    switch(mod) {
        case 0: return store[idx].b0 == 1;
        case 1: return store[idx].b1 == 1;
        case 2: return store[idx].b2 == 1;
        case 3: return store[idx].b3 == 1;
        case 4: return store[idx].b4 == 1;
        case 5: return store[idx].b5 == 1;
        case 6: return store[idx].b6 == 1;
        case 7: return store[idx].b7 == 1;
    }

    return 0;
}

void set_active_locker(bits_t *store, uint16_t id, int value) {
    int idx = id / LOCKER_PER_RECORD;
    int mod = id % LOCKER_PER_RECORD;

    switch(mod) {
        case 0: store[idx].b0 = value; break;
        case 1: store[idx].b1 = value; break;
        case 2: store[idx].b2 = value; break;
        case 3: store[idx].b3 = value; break;
        case 4: store[idx].b4 = value; break;
        case 5: store[idx].b5 = value; break;
        case 6: store[idx].b6 = value; break;
        case 7: store[idx].b7 = value; break;
    }
}

void do_child_loop(char *shm_base, mqd_t *msg_queue) {
    // signal action struct
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    // set the sa_siginfo flag
    sa.sa_flags |= SA_SIGINFO;
    sa.sa_sigaction = &sig_handler;

    // bind the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
        perror("sigaction failed for SIGUSR1");

    // bind the signal handler for SIGUSR2
    if (sigaction(SIGUSR2, &sa, NULL) < 0)
        perror("sigaction failed for SIGUSR2");

    int last_msg_status = 0;
    uint64_t last_msg_id = -1;

    int cleanup = 0;

    // pointer to the shm_control_t.available
    shm_control_t *shm_status = (shm_control_t *)(shm_base + SHM_CTL_OFFSET);

    // loop
    while (!cleanup) {
        while (shm_status->available == last_msg_status && shm_status->msgid == last_msg_id) {
            // sleep for 50ms
            usleep(1000);
        }

        // status changed
        last_msg_status = shm_status->available;
        last_msg_id = shm_status->msgid;

        // no msg waiting
        if (!last_msg_status)
            continue;

        shm_msg_t msg_data;
        // store the message
        memcpy(&msg_data, shm_base + SHM_MSG_OFFSET, sizeof(shm_msg_t));

        // are we interested in the message
        if (msg_data.param == locker_data.id ||
            msg_data.param == MSG_PARAM_BROADCAST) {
            int lstatus = 0;

            //printf("slave %d received messaged: param=%d, id=%d, data = %d\n", locker_data.id, msg_data.param, msg_data.flags, msg_data.data);

            // what have we got here
            switch(msg_data.flags) {
                // message to query the locker
                case MSG_FLAG_QUERY:
                    // send a message to the message queue
                    mq_send(*msg_queue, (char *)&locker_data, sizeof(locker_t), 1);
                    break;
                // message to attach the locker
                case MSG_FLAG_ATTACH:
                    locker_data.user_id = msg_data.data;
                    locker_data.owned = 1;
                    // set the ok status
                    lstatus = 1;
                    // send a message to the message queue
                    mq_send(*msg_queue, (char *)&lstatus, sizeof(lstatus), 1);
                    break;
                // message to detach the locker
                case MSG_FLAG_DETACH:
                    if (locker_data.owned == 0) {
                        lstatus = 0;
                    } else {
                        locker_data.user_id = 0xffff;
                        locker_data.owned = 0;
                        // set the ok status
                        lstatus = 1;
                    }
                    // send a message to the message queue
                    mq_send(*msg_queue, (char *)&lstatus, sizeof(lstatus), 1);
                    break;
                // message to lock the locker
                case MSG_FLAG_LOCK:
                    locker_data.locked = 1;
                    // set the ok status
                    lstatus = 1;
                    // send a message to the message queue
                    mq_send(*msg_queue, (char *)&lstatus, sizeof(lstatus), 1);
                    break;
                // message to unlock the locker
                case MSG_FLAG_UNLOCK:
                    locker_data.locked = 0;
                    // set the ok status
                    lstatus = 1;
                    // send a message to the message queue
                    mq_send(*msg_queue, (char *)&lstatus, sizeof(lstatus), 1);
                    break;
                // message to delete the locker
                case MSG_FLAG_DELETE:
                    cleanup = 1;
                    break;
            }
        }
    }
}

int main(void) {
    int quit = 0;
    int active_lockers = 0;
    int is_child = 0;
    uint16_t next_locker_id = 1;

    // name of the shared memory block
    const char *shm_name = "/tmp/shm-locker";
    // name of the message queue
    const char *mq_name = "/mq-locker";

    int   shm_fd;
    char *shm_base, *shm_data;

    // keep track of lockers
    bits_t locker_store[LOCKER_RECORDS];
    memset(locker_store, 0, sizeof(bits_t)*LOCKER_RECORDS);

    // create the shared memory block
    shm_fd = open(shm_name, O_CREAT | O_RDWR, 0666);

    // create failed
    if (shm_fd == -1) {
        printf("Master: open failed for %s, error: %s\n", shm_name, strerror(errno));
        exit(1);
    }

    /* configure the size of the shared memory segment */
    ftruncate(shm_fd, SHM_SIZE);
    /* map the shared memory segment to the address space of the process */
    shm_base = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // pointer to the data region of shared memory
    shm_data = shm_base + SHM_CTL_OFFSET;
    // mapping failed
    if (shm_base == MAP_FAILED) {
        printf("Master: mmap failed for %s, error: %s\n", shm_name, strerror(errno));
        close(shm_fd);

        if (unlink(shm_name) == -1)
            printf("Master: shm_unlink failed for %s, error: %s\n", shm_name, strerror(errno));

        exit(1);
    }

    // message queue attributes
    struct mq_attr mqa;
    // fill out the message queue attr struct
    mqa.mq_msgsize = sizeof(locker_t);
    mqa.mq_maxmsg  = MQ_MAX_MESSAGE;
    // try to create the message queue
    mqd_t msg_queue = mq_open(mq_name, O_RDWR | O_CREAT, 0666, &mqa);

    if (msg_queue == -1) {
        printf("Master: mq_open failed for %s, error: %s\n",
                mq_name, strerror(errno));
        perror("mq_open failed\n");
        // cleanup
        munmap(shm_base, SHM_SIZE);
        close(shm_fd);
        unlink(shm_name);
    }

    // semaphore for writer locking on the response value in shared memory
    sem_t *response_wr_lock = (sem_t *)(shm_base + SHM_SEM_OFFSET);
    // initialize a semaphore for write lock on shm_control_t.response
    sem_init(response_wr_lock, 1, 1);
    // create the locker queue
    queue_t *locker_queue = create_queue();

    while (!quit) {
        char buf[1024] = {0}, c;
        int bufread = 0, tokens = 0;

        while ((c = getchar()) != '\n') {
            if (bufread < 1024) {
                if (c == ' ')
                    tokens++;

                buf[bufread++] = c;
            }
        }

        if (bufread == 0)
            continue;

        int curr_token    = 0;
        char **token_list = malloc(sizeof(*token_list) * (tokens+1));
        char *tok         = strtok(buf, " ");

        while (tok != NULL) {
            token_list[curr_token++] = tok;
            tok  = strtok(NULL, " ");
        }

        // clear the shared memory
        memset(shm_data, 0, SHM_DATA_SIZE);

        shm_control_t *ctl = (shm_control_t *)shm_data;
        ctl->msgid         = -1;

        if (strcmp(token_list[0],"CREATE") == 0) {
            // short wrapped around to 0
            if (next_locker_id == 0) {
                printf("Cannot Build Locker\n\n");
                free(token_list);
                continue;
            }

            locker_data.id      = next_locker_id++;
            locker_data.user_id = 0xffff;
            locker_data.locked  = 1;
            locker_data.owned   = 0;

            pid_t fork_pid;
            fflush(stdout);

            // fork failed
            if ((fork_pid = fork()) < 0) {
                int _err = errno;

                if (_err == EAGAIN || _err == ENOMEM) {
                    printf("Cannot Build Locker\n\n");
                    fflush(stdout);
                    // undo the next locker id
                    next_locker_id--;
                    // cleanup the token list
                    free(token_list);
                    continue;
                }
            }

            // we're the child process now so break the while loop
            if (fork_pid == 0) {
                is_child = 1;
                free(token_list);
                break;
            }

            // set the locker to be active
            set_active_locker(locker_store, locker_data.id, 1);
            // increment active lockers
            active_lockers++;

            // back to parent, enqueue the locker id
            enqueue_item(locker_queue, &locker_data.id, sizeof(locker_data.id));
            printf("New Locker Created: %d\n\n", locker_data.id);

            // clear the locker data
            memset(&locker_data, 0, sizeof(locker_data));

        } else if (strcmp(token_list[0],"DELETE") == 0) {
            if (tokens < 1) {
                printf("Invalid Command.\n\n");
                free(token_list);
                continue;
            }

            uint16_t locker_id = (uint16_t)atoi(token_list[1]);

            // this case requires no further checking
            if (locker_id < 1 || locker_id >= next_locker_id ||
                active_lockers == 0 || !is_active_locker(locker_store, locker_id)) {
                printf("Locker Does Not Exist\n\n");
            } else {
                send_message(shm_base, locker_id, MSG_FLAG_DELETE, 0, NULL, 0L);

                // check if the locker exists in the queue
                queue_item_t *curr = locker_queue->head;

                while (curr != NULL) {
                    uint16_t *item = (uint16_t *)curr->item;
                    // found the locker id so remove it from the queue
                    if (*item == locker_id) {
                        remove_item(locker_queue, curr);
                        break;
                    }

                    curr = curr->next;
                }
                // pointer to the shm_control_t.available
                shm_control_t *shm_ctl_ptr = (shm_control_t *)(shm_base+SHM_CTL_OFFSET);

                while (shm_ctl_ptr->response < 1) {
                    // sleep for 50ms
                    usleep(1000*50);
                }

                // decrement the number of active lockers
                active_lockers--;
                // disable the locker
                set_active_locker(locker_store, locker_id, 0);

                printf("Locker %d Removed\n", locker_id);
            }

        } else if (strcmp(token_list[0],"QUERY") == 0) {
            if (tokens < 1) {
                printf("Invalid Command.\n\n");
                free(token_list);
                continue;
            }

            uint16_t locker_id = (uint16_t)atoi(token_list[1]);

            // this case requires no further checking
            if (locker_id < 1 || locker_id >= next_locker_id ||
                active_lockers == 0 || !is_active_locker(locker_store, locker_id)) {
                printf("Locker Does Not Exist\n\n");
            } else {
                send_message(shm_base, locker_id, MSG_FLAG_QUERY, 0, NULL, 0L);

                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) >= 0) {
                    locker_t *l = (locker_t *)buf;

                    // print stuff
                    printf("Locker ID: %d\n", l->id);
                    printf("Lock Status: %s\n", (l->locked) ? "locked" : "unlocked");

                    if (l->owned)
                        printf("Owner: %d\n\n", l->user_id);
                    else
                        printf("Owner: unowned\n\n");
                }
            }

        } else if (strcmp(token_list[0],"QUERYALL") == 0) {
            if (active_lockers != 0) {
                send_message(shm_base, MSG_PARAM_BROADCAST, MSG_FLAG_QUERY, 0, NULL, 0L);
                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                int qry_received = 0;
                // malloc enough for all the lockers
                locker_t *lquery = malloc(sizeof(locker_t) * active_lockers);

                while (qry_received < active_lockers) {
                    if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) == sizeof(locker_t)) {
                        memcpy(&lquery[qry_received], buf, sizeof(locker_t));
                        qry_received++;
                    }
                }

                // sort the results
                qsort(lquery, active_lockers, sizeof(locker_t), cmp_locker_id);

                for (int i = 0; i < active_lockers; i++) {
                    // print stuff
                    printf("Locker ID: %d\n", lquery[i].id);
                    printf("Lock Status: %s\n", (lquery[i].locked) ? "locked" : "unlocked");

                    if (lquery[i].owned)
                        printf("Owner: %d\n\n", lquery[i].user_id);
                    else
                        printf("Owner: unowned\n\n");
                }

                free(lquery);
            } else {
                printf("\n");
            }

        } else if (strcmp(token_list[0],"LOCK") == 0) {
            if (tokens < 1) {
                printf("Invalid Command.\n\n");
                free(token_list);
                continue;
            }

            uint16_t locker_id = (uint16_t)atoi(token_list[1]);
            // this case requires no further checking
            if (locker_id < 1 || locker_id >= next_locker_id ||
                active_lockers == 0 || !is_active_locker(locker_store, locker_id)) {
                printf("Locker Does Not Exist\n\n");
            } else {
                //printf("sending message to locker: %d\n", locker_id);
                send_message(shm_base, locker_id, MSG_FLAG_LOCK, 0, NULL, 0L);
                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) >= 0) {
                    //printf("got a response: %d\n", *(int *)buf);
                    if (*(int *)buf) {
                        printf("Locker %d locked\n\n", locker_id);
                    }
                }
            }

        } else if (strcmp(token_list[0],"UNLOCK") == 0) {
            if (tokens < 1) {
                printf("Invalid Command.\n\n");
                free(token_list);
                continue;
            }

            uint16_t locker_id = (uint16_t)atoi(token_list[1]);

            // this case requires no further checking
            if (locker_id < 1 || locker_id >= next_locker_id ||
                active_lockers == 0 || !is_active_locker(locker_store, locker_id)) {
                printf("Locker Does Not Exist\n\n");
            } else {
                send_message(shm_base, locker_id, MSG_FLAG_UNLOCK, 0, NULL, 0L);
                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) >= 0) {
                    if (*(int *)buf) {
                        printf("Locker %d unlocked\n\n", locker_id);
                    }
                }
            }

        } else if (strcmp(token_list[0],"ATTACH") == 0) {
            // queue is empty
            if (locker_queue->size == 0) {
                printf("No Lockers Available\n\n");
            // attached a locker from the queue
            } else {
                if (tokens < 1) {
                    printf("Invalid Command.\n\n");
                    free(token_list);
                    continue;
                }
                uint16_t owner_id = (uint16_t)atoi(token_list[1]);

                uint16_t locker_id;
                // dequeue the locker id
                dequeue_item(locker_queue, &locker_id, sizeof(locker_id));

                send_message(shm_base, locker_id, MSG_FLAG_ATTACH, owner_id, NULL, 0L);
                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) >= 0) {
                    if (*(int *)buf) {
                        printf("Locker %d Owned By %d\n\n", locker_id, owner_id);
                    }
                }
            }

        } else if (strcmp(token_list[0],"DETACH") == 0) {
            if (tokens < 1) {
                printf("Invalid Command.\n\n");
                free(token_list);
                continue;
            }

            uint16_t locker_id = (uint16_t)atoi(token_list[1]);

            // this case requires no further checking
            if (locker_id < 1 || locker_id >= next_locker_id ||
                active_lockers == 0 || !is_active_locker(locker_store, locker_id)) {
                printf("Locker Does Not Exist\n\n");
            } else {
                send_message(shm_base, locker_id, MSG_FLAG_DETACH, 0, NULL, 0L);
                // set up the receive for the message
                char buf[MQ_RCV_BUFSIZE];

                if (mq_receive(msg_queue, buf, MQ_RCV_BUFSIZE, 0) >= 0) {
                    if (*(int *)buf)
                        enqueue_item(locker_queue, &locker_id, sizeof(locker_id));

                    printf("Locker %d Unowned\n\n", locker_id);
                }
            }

        } else if (strcmp(token_list[0],"QUIT") == 0) {
            // broadcast to every slave to shutdown
            send_message(shm_base, MSG_PARAM_BROADCAST, MSG_FLAG_DELETE, 0, NULL, 0L);
            quit = 1;

            free(token_list);
            break;
        } else {
            printf("Invalid Command.\n\n");
        }

        free(token_list);
    }

    if (is_child) {
        // perform cleanup of mallocs
        if (locker_queue != NULL)
            destroy_queue(locker_queue);

        // run the child process loop to listen for commands
        do_child_loop(shm_base, &msg_queue);

        //printf("child %d: waiting on semaphore\n", locker_data.id);
        // lock the response semaphore
        sem_wait(response_wr_lock);
        //printf("child %d: locked semaphore\n", locker_data.id);
        // take a pointer to the response
        shm_control_t *shm_ctl_ptr = (shm_control_t *)(shm_base + SHM_CTL_OFFSET);
        // we've responded to a cleanup
        shm_ctl_ptr->response++;
        // unlock the response semaphore
        sem_post(response_wr_lock);
        //printf("child %d: releasing semaphore\n", locker_data.id);

        // unmap the shared memory
        if (munmap(shm_base, SHM_SIZE) == -1) {
            printf("Slave %d: munmap failed for %s, error: %s\n",
                    locker_data.id, shm_name, strerror(errno));
            exit(1);
        }

        // close the shared memory fd
        if (close(shm_fd) == -1) {
            printf("Slave %d: close failed for %s, error: %s\n",
                    locker_data.id, shm_name, strerror(errno));
            exit(1);
        }

        if (mq_close(msg_queue) == -1) {
            printf("Slave %d: mq_close failed for %s, error: %s\n",
                    locker_data.id, mq_name, strerror(errno));
            exit(1);
        }

        return 0;
    }

    // pointer to the shm_control_t.available
    shm_control_t *shm_status = (shm_control_t *)(shm_base + SHM_CTL_OFFSET);

    while (shm_status->response < active_lockers)
        usleep(10*1000); // sleep for 10ms

    sem_wait(response_wr_lock);
    // cleanup the semaphore
    sem_destroy(response_wr_lock);
    // unmap the shared memory
    munmap(shm_base, SHM_SIZE);
    // close the shared memory fd
    close(shm_fd);

    if (unlink(shm_name) == -1) {
        printf("Master: shm_unlink failed for %s, error: %s\n", shm_name, strerror(errno));
    }

    if (mq_close(msg_queue) == 0) {
        if (mq_unlink(mq_name) == -1) {
            printf("Master: mq_unlink failed for %s, error: %s\n", mq_name, strerror(errno));
        }
    }
    // cleanup
    destroy_queue(locker_queue);

    return 0;
}