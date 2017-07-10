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
#include <math.h>

#include "atoms.h"

// global state struct
state_t g_State = {0};

// global state struct helpers
void set_state_loaded() {
    g_State.loaded_flag = 1;
    g_State.loading_flag = 0;
    g_State.start_flag = 1;
}

void set_state_loading() {
    g_State.loaded_flag = 0;
    g_State.loading_flag = 1;
    g_State.start_flag = 0;
}

uint8_t  get_state_loaded()    { return g_State.loaded_flag; }
uint8_t  get_state_loading()   { return g_State.loading_flag; }

void     set_state_started()   { g_State.start_flag = 1; }
uint8_t  get_state_started()   { return g_State.start_flag; }

void     set_state_quit()      { g_State.quit_flag = 1; }
uint8_t  get_state_quit()      { return g_State.quit_flag; }

void set_next_turn()  { g_State.current_turn++; }
void set_prev_turn()  { g_State.current_turn--; }
int  get_turn()       { return g_State.current_turn; }

void set_active_player(int id) { g_State.active_player = id; }
int  get_active_player()       { return g_State.active_player; }

void set_next_player() {
    set_active_player((get_active_player()+1) % g_State.no_players);
}

void set_prev_player() {
    if (get_active_player() == 0) {
        set_active_player(g_State.no_players-1);
    } else {
        set_active_player((get_active_player()-1) % g_State.no_players);
    }
}

// print help
void print_help(void) {
    printf("\n");
    printf("%s displays this help message\n","HELP");
    printf("%s quits the current game\n","QUIT");
    printf("\n");
    printf("%s draws the game board in terminal\n","DISPLAY");
    printf("%s starts the game\n","START <number of players> <width> <height>");
    printf("%s places an atom in a grid space\n","PLACE <x> <y>");
    printf("%s undoes the last move made\n","UNDO");
    printf("%s displays game statistics\n","STAT");
    printf("\n");
    printf("%s saves the state of the game\n","SAVE <filename>");
    printf("%s loads a save file\n","LOAD <filename>");
    printf("%s plays from n steps into the game\n","PLAYFROM <turn>");
    printf("\n");
}

void print_stats(player_t *players) {
    for (int i = 0; i < g_State.no_players; i++) {
        printf("Player %s:\n",players[i].colour);

        if (get_turn() > g_State.no_players && players[i].grids_owned == 0) {
            printf("Lost\n");
        } else {
            printf("Grid Count: %d\n", players[i].grids_owned);
        }


        printf("\n");
    }
}

/*
Grid Layout 1x1
+--+
|  |
+--+

Grid Layout 2x2
+-----+
|  |  |
|  |  |
+-----+
*/

// print the current game grid
void print_grid(game_grid_t *grid_data) {
    int rows = grid_data->height+2;
    // width = (width*2)+(width-1)+2
    int cols = (grid_data->width*2) + (grid_data->width-1) + 2;
    char **grid = malloc(sizeof(*grid) * rows);

    MALLOC_TEST(grid);

    for (int i = 0; i < rows; i++) {
        grid[i] = malloc(sizeof(**grid) * (cols+1));
        MALLOC_TEST(grid[i]);

        // fill with spaces
        memset(grid[i], ' ', sizeof(**grid) * (cols+1));
    }

    for (int i = 0; i < rows; i++) {
        if (i == 0 || i == rows-1) {
            grid[i][0] = grid[i][cols-1] = '+';

            for (int j = 1; j < cols-1; j++)
                grid[i][j] = '-';

        } else {
            for (int j = 0; j < cols; j += 3)
                grid[i][j] = '|';
        }

        // null terminate row
        grid[i][cols] = '\0';
    }

    printf("\n");

    for (int i = 1, idx_y = 0; i < rows-1; i++, idx_y++) {
        for (int j = 1, idx_x = 0; j < cols-1; j+=3, idx_x++) {
            // create a tmp pointer to the grid square
            grid_t *this_grid = &grid_data->grid[(idx_y*grid_data->width)+idx_x];

            // if we have an owner
            if (this_grid->owner != NULL) {
                // copy first character
                grid[i][j] = this_grid->owner->colour[0];
                // convert the atom count to a character
                grid[i][j+1] = this_grid->atom_count + '0';
            }
        }
    }

    // print the grid
    for (int i = 0; i < rows; i++) {
        printf("%s\n",grid[i]);
    }

    printf("\n");

    // free the grid rows
    for (int i = 0; i < rows; i++)
        if (grid[i] != NULL)
            free(grid[i]);

    // free the grid
    free(grid);
}

// create the player data struct
// must be matched with free_player_data
player_t *create_player_data(int count) {
    player_t *players = malloc(sizeof(player_t)*count);
    MALLOC_TEST(players);

    for (int i = 0; i < count; i++) {
        switch (i) {
            case 0: players[i].colour = "Red"; break;
            case 1: players[i].colour = "Green"; break;
            case 2: players[i].colour = "Purple"; break;
            case 3: players[i].colour = "Blue"; break;
            case 4: players[i].colour = "Yellow"; break;
            case 5: players[i].colour = "White"; break;
        }

        players[i].grids_owned = 0;
    }

    return players;
}

// free the player data struct
void free_player_data(player_t *players) {
    if (players != NULL) {
        free(players);
        players = NULL;
    }
}

// get a count of players remaining in the game
int get_players_left(player_t *players) {
    int count = 0;

    // until every player performs their turn at least once
    // no winner check is performed
    if (get_turn() < g_State.no_players)
        return g_State.no_players;

    for (int i = 0; i < g_State.no_players; i++)
        count += (players[i].grids_owned > 0);

    return count;
}

// create a save data struct
// must be matched with free_save_data
save_t *create_save_data(char *filename) {
    save_t *psave = malloc(sizeof(*psave));
    MALLOC_TEST(psave);

    psave->filename = NULL;

    if (filename != NULL)
        psave->filename = strdup(filename);

    psave->data = malloc(sizeof(*(psave->data)));
    MALLOC_TEST(psave->data);

    psave->data->width = 0;
    psave->data->height = 0;
    psave->data->no_players = 0;
    psave->data->no_moves = 0;
    psave->data->raw_move_data = NULL;

    return psave;
}

// read save data from a file into a save_data struct
int read_save_data(save_t *psave) {
    if (psave == NULL) {
        printf("Cannot Load Save\n");
        return 1;
    }

    if (psave->filename == NULL)
        return 1;

    FILE *fp = fopen(psave->filename, "rb");

    if (fp == NULL) {
        printf("Cannot Load Save\n");
        return 1;
    }

    // check the filesize is 3 + 4n bytes
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    if ((fsize-3) % 4 != 0) {
        fclose(fp);
        printf("Corrupt Save Data\n");
        return 1;
    }

    // calculate how many moves are stored in the save
    int num_moves = (fsize-3)/4;

    char *buf = malloc(sizeof(*buf)*fsize);

    if (buf == NULL) {
        printf("Malloc failed\n");
        fclose(fp);
        exit(1);
    }

    int bytes_read = fread(buf, 1, fsize, fp);
    // close the file handle
    fclose(fp);

    if (bytes_read != fsize) {
        printf("Read error\n");
        exit(1);
    }

    uint8_t w = (uint8_t)buf[0];
    uint8_t h = (uint8_t)buf[1];
    uint8_t p = (uint8_t)buf[2];

    if (w < MIN_WIDTH || w > MAX_WIDTH || h < MIN_HEIGHT || h > MAX_HEIGHT ||
        p < MIN_PLAYERS || p > MAX_PLAYERS)
    {
        free(buf);
        return 1;
    }

    /*
    printf("width = %d, height = %d, no_players = %d\n",w,h,p);
    */
    psave->data->width = w;
    psave->data->height = h;
    psave->data->no_players = p;
    // number of raw_move_data entries
    psave->data->no_moves = num_moves;

    if (num_moves == 0) {
        free(buf);
        return 0;
    }

    // allocate the move data array
    psave->data->raw_move_data = malloc(sizeof(*(psave->data->raw_move_data)) * num_moves);
    MALLOC_TEST(psave->data->raw_move_data);

    uint32_t *pmove = (uint32_t *)(buf + 3);

    for (int i = 0; i < num_moves; i++)
        psave->data->raw_move_data[i] = *pmove++;

    /*
    for (int i = 0; i < num_moves; i++)
        printf("raw_move_data[%d] = %08x\n",i,psave->data->raw_move_data[i]);
    */

    free(buf);

    return 0;
}

// write a save_data struct to a file
int write_save_data(save_t *psave) {
    if (psave == NULL || psave->filename == NULL)
        return 1;

    FILE *fp = fopen(psave->filename, "rb");

    if (fp != NULL) {
        printf("File Already Exists\n");
        fclose(fp);

        return 1;
    }

    fp = fopen(psave->filename, "wb");

    if (fp == NULL) {
        printf("Error creating file '%s'\n",psave->filename);
        return 1;
    }

    fwrite(&(psave->data->width), sizeof(uint8_t), 1, fp);
    fwrite(&(psave->data->height), sizeof(uint8_t), 1, fp);
    fwrite(&(psave->data->no_players), sizeof(uint8_t), 1, fp);

    for (int i = 0; i < psave->data->no_moves; i++)
        fwrite(&(psave->data->raw_move_data[i]), sizeof(uint32_t), 1, fp);

    fclose(fp);

    return 0;
}

// free a save_data struct
void free_save_data(save_t *psave) {
    if (psave != NULL) {
        if (psave->filename != NULL)
            free(psave->filename);

        psave->filename = NULL;

        if (psave->data != NULL) {
            if (psave->data->raw_move_data != NULL) {
                free(psave->data->raw_move_data);
                psave->data->raw_move_data = NULL;
            }

            free(psave->data);
            psave->data = NULL;
        }

        free(psave);
        psave = NULL;
    }
}

// free a tokenized command
void free_cmd_string(char **tokens, int count) {
    if (tokens != NULL) {
        for (int i = 0; i < count; i++) {
            if (tokens[i] != NULL) {
                free(tokens[i]);
                tokens[i] = NULL;
            }
        }

        free(tokens);
        tokens = NULL;
    }
}

// validate a tokenized command
int parse_cmd_string(char **tokens, int count) {
    if (count == 0)
        return 1;

    if (strncmp(tokens[0],"HELP",4) == 0 ||
        strncmp(tokens[0],"QUIT",4) == 0) {
        if (count != 1) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }

    } else if (strncmp(tokens[0],"DISPLAY",7) == 0) {
        if (count != 1) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }
        if (!get_state_started()) {
            printf("Invalid Command\n");
            return 1;
        }
    } else if (strncmp(tokens[0],"STAT",4) == 0) {
        if (count != 1) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }
        if (!get_state_started()) {
            printf("Game Not In Progress\n");
            return 1;
        }

    } else if (strncmp(tokens[0],"START",5) == 0) {
        if (get_state_started()) {
            printf("Cannot Start Game\n");
            return 1;
        }
        if (count < 4) {
            printf("Missing Argument\n");
            return 1;
        }

        if (count > 4) {
            printf("Too Many Arguments\n");
            return 1;
        }

        int args[3] = {0};

        for (int i = 1; i < 4; i++)
            if (sscanf(tokens[i],"%d",&args[i-1]) != 1)
                printf("Invalid command arguments\n");

        if (args[0] < MIN_PLAYERS || args[0] > MAX_PLAYERS ||
            args[1] < MIN_WIDTH || args[1] > MAX_WIDTH ||
            args[2] < MIN_HEIGHT || args[2] > MAX_HEIGHT) {
            printf("Invalid command arguments\n");
            return 1;
        }

        // width * height >= k (number of players)
        if (args[1]*args[2] < args[0]) {
            printf("Cannot Start Game\n");
            return 1;
        }

    } else if (strncmp(tokens[0],"UNDO",4) == 0) {
        if (count != 1) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }
        if (get_turn() == 0) {
            printf("Cannot Undo\n");
            return 1;
        }
    } else if (strncmp(tokens[0],"SAVE",4) == 0) {
        if (count != 2) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }
        if (!get_state_started()) {
            printf("Invalid Command\n");
            return 1;
        }

        // check if the file exists by opening it
        FILE *fp = fopen(tokens[1], "rb");

        // file exists
        if (fp != NULL) {
            printf("File Already Exists\n");
            fclose(fp);
            return 1;
        }
    } else if (strncmp(tokens[0],"LOAD",4) == 0) {
        if (count != 2) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }

        if (get_state_loaded()) {
            printf("Invalid Command\n");
            return 1;
        }

        if (get_state_started() || get_state_loading()) {
            printf("Restart Application To Load Save\n");
            return 1;
        }

        // check if the file exists by opening it
        FILE *fp = fopen(tokens[1], "rb");

        // file does not exist
        if (fp == NULL) {
            printf("Cannot Load Save\n");
            return 1;
        }

        // close the file
        fclose(fp);

    } else if (strncmp(tokens[0],"PLAYFROM",8) == 0) {
        if (count != 2) {
            printf("Invalid Command\n");
            //printf("Too Many Arguments\n");
            return 1;
        }
        if (!get_state_loading()) {
            printf("Invalid Command\n");
            return 1;
        }

        int value = 0;

        if (sscanf(tokens[1],"%d",&value) != 1 && strncmp(tokens[1],"END",3) != 0) {
            printf("Invalid Command\n");
            return 1;
        }

    } else if (strncmp(tokens[0],"PLACE",5) == 0) {
        if (count != 3) {
            printf("Invalid Coordinates\n");
            return 1;
        }

        if (!get_state_started()) {
            printf("Invalid Command\n");
            return 1;
        }

        int args[2] = {0};

        for (int i = 1; i < 3; i++)
            if (sscanf(tokens[i],"%d",&args[i-1]) != 1)
                printf("Invalid Coordinates\n");

        if (args[0] >= g_State.width || args[1] >= g_State.height) {
            printf("Invalid Coordinates\n");
            return 1;
        }
    } else {
        printf("Invalid Command\n");
        return 1;
    }

    return 0;
}

// read a command and tokenize into a command string
// must be matched with free_cmd_string
char **read_cmd_string(int *count) {
    char *buf = NULL, c;
    int read = 0;

    *count = 0;

    // allocate a temp buffer
    buf = malloc(sizeof(*buf)*(MAX_LINE+1));
    MALLOC_TEST(buf);
    // zero the buffer
    memset(buf, 0, MAX_LINE+1);

    // read upto MAX_LINE characters from stdin and discard the rest
    while ((c = getchar()) != '\n') {
        if (read < MAX_LINE)
            buf[read++] = c;
    }

    if (read == 0) {
        free(buf);
        return NULL;
    }

    buf[read] = '\0';

    // create a temporary buffer since strtok modifies its source
    char *tmpbuf = strdup(buf);
    char *token  = strtok(tmpbuf, " ");

    // count tokens so we can allocate an array of pointers to return
    while (token != NULL) {
        (*count)++;
        token = strtok(NULL, " ");
    }

    // found no tokens
    if (*count == 0) {
        free(buf);
        free(tmpbuf);
        return NULL;
    }

    char **tokens = malloc((*count) * sizeof(*tokens));
    MALLOC_TEST(tokens);

    // copy the buffer back to the temp buffer to retokenize
    memcpy(tmpbuf, buf, sizeof(*tmpbuf)*(read+1));
    // do the allocation for the token strings
    token = strtok(tmpbuf, " ");

    for (int i = 0; token != NULL; i++) {
        // allocate each token
        tokens[i] = malloc((strlen(token)+1) * sizeof(**tokens));
        MALLOC_TEST(tokens[i]);
        // copy the token
        memcpy(tokens[i], token, strlen(token)+1);

        //printf("tokens[%d] = %s\n",i,tokens[i]);

        token = strtok(NULL, " ");
    }

    // free the buffer
    free(tmpbuf);
    free(buf);

    return tokens;
}

game_grid_t *create_grid(int width, int height) {
    game_grid_t *grid = malloc(sizeof(game_grid_t));

    MALLOC_TEST(grid);

    grid->width = width;
    grid->height = height;
    // allocate the grid squares
    grid->grid = malloc(sizeof(grid_t) * width * height);
    MALLOC_TEST(grid->grid);

    // setup the grid
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid->grid[(i*width)+j].owner = NULL;
            grid->grid[(i*width)+j].atom_count = 0;
        }
    }

    return grid;
}

void free_grid(game_grid_t *grid) {
    if (grid != NULL) {
        free(grid->grid);
        grid->grid = NULL;

        free(grid);
        grid = NULL;
    }
}

int count_player_owned_grids(game_grid_t *grid_data, player_t *this_player) {
    if (grid_data == NULL || this_player == NULL)
        return 0;

    int count = 0;

    for (int i = 0; i < grid_data->width*grid_data->height; i++) {
        grid_t *this_grid = &grid_data->grid[i];

        if (this_grid->owner == this_player)
            count++;
    }

    //printf("this_player = %s, count = %d\n",this_player->colour, count);

    return count;
}

player_t *place_grid(game_grid_t *grid_data, player_t *this_player, player_t *players, int x, int y) {
    if (grid_data == NULL || this_player == NULL)
        return NULL;

    // store the old owner
    player_t *old_owner = grid_data->grid[(y*grid_data->width)+x].owner;
    point_t pt = { x, y }, pt2 = { 0 };
    int expansion = 0;

    // initialize a stack for DFS
    list_t *pt_list = create_point_list();

    // push the point onto the stack
    push_point_list(pt_list, pt);

    // start popping the stack until its empty
    while (pop_point_list(pt_list, &pt2)) {
        int adj_count     = get_adjacent_grid_count(pt2.x, pt2.y);
        //printf("(%d,%d) => adjacent grid count = %d\n",pt2.x,pt2.y,adj_count);
        grid_t *this_grid = &grid_data->grid[(pt2.y * grid_data->width) + pt2.x];

        if (count_player_owned_grids(grid_data, this_player) == grid_data->width*grid_data->height-1 &&
            get_players_left(players) == 1)
            break;

        if (this_grid->owner == NULL) {
            this_grid->owner = this_player;
            this_grid->atom_count++;

            // increment grids owned
            this_player->grids_owned++;
            // this should be end either move or expansion
        } else if (this_grid->owner == this_player) {
            // increase the atom count
            this_grid->atom_count++;

            // if the atom count equals adjacent count we expand into adjacent squares
            // and give up the square that triggered the expansion
            if (this_grid->atom_count == adj_count) {
                expansion = 1;
                // all adjacent point candidates
                // clockwise from top, right, bottom, left
                point_t adj_pts[] = {{ pt2.x, pt2.y-1 }, { pt2.x+1, pt2.y },
                                     { pt2.x, pt2.y+1 }, { pt2.x-1, pt2.y }};

                // discard any points not on the grid
                for (int i = 3; i >= 0; i--) {
                    if (validate_point(adj_pts[i].x, adj_pts[i].y)) {
                        // push onto the stack
                        push_point_list(pt_list, adj_pts[i]);
                    }
                }

                // give up the centre square
                this_grid->owner = NULL;
                this_grid->atom_count = 0;
                // player loses a grid
                this_player->grids_owned--;
            }
        } else {
            if (expansion) {
                // save the previous owner
                player_t *last_owner = this_grid->owner;
                // take ownership and updates grids owned
                this_grid->owner = this_player;
                this_player->grids_owned++;
                // update the last owners grids owned
                last_owner->grids_owned--;

                // push the point back onto the stack so it gets reprocessed
                push_point_list(pt_list, pt2);
            }
        }
    }

    free_point_list(pt_list);

    return old_owner;
}

int push_move_list(game_t *list, int x, int y) {
    if (list == NULL)
        return 0;

    list->count++;

    move_t *move = malloc(sizeof *move);
    MALLOC_TEST(move);

    move->x = x;
    move->y = y;
    move->prev = move->next = NULL;

    if (list->head == NULL) {
        list->head = list->tail = move;
    } else {
        move->next = list->head;
        list->head->prev = move;
        list->head = move;
    }

    return 1;
}

move_t *pop_move_list(game_t *list) {
    if (list->head == NULL)
        return NULL;

    //printf("list->head = %p, list->tail = %p, count = %d\n", list->head, list->tail, list->count);
    move_t *move = list->head;
    list->head = move->next;
    //printf("list->head = %p, list->tail = %p, count = %d\n", list->head, list->tail, list->count);

    if (list->head != NULL)
        list->head->prev = NULL;

    move->next = NULL;
    move->prev = NULL;

    list->count--;

    return move;
}

void free_move(move_t *move) {
    if (move != NULL)
        free(move);
}

game_t *copy_move_list(game_t *list) {
    game_t *copy = create_move_list();

    move_t *curr = list->tail;

    while (curr != NULL) {
        push_move_list(copy, curr->x, curr->y);
        curr = curr->prev;
    }

    return copy;
}

// initialize the move data struct
game_t *create_move_list() {
    game_t *list = malloc(sizeof *list);
    MALLOC_TEST(list);

    list->head = list->tail = NULL;
    list->count = 0;

    return list;
}

void free_move_list(game_t *list) {
    if (list != NULL) {
        if (list->head != NULL) {
            move_t *curr = list->head;

            while (curr != NULL) {
                move_t *tmp = curr->next;
                free_move(curr);
                curr = tmp;
            }
        }
        free(list);
    }
}

undo_t *insert_move(game_t *game_data, game_grid_t *grid_data, player_t *player,
                    int x, int y) {

    grid_t *this_grid = &grid_data->grid[(y*grid_data->width)+x];

    if (this_grid->owner != NULL && this_grid->owner != &player[get_active_player()]) {
        printf("Cannot Place Atom Here\n");
        return NULL;
    }

    // push the current move onto the move list
    push_move_list(game_data, x, y);
    // create the undo state
    undo_t *undo_data = create_undo_state(grid_data, player);
    // perform grid placement
    place_grid(grid_data, &player[get_active_player()], player, x, y);

    return undo_data;
}

undo_t *create_undo_state(game_grid_t *grid_data, player_t *players) {
    undo_t *undo = malloc(sizeof(undo_t));
    MALLOC_TEST(undo);

    // allocate enough space for a copy of the grid data
    undo->saved_grid = malloc(sizeof(grid_t) * grid_data->width * grid_data->height);
    MALLOC_TEST(undo->saved_grid);

    // make a copy of the previous grid
    memcpy(undo->saved_grid, grid_data->grid, sizeof(grid_t) * grid_data->width * grid_data->height);

    // create a copy of the player data
    undo->saved_players = malloc(sizeof(int)*g_State.no_players);
    MALLOC_TEST(undo->saved_players);

    for (int i = 0; i < g_State.no_players; i++)
        undo->saved_players[i] = players[i].grids_owned;

    return undo;
}

void free_undo_state(undo_t *undo_data) {
    if (undo_data != NULL) {
        //undo_data->last_move = NULL;
        if (undo_data->saved_grid != NULL) {
            free(undo_data->saved_grid);
            undo_data->saved_grid = NULL;
        }

        if (undo_data->saved_players != NULL) {
            free(undo_data->saved_players);
            undo_data->saved_players = NULL;
        }

        free(undo_data);
        undo_data = NULL;
    }
}

undo_list_t *create_undo_list() {
    undo_list_t *list = malloc(sizeof(undo_list_t));

    MALLOC_TEST(list);
    memset(list, 0, sizeof(undo_list_t));

    return list;
}

void free_undo_list(undo_list_t *list) {
    if (list != NULL) {
        undo_t *undo = NULL;

        while ((undo = pop_undo_list(list)) != NULL)
            free_undo_state(undo);

        free(list);
    }
}

int push_undo_list(undo_list_t *list, undo_t *undo) {
    if (list == NULL)
        return 0;

    list->count++;

    if (list->head == NULL) {
        list->head = list->tail = undo;
        undo->next = NULL;
        undo->prev = NULL;
    } else {
        undo->next = list->head;
        undo->prev = NULL;
        list->head->prev = undo;
        list->head = undo;
    }

    return 1;
}

undo_t *pop_undo_list(undo_list_t *list) {
    if (list->head == NULL)
        return NULL;

    //printf("list->head = %p, list->tail = %p, count = %d\n", list->head, list->tail, list->count);

    undo_t *undo = list->head;
    list->head = undo->next;

    //printf("list->head = %p, list->tail = %p, count = %d\n", list->head, list->tail, list->count);

    if (list->head != NULL)
        list->head->prev = NULL;

    undo->next = NULL;
    undo->prev = NULL;

    list->count--;

    return undo;
}

void undo_last_move(game_t *game_data, game_grid_t *grid_data, player_t *players, undo_t *undo_data) {
    // restore the previous grid
    memcpy(grid_data->grid, undo_data->saved_grid, sizeof(grid_t) * grid_data->width * grid_data->height);

    // restore the previous player data
    for (int i = 0; i < g_State.no_players; i++)
        players[i].grids_owned = undo_data->saved_players[i];

    // remove the move from the move list
    move_t *move = pop_move_list(game_data);
    free_move(move);

    return;
}

int get_adjacent_grid_count(int x, int y) {
    int w = g_State.width;
    int h = g_State.height;

    if ((x == 0 || x == w-1) && (y == 0 || y == h-1)) {
        return 2;
    }

    if (((x == 0 || x == w-1) && (y >= 1 && y <= h-2)) ||
        ((y == 0 || y == h-1) && (x >= 1 && x <= w-2))) {
        return 3;
    }

    return 4;
}

int validate_point(int x, int y) {
    return ((x >= 0 && x < g_State.width) && (y >= 0 && y < g_State.height));
}

/*
 * simple doubly linked list implementation of a dynamic stack that
 * can be used to store point structs
 *
 * create_point_list(void) initializes the stack and returns it
 * free_point_list(list *)   frees the stack
 *
 * push_point_list(list *, point_t) pushes a point onto the stack
 *  pop_point_list(list *)
 *
 */
list_t *create_point_list() {
    list_t *list = malloc(sizeof(list_t));

    MALLOC_TEST(list);
    memset(list, 0, sizeof(list_t));

    return list;
}

void free_point_list(list_t *list) {
    if (list != NULL) {
        if (list->head != NULL) {
            list_entry_t *curr = list->tail->prev;

            for ( ; curr != NULL; curr = curr->prev)
                if (curr->next != NULL)
                    free(curr->next);

            free(curr);
        }
        free(list);
    }
}

int push_point_list(list_t *list, point_t pt) {
    if (list == NULL)
        return 0;

    list_entry_t *entry = malloc(sizeof(list_entry_t));
    MALLOC_TEST(entry);

    entry->entry.x = pt.x;
    entry->entry.y = pt.y;
    entry->prev = NULL;
    entry->next = NULL;

    if (list->head == NULL) {
        list->head = list->tail = entry;
    } else {
        entry->next = list->head;
        list->head->prev = entry;
        list->head = entry;
    }

    return 1;
}

int pop_point_list(list_t *list, point_t *pt) {
    if (list->head == NULL)
        return 0;

    list_entry_t *entry = list->head;

    pt->x = entry->entry.x;
    pt->y = entry->entry.y;

    list->head = entry->next;

    if (list->head != NULL)
        list->head->prev = NULL;

    free(entry);

    return 1;
}

/*
 * pack the moves into a binary format for saving to a file
 *
 * raw_move_data is in the byte format
 * raw_move_data+0 = XX
 * raw_move_data+1 = YY
 * raw_move_data+2 = 00
 * raw_move_data+3 = 00
 */
void pack_save_data(game_t *list, save_t *save) {
    if (list == NULL || save == NULL)
        return;

    // start at the end of the list
    move_t      *curr  = list->tail;
    save_file_t *sfd   = save->data;

    sfd->width      = (uint8_t)g_State.width;
    sfd->height     = (uint8_t)g_State.height;
    sfd->no_players = (uint8_t)g_State.no_players;

    // allocate the raw move data
    sfd->raw_move_data = malloc(sizeof(uint32_t) * list->count);
    MALLOC_TEST(sfd->raw_move_data);

    sfd->no_moves = list->count;

    int i = 0;

    while (curr != NULL) {
        uint32_t tmp = 0;
        // write x at first byte
        tmp |= ((uint8_t)curr->x);
        // write y at second byte
        tmp |= ((uint8_t)curr->y << 8);

        sfd->raw_move_data[i++] = tmp;
        // iterate backwards since we want it last->first
        curr = curr->prev;
    }
}

void unpack_save_data(int turns, save_t *save, point_t **points) {
    if (save == NULL)
        return;

    save_file_t *sfd   = save->data;

    *points = malloc(sizeof(point_t)*turns);
    MALLOC_TEST(*points);
    memset(*points,0,sizeof(point_t)*turns);

    for (int i = 0; i < turns; i++) {
        uint32_t tmp = sfd->raw_move_data[i];
        // extract x from first byte
        int x = tmp & 0xff;
        // extract y from second byte
        int y = (tmp & 0xff00) >> 8;
        // save the points for performing moves
        point_t *pp = *points;
        pp[i].x = x;
        pp[i].y = y;
    }
}

int main(int argc, char** argv) {
    // struct containing save data
    save_t *save_data = NULL;
    // struct containing player data
    player_t *player_data = NULL;
    // struct of grid squares
    game_grid_t *grid_data = NULL;
    // struct for undo data
    undo_list_t *undo_list = NULL;
    // struct for move data
    game_t *game_data = NULL;

    while (!get_state_quit()) {
        int count;
        char **tokens = NULL;

        // read in a command string and tokenize it
        if ((tokens = read_cmd_string(&count)) == NULL) {
            // failed
            continue;
        }
        // parse the tokenized command
        if (parse_cmd_string(tokens, count)) {
            // validation failed
            free_cmd_string(tokens, count);
            printf("\n");
            continue;
        }

        // command = HELP
        if (strncmp(tokens[0],"HELP",4) == 0) {
            print_help();

        // command = STAT
        } else if (strncmp(tokens[0],"STAT",4) == 0) {
            print_stats(player_data);

        // command = DISPLAY
        } else if (strncmp(tokens[0],"DISPLAY",7) == 0) {
            print_grid(grid_data);

        // command = QUIT
        } else if (strncmp(tokens[0],"QUIT",4) == 0) {
            set_state_quit();

        // command = START k width height
        } else if (strncmp(tokens[0],"START",5) == 0) {
            int p = atoi(tokens[1]);
            int w = atoi(tokens[2]);
            int h = atoi(tokens[3]);

            g_State.width = (uint8_t)w;
            g_State.height = (uint8_t)h;
            g_State.no_players = (uint8_t)p;

            // set up the data
            //save_data = create_save_data(filename);
            player_data = create_player_data(p);
            grid_data = create_grid(w,h);
            game_data = create_move_list();
            undo_list = create_undo_list();

            // weve started the game
            set_state_started();
            // set the active player to player 1
            set_active_player(0);

            printf("Game Ready\n");
            printf("%s's Turn\n", player_data[get_active_player()].colour);
            printf("\n");

        // command = PLACE x y
        } else if (strncmp(tokens[0],"PLACE",5) == 0) {
            int x = atoi(tokens[1]);
            int y = atoi(tokens[2]);

            // perform the move and save a pointer to the undo data
            undo_t *undo = insert_move(game_data, grid_data, player_data, x, y);

            if (undo == NULL) {
                printf("\n");
                free_cmd_string(tokens, count);
                continue;
            }

            // add the undo data for the current move to the end of the undo list
            push_undo_list(undo_list, undo);

            // last man standing wins, flag a quit to occur
            if (get_players_left(player_data) > 1) {
                // go to next player
                set_next_player();

                if (get_turn() >= g_State.no_players) {
                // check if the player is still in the game, if not goto next
                    while (player_data[get_active_player()].grids_owned == 0)
                        set_next_player();
                }

                // increment the turn
                set_next_turn();

                printf("%s's Turn\n", player_data[get_active_player()].colour);
                printf("\n");
            } else {
                printf("%s Wins!\n", player_data[get_active_player()].colour);
                //print_grid(grid_data);
                set_state_quit();
                g_State.win_flag = 1;
            }

        // command = UNDO
        } else if (strncmp(tokens[0],"UNDO",4) == 0) {
            // rewind player and turn
            set_prev_player();
            set_prev_turn();

            undo_t *undo = pop_undo_list(undo_list);

            // undo last move
            undo_last_move(game_data, grid_data, player_data, undo);
            // cleanup undo data for last move
            free_undo_state(undo);

            printf("%s's Turn\n", player_data[get_active_player()].colour);
            printf("\n");

        // command = LOAD filename
        } else if (strncmp(tokens[0],"LOAD",4) == 0) {
            save_data = create_save_data(tokens[1]);

            if (!read_save_data(save_data)) {
                printf("Game Loaded\n");
                printf("\n");

                // set the state of the game to waiting for PLAYFROM command
                set_state_loading();
            }

        // command = SAVE filename
        } else if (strncmp(tokens[0],"SAVE",4) == 0) {
            // allocate the save data
            save_data = create_save_data(tokens[1]);
            // pack the game data into the save binary format
            pack_save_data(game_data, save_data);

            if (!write_save_data(save_data)) {
                printf("Game Saved\n");
                printf("\n");
            }

            // free the save data and set it to null
            free_save_data(save_data);
            save_data = NULL;

        // command = PLAYFROM pos
        } else if (strncmp(tokens[0],"PLAYFROM",8) == 0) {
            int turn = 0;

            if (strncmp(tokens[1],"END",3) == 0)
                turn = save_data->data->no_moves;
            else
                turn = atoi(tokens[1]);

            if (turn < 0) {
                printf("Invalid Turn Number\n");
                printf("\n");
            } else {
                turn = MIN(turn, save_data->data->no_moves);

                point_t *move_points = NULL;
                // unpack and store the requested moves in point structs
                unpack_save_data(turn, save_data, &move_points);
                // change state to loaded
                set_state_loaded();

                // initialize global state
                g_State.width = (uint8_t)save_data->data->width;
                g_State.height = (uint8_t)save_data->data->height;
                g_State.no_players = (uint8_t)save_data->data->no_players;

                // setup game data
                player_data = create_player_data(g_State.no_players);
                grid_data = create_grid(g_State.width,g_State.height);
                game_data = create_move_list();
                undo_list = create_undo_list();

                // weve started the game
                set_state_started();
                // set the active player to player 1
                set_active_player(0);

                // silently simulate playing the game upto the requested turn
                for (int i = 0; i < turn; i++) {
                    // perform the move and save a pointer to the undo data
                    //printf("move_points[%d].x = %d, move_points[%d].y = %d\n",i,move_points[i].x,i,move_points[i].y);
                    undo_t *undo = insert_move(game_data, grid_data, player_data, move_points[i].x, move_points[i].y);
                    // add the undo data for the current move to the end of the undo list
                    push_undo_list(undo_list, undo);

                    // last man standing wins, flag a quit to occur
                    if (get_players_left(player_data) > 1) {
                        // go to next player
                        set_next_player();

                        if (get_turn() >= g_State.no_players) {
                            // check if the player is still in the game, if not goto next
                            while (player_data[get_active_player()].grids_owned == 0)
                                set_next_player();
                        }

                        // increment the turn
                        set_next_turn();
                    }
                }

                // free the point list for the moves
                free(move_points);

                // free the save data for the load
                free_save_data(save_data);
                save_data = NULL;

                printf("Game Ready\n");
                printf("%s's Turn\n", player_data[get_active_player()].colour);
                printf("\n");
            }
        }

        // clean up the tokenized command
        free_cmd_string(tokens, count);
    }

    if (game_data   != NULL) free_move_list(game_data);
    // cleanup player data
    if (player_data != NULL) free_player_data(player_data);
    if (grid_data   != NULL) free_grid(grid_data);
    if (undo_list   != NULL) free_undo_list(undo_list);
    // cleanup save data
    if (save_data   != NULL) free_save_data(save_data);

    if (!g_State.win_flag)
        printf("Bye!\n");

    return 0;
}
