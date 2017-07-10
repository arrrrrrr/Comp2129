#include "tapereader.h"

fmap_t g_mapping;

void* tape_reader(void* args) {
    thr_data_t *data = (thr_data_t *)args;

    FILE *fp = fopen(data->filename, "a");

    if (fp == NULL) {
        return (void *)1;
    }

    //printf("thread %d reporting in\n",data->id);

    char *wbuf = read_from_read_ptr(&(data->readptr), data->length, data->id);

    fwrite(wbuf, sizeof(*wbuf), abs(data->length), fp);
    fclose(fp);
    free(wbuf);

    return (void *)0;
}

// read from the pointer to the file map and update it when we're done
// returns a malloc'd buffer
char *read_from_read_ptr(char **read_ptr, int length, int id) {
    char *rp = *read_ptr;
    int abs_len = abs(length);

    // allocate a buffer to store the data we read
    char *buffer = malloc(sizeof(*buffer) * abs_len);

    // direction we read
    int dir = length / abs_len;
    int written = 0;

    // patch up rp to read backwards correctly
    if (dir < 0) {
        while (written < abs_len) {
            rp = g_mapping.map + (((((int)(rp - g_mapping.map) + dir) % (int)g_mapping.size) + (int)g_mapping.size) % (int)g_mapping.size);
            buffer[written] = *rp;
            written++;
        }

    } else {

        while (written < abs_len) {
            buffer[written] = *rp;
            // perform pointer adjustments
            rp = g_mapping.map + (((((int)(rp - g_mapping.map) + dir) % (int)g_mapping.size) + (int)g_mapping.size) % (int)g_mapping.size);

            written++;
        }

    }

    *read_ptr = rp;

    return buffer;
}

// returns a file map read pointer from 0 + offset
char *get_read_ptr(int offset) {
    char *rp = g_mapping.map;
    int size = (int)g_mapping.size;

    // calculate the actual offset of the pointer
    char *rp2 = rp + (((offset % size) + size) % size);
    //printf("size: %d\n",size);
    //printf("offset: %d, rp: %p, rp2: %p, mo: %d\n",offset,rp,rp2,(((offset % size) + size) % size));
    return rp2;
}

int main(int argc, char **argv) {
    int quit = 0;

    // no filename
    if (argc == 1) {
        printf("Tape Not Inserted\n");
        return 1;
    }

    struct stat sstat;

    // open a file descriptor
    int fd = open(argv[1], O_RDONLY);
    // file does not exist
    if (fd < 0) {
        printf("Cannot Read Tape\n");
        return 1;
    }

    // setup the stat structure
    if (fstat(fd, &sstat) == -1) {
        printf("File: %s error checking size. Exiting.\n", argv[1]);
        close(fd);
        exit(1);
    }

    // create the file mapping
    char *fmap = mmap(NULL, sstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (fmap == MAP_FAILED) {
        close(fd);
        printf("File: %s mapping failed. Exiting.\n", argv[1]);
        exit(1);
    }

    // setup the global mapping structure
    g_mapping.fd = fd;
    g_mapping.size = sstat.st_size;
    g_mapping.map = fmap;

    thr_data_t *thrdata = NULL;
    int head_count = 0;

    while (!quit) {
        char buf[1024] = {0};
        int rdbytes = 0;
        char c;
        int sep = 0;

        while ((c = getchar()) != '\n') {
            if (rdbytes < 1024) {
                if (c == ' ')
                    sep++;
                buf[rdbytes++] = c;
            }
        }

        buf[rdbytes] = '\0';

        int ctr = 0;
        char **tokens = malloc(sizeof(*tokens) * (sep+1));
        char *token = strtok(buf, " ");

        while (token != NULL) {
            tokens[ctr++] = token;
            token = strtok(NULL, " ");
        }

        if (ctr == 0) {
            free(tokens);
            continue;
        }

        if (strcmp(tokens[0], "QUIT") == 0) {
            quit = 1;

        } else if (strcmp(tokens[0], "HEAD") == 0) {
            if (ctr != 2) {
                printf("Invalid Command\n\n");
                free(tokens);
                continue;
            }

            int offset = atoi(tokens[1]);
            // increment the head count
            head_count++;

            printf("HEAD %d at %+d\n\n", head_count, offset);

            // allocate some data for the head
            if (thrdata == NULL) {
                thrdata = malloc(sizeof(*thrdata) * head_count);
            } else {
                thrdata = realloc(thrdata, sizeof(*thrdata) * head_count);
            }

            thrdata[head_count-1].id = head_count;
            thrdata[head_count-1].length = 0;
            thrdata[head_count-1].readptr = get_read_ptr(offset);

            snprintf(thrdata[head_count-1].filename, sizeof(thrdata[head_count-1].filename),
                     "./head%d", head_count );

            // check if a previous file exists
            FILE *fp = fopen(thrdata[head_count-1].filename, "r");

            // delete the file if it does
            if (fp != NULL) {
                fclose(fp);
                unlink(thrdata[head_count-1].filename);
            }

        } else if (strcmp(tokens[0], "READ") == 0) {
            if (ctr != 2) {
                printf("Invalid Command\n\n");
                free(tokens);
                continue;
            }

            if (head_count != 0) {
                int read_bytes = atoi(tokens[1]);
                //printf("read: %d, heads: %d\n",read_bytes,head_count);

                // set up how many bytes to read for each thread
                for (int i = 0; i < head_count; i++)
                    thrdata[i].length = read_bytes;

                // allocate threads
                pthread_t *threads = malloc(sizeof(*threads) * head_count);

                // create the threads
                for (int i = 0; i < head_count; i++)
                    pthread_create(&threads[i], NULL, tape_reader, (void *)&thrdata[i]);

                for (int i = 0; i < head_count; i++) {
                    int *ret = 0;
                    // join each thread
                    pthread_join(threads[i], (void*)&ret);

                    if ((int)ret != 0) {
                        printf("Something terrible happened on thread %d.\n", i);
                        quit = 1;
                    }
                }

                if (!quit) {
                    printf("Finished Reading\n\n");
                }

                // free the threads
                free(threads);
            } else {
                printf("Cannot read. No heads defined\n\n");
            }
        } else {
            printf("Invalid Command\n\n");
        }

        free(tokens);
    }

    if (thrdata != NULL)
        free(thrdata);

    // unmap the file mapping and close its fd
    munmap(g_mapping.map, g_mapping.size);
    close(g_mapping.fd);

    return 0;
}
