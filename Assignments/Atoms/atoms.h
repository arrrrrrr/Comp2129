#ifndef ATOMS_H
#define ATOMS_H

#include <stdint.h>

#define MALLOC_TEST(PTR) if (PTR == NULL) {            \
                            printf("Malloc Failed\n"); \
                            exit(1);                   \
                         }

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MAX_LINE 255
#define MAX_PLAYERS 6
#define MIN_PLAYERS 2
#define MAX_WIDTH 255
#define MAX_HEIGHT 255
#define MIN_WIDTH 2
#define MIN_HEIGHT 2

typedef struct move_t move_t;
typedef struct player_t player_t;
typedef struct game_t game_t;
typedef struct grid_t grid_t;
typedef struct save_t save_t;
typedef struct save_file_t save_file_t;
typedef struct state_t state_t;
typedef struct game_grid_t game_grid_t;
typedef struct undo_t undo_t;
typedef struct undo_list_t undo_list_t;

typedef struct list_t list_t;
typedef struct list_entry_t list_entry_t;
typedef struct point_t point_t;

struct list_t {
    list_entry_t *head;
    list_entry_t *tail;
};

struct point_t {
    int x;
    int y;
};

struct list_entry_t {
    point_t entry;
    list_entry_t *prev;
    list_entry_t *next;
};

struct state_t {
    uint8_t start_flag;
    uint8_t loaded_flag;
    uint8_t loading_flag;
    uint8_t quit_flag;
    uint8_t win_flag;
    uint8_t width;
    uint8_t height;
    uint8_t no_players;
    int active_player;
    int current_turn;
};

struct save_t {
    char* filename;
    save_file_t* data;
};

struct player_t {
    char* colour;
    int grids_owned;
};

struct game_grid_t {
    int width;
    int height;
    grid_t *grid;
};

struct grid_t {
    player_t* owner;
    int atom_count;
};

struct move_t {
    int x;
    int y;
    move_t* prev;
    move_t* next;
};

struct game_t {
    int count;
    move_t *head;
    move_t *tail;
};

struct undo_t {
    grid_t *saved_grid;
    int    *saved_players;
    undo_t *prev;
    undo_t *next;
};

struct undo_list_t {
    int count;
    undo_t *head;
    undo_t *tail;
};

struct save_file_t {
    uint8_t width;
    uint8_t height;
    uint8_t no_players;
    int no_moves;
    uint32_t* raw_move_data;
};

void print_help(void);

void    set_state_loaded(void);
uint8_t get_state_loaded(void);
void    set_state_loading(void);
uint8_t get_state_loading(void);
void    set_state_started(void);
uint8_t get_state_started(void);
void    set_state_quit(void);
uint8_t get_state_quit(void);

void    set_next_turn(void);
void    set_prev_turn(void);
int     get_turn(void);
void    set_active_player(int id);
int     get_active_player(void);
void    set_next_player(void);
void    set_prev_player(void);

int     get_players_left(player_t *players);
int     get_adjacent_grid_count(int x, int y);
int     validate_point(int x, int y);

list_t  *create_point_list();
void     free_point_list(list_t *list);
int      push_point_list(list_t *list, point_t pt);
int      pop_point_list(list_t *list,  point_t *pt);

save_t *create_save_data(char *filename);
int read_save_data(save_t *psave);
int write_save_data(save_t *psave);
void free_save_data(save_t *psave);

void pack_save_data(game_t *list, save_t *save);
void unpack_save_data(int turns, save_t *save, point_t **points);

player_t *create_player_data(int count);
void free_player_data(player_t *players);

game_t *create_move_list();
game_t *copy_move_list(game_t *list);
void    free_move_list(game_t *list);
void    free_move(move_t *move);

int     push_move_list(game_t *list, int x, int y);
move_t *pop_move_list(game_t *list);

undo_list_t *create_undo_list();
void        free_undo_list(undo_list_t *list);
int         push_undo_list(undo_list_t *list, undo_t *undo);
undo_t      *pop_undo_list(undo_list_t *list);

undo_t *create_undo_state(game_grid_t *grid_data, player_t *players);
void free_undo_state(undo_t *undo_data);

game_grid_t *create_grid(int width, int height);
player_t *place_grid(game_grid_t *grid_data, player_t *this_player, player_t *players, int x, int y);
void free_grid(game_grid_t *grid);

void print_grid(game_grid_t *grid_data);
void print_stats(player_t *players);
int  count_player_owned_grids(game_grid_t *grid_data, player_t *this_player);

undo_t *insert_move(game_t *game_data, game_grid_t *grid_data, player_t *player, int x, int y);
void undo_last_move(game_t *game_data, game_grid_t *grid_data, player_t *players, undo_t *undo_data);

char **read_cmd_string(int *count);
int  parse_cmd_string(char **tokens, int count);
void free_cmd_string(char **tokens, int count);

#endif
