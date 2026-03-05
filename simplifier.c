#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE(array) sizeof array / sizeof array[0]

int isInt(char * p);
int in(char x, char list[], unsigned long len);
void longToString(long l, char *out, size_t out_sz);

char operators[] = { '*', '/', '+', '-'};
char notation[] = {'(', ')', ','};


// Expression List STRUCTURE
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

int expr_lshift(expression *e, int index, int shift){
    // Returns 1 if shift failed
    if (index - shift < 0) return 1;
    for (size_t i = index; i < e->len; i++){
        e->data[i-shift] = e->data[i];
    }
    e->len = e->len - shift;
    return 0;
}

int expr_parse(expression *e, char *line){
    int parseError = 0;
    int invalidExpr = 0;
    int isFirstTerm = 1;

    char *p = line;
    while(*p){
        // Skip whitespace and if end was reached while skipping break
        while(isspace((unsigned char) *p)) p++;
        if (!*p) break;

        while (*p == '+' && !isFirstTerm){
            char s[2] = { '+', '\0' };
            expr_push(e, strdup(s));
            p++;
            while(isspace((unsigned char) *p)) p++;
            if (!*p) break;
        }

        // Parses out a long from str and returns the remaining str
        char *remainderPtr = NULL;
        long x = strtol(p, &remainderPtr, 10);
        
        // Integer Parsing: if pointer3s different then an int was read
        if (remainderPtr != p){

            if (x < 0 ) {
                // printf("OperatorN: %c\n", op);
                char op = '-';
                char s[2] = {  op, '\0' };
                expr_push(e, strdup(s));
                x = labs(x);
            }

            // printf("Integer: %ld\n", x);
            char num[32];
            snprintf(num, sizeof num, "%ld", x);
            expr_push(e, num);

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
                    if (isFirstTerm && c != '-'){
                        invalidExpr = 1;
                        break;
                    }else expr_push(e, strdup(s));
                }
                else if(in(c, notation, SIZE(notation))){
                    // printf("Notation: %c", c);
                    expr_push(e, strdup(s));
                }
                else if(isalpha(c)){
                    // printf("Variable: %c", c);
                    expr_push(e, strdup(s));
                }
                else {
                    printf("Cannot Parse: %c\n", c);
                    invalidExpr = 1;
                }


                // Set zero after dealing with first term
                isFirstTerm = 0;
                
                // Checks if p is numerical 
                // If so breaks out to start parsing integers again, otherwise the next char will be parsed
                if(isInt(p)) break;
            }
        } 
        if (invalidExpr) {parseError = 1; break;}
    }
    return parseError;
}

int expr_validate(expression *e){
    // Returns 0 if parse successful
    if (e->len == 0) return -1;
    int minusCount = 0;
    char *lastVal = NULL;
    for (size_t i = 0; i < e->len; i++){
        char * v = e->data[i];
        char *nextVal = (i != e->len-1) ? e->data[i+1] : NULL;

        // Operator Checks:
        // Determine if values are operators
        int vIsOp = in(v[0], operators, SIZE(operators));
        int lastVIsOp = (lastVal == NULL) ? 0 : in(lastVal[0], operators, SIZE(operators));
        int nextVIsOp = (nextVal == NULL) ? 0 : in(nextVal[0], operators, SIZE(operators));

        // Returns 2 if multiple operations are used in sequence, with exception for minus
        if (vIsOp && strcmp(v, "-") && lastVIsOp && (strcmp(v, "-") || strcmp(lastVal, "-"))) return 1;
        else if (lastVal && !strcmp(v, "-") && !strcmp(lastVal, "+")){
            if (expr_lshift(e, i, 1)) return 2;
        }
        // printf("lastval:%s, currval:%s, lastVisOp:%d\n", lastVal, v, lastVIsOp);

        // Track the number of consecutive minuses 
        if (strcmp(v, "-") == 0) ++minusCount;
        else minusCount = 0;
        // printf("mcount:%d, nextval:%s, nextop:%d\n", minusCount, nextVal, nextVIsOp);

        // Return 1 if more than two consecutive minuses 
        if (minusCount > 2 || (minusCount == 2 && i == 1)) return 1;
        // Convert a double minus to positive
        else if (minusCount == 2 && !nextVIsOp){
            e->data[i] = "+";
            if (expr_lshift(e, i, 1)) return 2;
        }

        //////////////////////////////////////

        // Variable Checks:
        // Returns 3 if a number comes before a variable with no operator
        if (isalpha(v[0]) && nextVal != NULL && isInt(nextVal)) return 3;

        lastVal = v;
    }
    return 0;
}

int expr_eval(expression *e){

    for(size_t o = 0; o < SIZE(operators); o++){
        int op = operators[o];

        size_t contI=0;
        char *lastVal = NULL;
        while (contI != e->len-1){
            for (size_t i = contI; i < e->len; i++){
                char * v = e->data[i];
                char *nextVal = (i != e->len-1) ? e->data[i+1] : NULL;

                if (op == '*' && !strcmp(v, "*")){
                    if (lastVal && nextVal && isInt(lastVal) && isInt(nextVal)){
                        char p[32];
                        long product = strtol(lastVal, NULL, 10) * strtol(nextVal, NULL, 10);
                        longToString(product, p, SIZE(p));
                        e->data[i-1] = strdup(p);
                        expr_lshift(e, i+2, 2);
                        break;
                    }
                }

                contI=i;
                lastVal = v;
            }
        }
    }

    return 0;
}

char *expr_curr(expression *e){
    return e->data[e->len-1];
}

void expr_print(expression *e){
    char **p = e->data;

    for (size_t i = 0; i < e->len; i++){
        printf("%s", *(p+i));
        if (!(i==e->len-1)) printf(" ");
    }
    printf("\n");
}

void expr_free(expression *e){
    free(e->data); e->data = NULL; e->len = 0; e->capacity = 0;
}


///////////////////////////////


int main()
{
    printf("Expression Simplifier\n");
    while(1){

        expression e;
        expr_init(&e);

        printf("Enter expression> ");

        char line[1024];
        if (fgets(line, sizeof line, stdin)){

            // Parse Expression 
            int parseError = expr_parse(&e, line);
            if (parseError) {
                printf("Error: Invalid Expression\n");
            }
            else{
                // Validate Expression
                int validationError = expr_validate(&e);
                if (validationError == 1){
                    printf("Error: Multiple operations used in sequence\n");
                }
                else if (validationError == 2){
                    printf("Error Internal: Shift\n");
                }
                else if (validationError == 3){
                    printf("Error: Number before variable\n");
                }
                else if(validationError != -1) expr_print(&e);
                
                // Evaluate Expression
                int evaluationError = expr_eval(&e);
                if (!validationError && evaluationError){
                }
                else if(validationError != -1) expr_print(&e);

            }

            expr_free(&e);            
        }
    }

    return 0;
}

int in(char x, char list[], unsigned long len){

    for (unsigned long i = 0; i < len; i++){
        if (list[i] == x) return 1;
    }
    return 0;
}

int isInt(char * p){
    // Returns 0 if true, 1 if false, -1 if NULL
    if (p == NULL) return -1;
    char *end = NULL;
    char *check = p;
    strtol(check, &end, 10);
    if (end != check) return 1;
    return 0;
}

void longToString(long l, char *out, size_t out_sz) {
    snprintf(out, out_sz, "%ld", l);
}