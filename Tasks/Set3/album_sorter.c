#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
#define strtok_s strtok_r
#endif

#define MAX_FILE_LINE 8192
#define MAX_LINE 256
#define MAX_COLUMNS 4

typedef struct album_entry_t album_entry_t;

enum sort_dir_t { SORT_ASC, SORT_DESC };
enum cmd_t { CMD_ERROR, CMD_DISPLAY, CMD_SORT, CMD_QUIT };

struct album_entry_t {
    char *name;
    int year;
    char *genre;
    char *artist;
};

void print_entries(album_entry_t *ae, int count);

int cmp_name_asc(const void *a, const void *b);
int cmp_name_desc(const void *a, const void *b);
int cmp_year_asc(const void *a, const void *b);
int cmp_year_desc(const void *a, const void *b);
int cmp_genre_asc(const void *a, const void *b);
int cmp_genre_desc(const void *a, const void *b);
int cmp_artist_asc(const void *a, const void *b);
int cmp_artist_desc(const void *a, const void *b);

char **read_cmd_string(int *count, const char *delim);
void free_cmd_string(char **tokens, int count);
int parse_cmd_string(char **tokens, int count);
void free_album_entries(album_entry_t *ae, int count);

void print_entries(album_entry_t *ae, int count) {
    for (int i = 0; i < count; i++)
        printf("%s, %d, %s, %s\n",
               ae[i].name,
               ae[i].year,
               ae[i].genre,
               ae[i].artist);
}


int cmp_name_asc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(ea->name, eb->name);
}

int cmp_name_desc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(eb->name, ea->name);
}

int cmp_year_asc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return (ea->year < eb->year) ? -1 : ((ea->year > eb->year) ? 1 : 0);
}

int cmp_year_desc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return (eb->year < ea->year) ? -1 : ((eb->year > ea->year) ? 1 : 0);
}

int cmp_genre_asc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(ea->genre, eb->genre);
}

int cmp_genre_desc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(eb->genre, ea->genre);
}

int cmp_artist_asc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(ea->artist, eb->artist);
}

int cmp_artist_desc(const void *a, const void *b) {
    album_entry_t *ea = (album_entry_t *)a;
    album_entry_t *eb = (album_entry_t *)b;

    return strcmp(eb->artist, ea->artist);
}

// read a command and tokenize into a command string
// must be matched with free_cmd_string
char **read_cmd_string(int *count, const char *delim) {
    char *buf = NULL, c;
    int read = 0;

    *count = 0;

    // allocate a temp buffer
    buf = malloc(sizeof(*buf)*(MAX_LINE+1));
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
    char *token  = strtok(tmpbuf, delim);

    // count tokens so we can allocate an array of pointers to return
    while (token != NULL) {
        (*count)++;
        token = strtok(NULL, delim);
    }

    // found no tokens
    if (*count == 0) {
        free(buf);
        free(tmpbuf);
        return NULL;
    }

    char **tokens = malloc((*count) * sizeof(*tokens));

    // copy the buffer back to the temp buffer to retokenize
    memcpy(tmpbuf, buf, sizeof(*tmpbuf)*(read+1));
    // do the allocation for the token strings
    token = strtok(tmpbuf, delim);

    for (int i = 0; token != NULL; i++) {
        // allocate each token
        tokens[i] = malloc((strlen(token)+1) * sizeof(**tokens));
        // copy the token
        memcpy(tokens[i], token, strlen(token)+1);
        token = strtok(NULL, delim);
    }

    // free the buffer
    free(tmpbuf);
    free(buf);

    return tokens;
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

int parse_cmd_string(char **tokens, int count) {
    if (strcmp(tokens[0],"QUIT") == 0) {
        return CMD_QUIT;
    } else if (strcmp(tokens[0], "DISPLAY") == 0) {
        return CMD_DISPLAY;
    } else if (strcmp(tokens[0], "SORT") == 0) {
        if (count >= 2) {
            int col = atoi(tokens[1]);
            if (col < 0 || col >= MAX_COLUMNS)
                return CMD_ERROR;

            if (count == 3 && !(strcmp(tokens[2],"ASC") == 0 || strcmp(tokens[2],"DESC") == 0))
                return CMD_ERROR;
        }
        return CMD_SORT;
    } else {
        return CMD_ERROR;
    }
}

void free_album_entries(album_entry_t *ae, int count)  {
    if (ae == NULL || count == 0)
        return;

    for (int i = 0; i < count; i++) {
        free(ae[i].name);
        free(ae[i].genre);
        free(ae[i].artist);
    }

    free(ae);
}

int main(int argc, char** argv) {
    int quit = 0;
    int n_album_entry = 0;
    album_entry_t *ae = NULL;

    if (argc == 1) {
        printf("No File Specified\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL) {
        printf("File Does Not Exist\n");
        return 1;
    }

    // get the file size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *buf = malloc(sizeof *buf * fsize+1);
    // read the file into a buffer
    fread(buf, 1, fsize, fp);
    buf[fsize] = '\0';

    fclose(fp);

    char *tmpbuf = strdup(buf);
    char *token = strtok(tmpbuf, "\n");
    // count valid entries
    while (token != NULL) {
        if (strlen(token) > 0)
            n_album_entry++;
        token = strtok(NULL, "\n");
    }
    // free the duplicated buffer
    free(tmpbuf);
    // allocate enough for the album entries
    ae = malloc(sizeof *ae * n_album_entry);
    // now begin filling the entries
    tmpbuf = strdup(buf);

    char *strl, *svptr1, *svptr2, *subtoken;
    int curr_entry = 0;

    for (strl = tmpbuf; ; strl = NULL) {
        token = strtok_s(strl, "\n", &svptr1);

        if (token == NULL)
            break;

        int curr_sep = 0;
        char *strf;

        for (strf = token; ; curr_sep++, strf = NULL) {
            subtoken = strtok_s(strf, ",", &svptr2);

            if (subtoken == NULL) {
                if (curr_sep == 0)
                    break;

                curr_entry++;
                break;
            }

            switch (curr_sep) {
                case 0: ae[curr_entry].name = strdup(subtoken); break;
                case 1: ae[curr_entry].year = atoi(subtoken); break;
                case 2: ae[curr_entry].genre = strdup(subtoken); break;
                case 3: ae[curr_entry].artist = strdup(subtoken); break;
            }
        }

    }

    // free the duplicated buffer
    free(tmpbuf);
    free(buf);

    while (!quit) {
        int cmd_count = 0;
        const char *delim = " ";
        char **cmd = read_cmd_string(&cmd_count, delim);

        int cmd_id = parse_cmd_string(cmd, cmd_count);

        if (cmd_id == CMD_ERROR) {
            printf("Invalid Command\n");
        } else if (cmd_id == CMD_DISPLAY) {
            print_entries(ae, n_album_entry);
        } else if (cmd_id == CMD_QUIT) {
            quit = 1;
        } else if (cmd_id == CMD_SORT) {
            int col = atoi(cmd[1]);
            int order = SORT_ASC;

            if (cmd_count == 3) {
                if (strcmp(cmd[2],"DESC") == 0)
                    order = SORT_DESC;
            }

            // function pointer to the compare function for qsort
            int (*pcmp)(const void *,const void *) = NULL;

            switch (col) {
                case 0:
                    pcmp = (order == SORT_ASC) ? cmp_name_asc : cmp_name_desc;
                    break;
                case 1:
                    pcmp = (order == SORT_ASC) ? cmp_year_asc : cmp_year_desc;
                    break;
                case 2:
                    pcmp = (order == SORT_ASC) ? cmp_genre_asc : cmp_genre_desc;
                    break;
                case 3:
                    pcmp = (order == SORT_ASC) ? cmp_artist_asc : cmp_artist_desc;
                    break;
            }

            qsort(ae, n_album_entry, sizeof(album_entry_t), pcmp);
        }

        free_cmd_string(cmd, cmd_count);
    }

    free_album_entries(ae, n_album_entry);

    return 0;
}

