#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE(array) sizeof array / sizeof array[0]

typedef struct 
{
    char **data;
    size_t len;
    size_t capacity;
} expression;

void expr_init(expression *e){
    e->data = NULL; e->len = 0; e->capacity = 0;
}

int expr_push(expression *e, char *s){
    if (e->len == e->capacity){
        size_t new_capacity = (e->capacity) ? e->capacity*2 : 8;
        char **new_data_space = realloc(e->data, new_capacity * sizeof *new_data_space);
        if(!new_data_space) return 0;
        e->data = new_data_space;
        e->capacity = new_capacity;
    }
    e->data[e->len] = strdup(s);
    if (!e->data[e->len]) return 0;
    e->len++;
    return 1;
}

void expr_free(expression *e){
    free(e->data); e->data = NULL; e->len = 0; e->capacity = 0;
}

void expr_print(expression *e){
    char **p = e->data;

    for (size_t i = 0; i < e->len; i++){
        printf("%s", *(p+i));
        if (!(i==e->len-1)) printf(" ");
    }
    printf("\n");
}

int in(char x, char list[], unsigned long len);
char operators[] = {'+', '-', '*', '/', '^', '!'};
char notation[] = {'(', ')', ','};

int main()
{
    printf("Expression Simplifier\n");
    while(1){

        expression e;
        expr_init(&e);


        printf("Enter expression> ");
        char line[1024];
        int isFirstTerm = 1;
        if (fgets(line, sizeof line, stdin)){

            char *p = line;
            while(*p){
                // Skip whitespace and if end was reached while skipping break
                while(isspace((unsigned char) *p)) p++;
                if (!*p) break;

                // Parses out a long from str and returns the remaining str
                char *remainderPtr = NULL;
                long x = strtol(p, &remainderPtr, 10);
                
                // Integer Parsing: if pointers different then an int was read
                if (remainderPtr != p){

                    char op;
                    if (!isFirstTerm){
                        if (x >= 0) op = '+';
                        else if (x < 0) {
                            op = '-';
                            x = labs(x);
                        }
                        else break;
                        // printf("OperatorN: %c\n", op);
                        char s[2] = { op, '\0' };
                        expr_push(&e, s);
                    }

                    // printf("Integer: %ld\n", x);
                    char num[32];
                    snprintf(num, sizeof num, "%lu", x);
                    expr_push(&e, num);

                    // Push pointer p forward
                    p = remainderPtr;
                    
                    // Set zero after dealing with first term
                    isFirstTerm = 0;
                }
                // Character Parsing: read chars until next integer or end reached
                else{
                    while (*p && !isspace((unsigned char) *p)) {

                        char c = (*p++);
                        char s[2] = { c, '\0' };
                        if(in(c, operators, SIZE(operators))){
                            // printf("Operator: %c", c);
                            expr_push(&e, s);
                        }
                        else if(in(c, notation, SIZE(notation))){
                            // printf("Notation: %c", c);
                            expr_push(&e, s);
                        }
                        else if(isalpha(c)){
                            // printf("Variable: %c", c);
                            expr_push(&e, s);
                        }
                        else printf("Invalid Expression, Cannot Parse: %c\n", c);

                        // Checks if p is numerical 
                        // If so breaks out to start parsing integers again, otherwise the next char will be parsed
                        char *end = NULL;
                        char *check = p;
                        strtol(check, &end, 10);
                        if (end != check) break;
                    }
                }  
            }
            
            expr_print(&e);
        }

        expr_free(&e);
    }

    return 0;
}

int in(char x, char list[], unsigned long len){

    for (unsigned long i = 0; i < len; i++){
        if (list[i] == x) return 1;
    }
    return 0;
}
