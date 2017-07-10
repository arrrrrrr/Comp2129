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
#include <math.h>

#define EPSILON 0.000001

enum { ADD, SUB, MUL, DIV };
enum { NUMBER, OPERATOR };

typedef struct expr_t {
    int type;
    double value;
} expr_t;

typedef struct entry_t entry_t;

struct entry_t {
    expr_t  element;
    entry_t *next;
};

void push_stack(entry_t **head, expr_t e);
int pop_stack(entry_t **head, expr_t *e);
void reverse_stack(entry_t **head);
void shuffle_expr(expr_t *expr, int format);
int apply_op(int *v, int o1, int o2, int o3, double expects);
void swap_expr(expr_t *x, expr_t *y);
void swap(int *x, int *y);
void permute(int *v, int l, int r);
int try_find_24(int *values);

void push_stack(entry_t **head, expr_t e) {
    if (*head == NULL) {
        entry_t *entry = malloc(sizeof(*entry));
        entry->element.type = e.type;
        entry->element.value = e.value;
        entry->next = NULL;
        *head = entry;
        return;
    }

    entry_t *entry = malloc(sizeof(*entry));
    entry->element.type = e.type;
    entry->element.value = e.value;
    entry->next = *head;

    *head = entry;

    return;
}

int pop_stack(entry_t **head, expr_t *e) {
    if (*head == NULL)
        return 0;

    if (e != NULL) {
        e->type = (*head)->element.type;
        e->value = (*head)->element.value;
    }

    entry_t *tmp = *head;
    *head = (*head)->next;

    free(tmp);
    return 1;
}

void reverse_stack(entry_t **head) {
    if (*head == NULL)
        return;

    entry_t *tmphead = *head, *newhead = NULL;

    while (tmphead != NULL) {
        entry_t *tmp = tmphead->next;
        tmphead->next = newhead;
        newhead = tmphead;
        tmphead = tmp;
    }

    *head = newhead;
}

char op_to_char(int op) {
    switch (op) {
        case ADD: return '+';
        case SUB: return '-';
        case MUL: return '*';
        case DIV: return '/';
    }

    return '\0';
}

void shuffle_expr(expr_t *expr, int format) {
    switch (format) {
        // (((i0 + i1) + i2) + i3)
        case 0:
            swap_expr(&expr[2],&expr[4]);
            swap_expr(&expr[3],&expr[4]);
            swap_expr(&expr[4],&expr[5]);
            break;
        // ((i0 (i1 + i2) + i3))
        case 1:
            swap_expr(&expr[3],&expr[4]);
            swap_expr(&expr[3],&expr[5]);
            swap_expr(&expr[5],&expr[6]);
            break;
        // (i0 + (i1 + (i2 + i3)))
        case 2:
            swap_expr(&expr[4],&expr[6]);
            break;
        // (((i0 + i1) + (i2 + i3)))
        case 3:
            swap_expr(&expr[2],&expr[4]);
            swap_expr(&expr[3],&expr[4]);
            swap_expr(&expr[5],&expr[6]);
            break;
    }
}

int *permutations;
int n_perm;

int apply_op(int *v, int o1, int o2, int o3, double expects) {
    // bracketing doesnt matter for add/sub/mul
    if (o1 == o2 == o3 && o1 != DIV) {
        double res = 0.0;
        switch (o1) {
            case ADD: res = (double)(v[0]+v[1]+v[2]+v[3]); break;
            case SUB: res = (double)(v[0]-v[1]-v[2]-v[3]); break;
            case MUL: res = (double)(v[0]*v[1]*v[2]*v[3]); break;
        }

        return res == expects;
    }

    for (int i = 0; i < 4; i++) {
        // rpn expr stack
        entry_t *rpn_head = NULL;

        expr_t etok[] = { (expr_t){ NUMBER, (double)v[0] }, (expr_t){ NUMBER, (double)v[1] },
                          (expr_t){ NUMBER, (double)v[2] }, (expr_t){ NUMBER, (double)v[3] },
                          (expr_t){ OPERATOR, (double)o1 }, (expr_t){ OPERATOR, (double)o2 },
                          (expr_t){ OPERATOR, (double)o3 }};
        // produce an rpn expr list
        shuffle_expr(&etok[0], i);

        // prepare the evaluation stack push reverse of the order we want to evaluate
        for (int j = 6; j >= 0; j--)
            push_stack(&rpn_head, etok[j]);

        // evaluation stack
        entry_t *eval_head = NULL;
        //
        expr_t expr_token;

        while(pop_stack(&rpn_head, &expr_token)) {
            // pop two numbers off the eval stack and apply the operator
            if (expr_token.type == OPERATOR) {
                expr_t eval_token1, eval_token2;
                // pop two numbers off the eval stack
                pop_stack(&eval_head, &eval_token1);
                pop_stack(&eval_head, &eval_token2);

                double res = 0.0;

                // apply the operator => tok2 OP tok1
                switch ((int)expr_token.value) {
                    case ADD: res = eval_token2.value + eval_token1.value; break;
                    case SUB: res = eval_token2.value - eval_token1.value; break;
                    case MUL: res = eval_token2.value * eval_token1.value; break;
                    case DIV: res = eval_token2.value / eval_token1.value; break;
                }

                // push result back onto the eval stack
                push_stack(&eval_head, (expr_t){ NUMBER, res });

                continue;
            }

            // push a number onto the eval stack
            push_stack(&eval_head, expr_token);
        }

        pop_stack(&eval_head, &expr_token);

        // if (expr_token.value >= 23.0 && expr_token.value <= 25.0)
        //     printf("result = %f\n", expr_token.value);

        if (fabs(expr_token.value - expects) < EPSILON)
            return 1;
    }

    return 0;
}

void swap_expr(expr_t *x, expr_t *y) {
    expr_t tmp;
    tmp = *x;
    *x = *y;
    *y = tmp;
}

void swap(int *x, int *y) {
    int temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

void permute(int *v, int l, int r) {
    int i;
    if (l == r) {
        int found = 0;
        for (int a = 0; a < n_perm; a++) {
            int pidx = a*4;
            if (permutations[pidx+0] == v[0] && permutations[pidx+1] == v[1] &&
                permutations[pidx+2] == v[2] && permutations[pidx+3] == v[3]) {
                found = 1;
                break;
            }
        }

        if (!found) {
            int idx = n_perm*4;
            permutations[idx+0] = v[0];
            permutations[idx+1] = v[1];
            permutations[idx+2] = v[2];
            permutations[idx+3] = v[3];
            n_perm++;
        }
    } else {
        for (i = l; i <= r; i++) {
            swap(&v[l], &v[i]);
            permute(v, l+1, r);
            swap(&v[l], &v[i]); //backtrack
        }
    }
}

int try_find_24(int *values) {
    for (int i = ADD; i <= DIV; i++) {
        for (int j = ADD; j <= DIV; j++) {
            for (int k = ADD; k <= DIV; k++) {

                for (int a = 0; a < n_perm; a++)
                    if (apply_op(&permutations[(a*4)],i,j,k,24.0))
                        return 1;
            }
        }
    }

    return 0;
}

int main(void) {
    int values[4] = {0};

    printf("Enter 4 integers: ");

    char buf[256] = {0}, c;
    int read = 0, tokens = 0;

    while ((c = getchar()) != '\n') {
        if (c == ' ')
            tokens++;
        buf[read++] = c;
    }

    if (tokens != 3) {
        printf("\nInput must consist of 4 integers\n");
        return 1;
    }

    if (sscanf(buf, "%d %d %d %d", &values[0], &values[1], &values[2], &values[3]) != 4) {
        printf("\nInput must consist of 4 integers\n");
        return 1;
    }

    permutations = malloc(sizeof(*permutations) * (24*4));
    n_perm = 0;

    permute(values, 0, 3);

    if (try_find_24(values))
        printf("\nYes! 24 is reachable from { %d, %d, %d, %d }\n",
               permutations[0], permutations[1], permutations[2], permutations[3]);
    else
        printf("\nNoooo :( 24 is unreachable from { %d, %d, %d, %d }\n",
               permutations[0], permutations[1], permutations[2], permutations[3]);

    free(permutations);

    return 0;
}