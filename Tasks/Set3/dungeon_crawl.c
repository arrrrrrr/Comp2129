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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX 8193
#define BUFFER_MAX 256

typedef struct room_t room_t;
enum cmd_t { CMD_ERROR, CMD_NORTH, CMD_SOUTH, CMD_EAST, CMD_WEST, CMD_QUIT };

struct room_t {
    char *name;
    room_t *north;
    room_t *south;
    room_t *east;
    room_t *west;
};

void print_room(room_t *room);
room_t *read_dungeon_file(char *filename, int *length);
int parse_command(char *cmd);
int has_path(room_t *room, int bearing);
void free_rooms(room_t *rooms, int len);
room_t *find_room(char *name, room_t *rooms, int length);

/*
 ---N---
|       |
|       |
E       |
|       |
|       |
 -------
*/

void print_room(room_t *room) {
    /*
    printf("room->name = %s\n", room->name);
    printf("room->north = %p\n", room->north);
    printf("room->south = %p\n", room->south);
    printf("room->east = %p\n", room->east);
    printf("room->west = %p\n", room->west);
    */

    printf("\n");
    printf("%s\n", room->name);
    printf(" ---%s--- \n", (room->north) == NULL ? "-" : "N");
    printf("|       |\n");
    printf("|       |\n");
    printf("%s       %s\n", (room->west == NULL) ? "|" : "W", (room->east == NULL) ? "|" : "E");
    printf("|       |\n");
    printf("|       |\n");
    printf(" ---%s--- \n", (room->south) == NULL ? "-" : "S");
    printf("\n");
}

void free_rooms(room_t *rooms, int len) {
    for (int i = 0; i < len; i++)
        free(rooms[i].name);

    free(rooms);
}

int has_path(room_t *room, int bearing) {
    switch (bearing) {
        case CMD_NORTH: return (room->north != NULL);
        case CMD_SOUTH: return (room->south != NULL);
        case CMD_EAST:  return (room->east != NULL);
        case CMD_WEST:  return (room->west != NULL);
    }

    return 0;
}

room_t *find_room(char *name, room_t *rooms, int length) {
    for (int i = 0; i < length; i++) {
        //printf("len(name): %zd, name: %s, rooms[%d].name: %s\n", strlen(name), name, i,rooms[i].name);
        if (strcmp(rooms[i].name, name) == 0) {
            return &rooms[i];
        }
    }

    return NULL;
}

room_t *read_dungeon_file(char *filename, int *room_count) {
    room_t *rooms = NULL;
    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("File '%s' does not exist\n", filename);
        exit(1);
    }

    char buf[LINE_MAX] = {0};

    // read the room names in
    if (fgets(buf, LINE_MAX, fp) != NULL) {
        // stupid newline ruining my life
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';

        int count = 0;

        // count tokens
        char *tmpbuf = strdup(buf);
        char *token = strtok(tmpbuf, " ");

        while (token != NULL) {
            count++;
            token = strtok(NULL, " ");
        }

        free(tmpbuf);
        tmpbuf = NULL;

        *room_count = count;
        // malloc rooms
        rooms = malloc(sizeof(*rooms) * count);
        memset(rooms, 0, sizeof(*rooms) * count);

        // count tokens
        tmpbuf = strdup(buf);
        token = strtok(tmpbuf, " ");
        count = 0;

        // assign names
        while (token != NULL) {
            rooms[count++].name = strdup(token);
            token = strtok(NULL, " ");
        }

        free(tmpbuf);
        // zero out the buffer
        memset(buf, 0, LINE_MAX);
    }
/*
    for (int i = 0; i < *room_count; i++) {
        printf("rooms[%d].name = %s\n", i, rooms[i].name);
    }
*/
    //
    while (fgets(buf, LINE_MAX, fp) != NULL) {
        // stupid newline ruining my life
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';

        char *token = strtok(buf, " > ");
        int count = 0, bearing = 0;
        room_t *room_a = NULL, *room_b = NULL;

        while (token != NULL) {
            switch (count) {
                case 0: room_a = find_room(token, rooms, *room_count); break;
                case 1: bearing = parse_command(token); break;
                case 2: room_b = find_room(token, rooms, *room_count); break;
            }
            //printf("iteration: %d, token = %s\n", count, token);
            count++;
            token = strtok(NULL, " > ");
        }

        //printf("room_a: %p, room_b: %p, bearing: %d\n", room_a, room_b, bearing);

        switch (bearing) {
            case CMD_NORTH: room_a->north = room_b; break;
            case CMD_SOUTH: room_a->south = room_b; break;
            case CMD_EAST:  room_a->east = room_b; break;
            case CMD_WEST:  room_a->west = room_b; break;
        }

        // zero out the buffer
        memset(buf, 0, LINE_MAX);
    }

    fclose(fp);

    return rooms;
}

int parse_command(char *cmd) {
    if (strncmp(cmd, "NORTH", 5) == 0)
        return CMD_NORTH;
    else if (strncmp(cmd, "SOUTH", 5) == 0)
        return CMD_SOUTH;
    else if (strncmp(cmd, "EAST", 4 ) == 0)
        return CMD_EAST;
    else if (strncmp(cmd, "WEST", 4) == 0)
        return CMD_WEST;
    else if (strncmp(cmd, "QUIT", 4) == 0)
        return CMD_QUIT;
    else
        return CMD_ERROR;
}


int main(int argc, char** argv) {
    room_t *rooms = NULL;
    int room_count = 0;
    room_t *curr_room = NULL;

    int quit = 0;
    char buf[BUFFER_MAX];
    int buf_read = 0;

    if (argc == 1) {
        printf("No Level File Specified\n");
        return 1;
    }

    // create the array of rooms
    rooms = read_dungeon_file(argv[1], &room_count);
    // current room
    curr_room = &rooms[0];

    // do input loop
    while (!quit) {
        print_room(curr_room);

        memset(buf, 0, BUFFER_MAX);
        buf_read = 0;

        char c;
        // read a command
        while ((c = getchar()) != '\n')
            if (buf_read < BUFFER_MAX)
                buf[buf_read++] = c;

        buf[buf_read] = '\0';

        int cmd = parse_command(buf);

        if (cmd == CMD_ERROR) {
            printf("What?\n");
            continue;
        }

        if (cmd == CMD_QUIT) {
            quit = 1;
            continue;
        }

        if (!has_path(curr_room, cmd)) {
            printf("No Path This Way\n");
            continue;
        }

        switch (cmd) {
            case CMD_NORTH:
                curr_room = curr_room->north;
                break;
            case CMD_SOUTH:
                curr_room = curr_room->south;
                break;
            case CMD_EAST:
                curr_room = curr_room->east;
                break;
            case CMD_WEST:
                curr_room = curr_room->west;
                break;
        }

    }

    free_rooms(rooms, room_count);

    return 0;
}
