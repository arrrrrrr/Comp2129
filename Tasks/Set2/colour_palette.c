#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//You may add/modify/remove structs

typedef struct list_t list_t;
typedef struct node_t node_t;
typedef struct pixel_t pixel_t;
typedef struct image_header_t image_header_t;
typedef struct image_data_t image_data_t;

struct pixel_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

struct node_t {
    pixel_t  pixel;
    node_t   *next;
};

struct list_t {
    node_t *head;
    node_t *tail;
};

struct image_header_t {
    uint32_t width;
    uint32_t height;
    uint16_t magic;
};

struct image_data_t {
    image_header_t header;
    list_t *pixels;
};


list_t *create_list();
void free_list(list_t *list);

list_t *create_list() {
    list_t *list = malloc(sizeof(list_t));
    list->head = list->tail = NULL;

    return list;
}

void free_list(list_t *list) {
    if (list != NULL) {
        node_t *curr = list->head, *tmp = NULL;

        while (curr != NULL) {
            tmp = curr;
            curr = curr->next;
            //printf("freeing => node = %p\n",tmp);
            free(tmp);
        }

        free(list);
    }
}

void push_list(list_t *list, pixel_t *pixel) {
    node_t *node = malloc(sizeof(node_t));
    memset(node, 0, sizeof(node_t));

    //printf("allocating => node = %p\n",node);

    node->pixel.red   = pixel->red;
    node->pixel.green = pixel->green;
    node->pixel.blue  = pixel->blue;
    node->pixel.alpha = pixel->alpha;

    if (list->head == NULL) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
}

node_t *head_list(list_t *list) {
    return list->head;
}

int find_list(list_t *list, pixel_t *pix) {
    node_t *curr = head_list(list);

//        printf("pixel[%2d] => red = %3d, green = %3d, blue = %3d, alpha = %3d\n",
//               i, curr->pixel.red, curr->pixel.green, curr->pixel.blue, curr->pixel.alpha);
    int i = 0;
    while (curr != NULL) {
        if (curr->pixel.red   == pix->red   &&
            curr->pixel.green == pix->green &&
            curr->pixel.blue  == pix->blue)
        {
            return i;
        }
        curr = curr->next;
        i++;
    }

    return -1;
}

pixel_t unpack_rgba_bytes(uint32_t packed) {
    pixel_t pixel = {0};
    pixel.red   = (uint8_t)(packed & 0xff);
    pixel.green = (uint8_t)((packed >> 8) & 0xff);
    pixel.blue  = (uint8_t)((packed >> 16) & 0xff);

    return pixel;
}

image_data_t *read_image(char *filename) {
    FILE *fp = fopen(filename,"rb");

    if (fp == NULL) {
        printf("File Does Not Exist\n");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    image_data_t *img = malloc(sizeof(image_data_t));
    //printf("allocating => img = %p\n",img);

    if (fread(&img->header, 10, 1, fp) < 1 || img->header.magic != 0xec77) {
        printf("Invalid Image Header\n");
        free(img);
        return NULL;
    }
/*
    printf("img->header.width  = %08x\nimg->header.height = %08x\nimg->header.magic  = %04x\n\n",
            img->header.width, img->header.height, img->header.magic);
*/
    int pixels = img->header.width*img->header.height;

    if (fsize != 10+(pixels*4)) {
        printf("Invalid Image Data\n");
        free(img);
        return NULL;
    }

    img->pixels = create_list();

    uint32_t data = 0;

    while (fread(&data, sizeof(uint32_t), 1, fp) == 1) {
        pixel_t px = unpack_rgba_bytes(data);
        push_list(img->pixels, &px);
    }

    fclose(fp);

    return img;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("No Filename Specified\n");
        return 1;
    }

    char *filename = strdup(argv[1]);

    image_data_t *img = read_image(filename);

    free(filename);

    if (img == NULL)
        return 1;

    int *plane = malloc(sizeof(int)*img->header.width*img->header.height);
    memset(plane, 0, sizeof(int)*img->header.width*img->header.height);

    list_t *unique = create_list();

    node_t *curr = head_list(img->pixels);

    int i = 0;

    while (curr != NULL) {
        int idx = 0;
        if ((idx = find_list(unique, &curr->pixel)) < 0) {
            push_list(unique, &curr->pixel);
            idx = find_list(unique, &curr->pixel);
        }

        plane[i] = idx;
        curr = curr->next;
        i++;
    }

    for (int i = 0; i < img->header.height; i++) {
        printf("[");
        for (int j = 0; j < img->header.width; j++) {
            int idx = i*img->header.width+j;
            printf("%s%2d",(j==0) ? "" : ",", plane[idx]);
        }
        printf(" ]\n");
    }

    free_list(unique);
    free_list(img->pixels);
    //printf("freeing => img = %p\n",img);
    free(plane);
    free(img);

    // TODO
    return 0;
}