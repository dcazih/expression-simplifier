#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SIZE(array) sizeof array / sizeof array[0]

int isInt(char * p);
int in(char x, char list[], unsigned long len);
void longToString(long double l, char *out, size_t out_sz);

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

void expr_print(expression *e){

    for (size_t i = 0; i < e->len; i++){
        char *nextVal = (i + 1 < e->len) ? e->data[i + 1] : NULL;

        printf("%s", e->data[i]);
        if (nextVal && !isalpha(nextVal[0])) printf(" ");
    }
    printf("\n");
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

        // Catch +/- before strtof does
        while (*p == '+' && !isFirstTerm){
            char s[2] = { '+', '\0' };
            expr_push(e, strdup(s));
            p++;
            while(isspace((unsigned char) *p)) p++;
            if (!*p) break;
        }

        while (*p == '-'){
            char s[2] = { '-', '\0' };
            expr_push(e, strdup(s));
            p++;
            while(isspace((unsigned char) *p)) p++;
            if (!*p) break;
        }

        // Parses out a long from str and returns the remaining str
        char *remainderPtr = NULL;
        long double x = strtof(p, &remainderPtr);
        
        // Integer Parsing: if pointers different then an int was read
        if (remainderPtr != p){
            
            char num[32];
            snprintf(num, sizeof num, "%.6Lg", x);
            expr_push(e, strdup(num));

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
    int opCount = 0;
    int minusCount = 0;
    char *lastVal = NULL;
    for (size_t i = 0; i < e->len; i++){
        char * v = e->data[i];
        char *nextVal = (i + 1 < e->len) ? e->data[i + 1] : NULL;

        // Operator Checks:
        // Determine if values are operators
        int vIsOp = in(v[0], operators, SIZE(operators));
        int lastVIsOp = (lastVal == NULL) ? 0 : in(lastVal[0], operators, SIZE(operators));
        int nextVIsOp = (nextVal == NULL) ? 0 : in(nextVal[0], operators, SIZE(operators));

        // Returns 2 if multiple operations are used in sequence, with exception for minus
        if (vIsOp && strcmp(v, "-") && lastVIsOp && (strcmp(v, "-") || strcmp(lastVal, "-"))) return 1;
        if (lastVal && !strcmp(v, "-") && !strcmp(lastVal, "+") && !nextVIsOp){
            if (expr_lshift(e, i, 1)) return 2;
        }
        // printf("lastval:%s, currval:%s, lastVisOp:%d\n", lastVal, v, lastVIsOp);

        // Track the number of consecutive operators 
        if (in(v[0], operators, SIZE(operators))) ++opCount;
        else opCount = 0;
        if (!strcmp(v, "-")) ++minusCount;
        else minusCount = 0;
        // printf("ocount:%d, nextval:%s, nextop:%d\n", opCount, nextVal, nextVIsOp);

        // Return 1 if more than two consecutive minuses 
        if (opCount > 2 || (opCount == 2 && i == 1)) return 1;
        // Convert a double minus to positive
        else if (minusCount == 2 && !nextVIsOp){
            e->data[i] = "+";
            if (expr_lshift(e, i, 1)) return 2;
        }

       if (isInt(v)==1 && isInt(nextVal)==1) return 4;

        //////////////////////////////////////

        // Variable Checks:
        // Returns 3 if a number comes before a variable with no operator
        if (isalpha(v[0]) && nextVal != NULL && isInt(nextVal)==1) return 3;


        lastVal = v;
    }
    return 0;
}

int expr_eval(expression *e, int debug){

    size_t continueAtI=0;
    char str_buf[32];
    char *lastVal = NULL;
    char *lastLastVal = NULL;
    char *lastLastLastVal = NULL;
    while (continueAtI < e->len-1){
        for (size_t i = continueAtI; i < e->len; i++){
            if (debug) printf("starts %zu\n", i);
            char * v = e->data[i];
            if (i == 0 && (!strcmp(v, "+") || !strcmp(v, "-"))) {
                if (debug) printf("skipped %s\n", v);
                lastVal = v;
                continue;
            };

            char *nextVal = (i + 1 < e->len) ? e->data[i + 1] : NULL;
            char *nextNextVal = (i + 2 < e->len) ? e->data[i + 2] : NULL;
            // if (lastLastVal) printf("lastlast:%s last:%s, curr: %s\n", lastLastVal, lastVal, v);



            if (!strcmp(v, "*")){
                // Neg-Pos Mult
                if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                    long double product = strtof(lastVal, NULL) * (strtof(e->data[i+1], NULL));
                    longToString(product, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup(str_buf);
                    expr_lshift(e, i+2, 2);
                    break;
                }
                // Neg-Neg Mult
                else if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                    long double product = strtof(lastVal, NULL) * (strtof(e->data[i+2], NULL));
                    longToString(product, str_buf, SIZE(str_buf));
                    e->data[i-2] = strdup(str_buf);
                    expr_lshift(e, i+3, 4);
                    break;
                }
                // Pos-Pos Mult
                else if (lastVal && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                    long double product = strtof(lastVal, NULL) * strtof(nextVal, NULL);
                    longToString(product, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup(str_buf);
                    expr_lshift(e, i+2, 2);
                    break;
                }
                // Pos-Neg Mult
                else if (lastVal && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                    long double product = strtof(lastVal, NULL) * (strtof(e->data[i+2], NULL));
                    longToString(product, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup("-");
                    e->data[i] = strdup(str_buf);
                    expr_lshift(e, i+3, 2);
                    break;
                }

            }
            if (!strcmp(v, "/")){
                // Neg-Pos Division
                if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                    long double quotient = strtof(lastVal, NULL) / (strtof(e->data[i+1], NULL));
                    longToString(quotient, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup(str_buf);
                    expr_lshift(e, i+2, 2);
                    break;
                }
                // Neg-Neg Division
                else if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                    long double quotient = strtof(lastVal, NULL) / (strtof(e->data[i+2], NULL));
                    longToString(quotient, str_buf, SIZE(str_buf));
                    e->data[i-2] = strdup(str_buf);
                    expr_lshift(e, i+3, 4);
                    break;
                }
                // Pos-Pos Division
                else if (lastVal && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                    long double quotient = strtof(lastVal, NULL) / strtof(nextVal, NULL);
                    longToString(quotient, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup(str_buf);
                    expr_lshift(e, i+2, 2);
                    break;
                }
                // Pos-Neg Division
                else if (lastVal && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                    long double quotient = strtof(lastVal, NULL) / (strtof(e->data[i+2], NULL));
                    longToString(quotient, str_buf, SIZE(str_buf));
                    e->data[i-1] = strdup("-");
                    e->data[i] = strdup(str_buf);
                    expr_lshift(e, i+3, 2);
                    break;
                }

            }
            if (!strcmp(v, "+")){

                //////////////////////////////
                // Like-Term Variable Addition
                // Neg-Pos Addition (1-1 case)
                if (lastLastVal && nextVal && (lastLastVal[0] =='-') && 
                    isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Neg-Pos Addition (1-1 case)\n");
                    e->data[i - ((i <= 2) ? 2 : 1)] = "0";
                    expr_lshift(e, i+2, ((i <= 2) ? 3 : 2));
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;            
                }
                
                // Neg-Pos Addition (Multi-1 case)
                else if (lastLastLastVal && nextVal && (lastLastLastVal[0] =='-') && isInt(lastLastVal)==1 &&
                    isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Neg-Pos Addition (Multi-1 case)\n");

                    long double sum = (-strtof(lastLastVal, NULL) + 1.0);
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));
                    if (sum == 0){
                        if (i <= 3) e->data[i - 3] = "0";
                        expr_lshift(e, i + 2,  ((i <= 3) ? 4 : 5));
                    }
                    else if (sum == 1){
                        if (i <= 3){
                            expr_lshift(e, i+1, 4);
                        }
                        else{
                            e->data[i - 3] = "+";
                            expr_lshift(e, i+1, 3);
                        }
                    }
                    else if (sum == -1){
                        expr_lshift(e, i+1, 3);
                    }
                    else{ 
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+1, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Neg-Pos Addition (1-Multi case)
                else if (lastLastVal && nextNextVal && (lastLastVal[0] =='-')  && isalpha(lastVal[0]) &&
                        isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    if (debug) printf("Neg-Pos Addition (1-Multi case)\n");

                    long double sum = (-1.0 + strtof(nextVal, NULL));
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));
                    if (sum == 0){
                        e->data[i - ((i <= 2) ? 2 : 1)] = "0";
                        expr_lshift(e, i+3, ((i <= 2) ? 4 : 3));
                    }
                    else if (sum == 1){
                        if (!(i<=2)) e->data[i - 2] = "+";
                        expr_lshift(e,  i+2, (i<=2) ? 4 : 3);
                    }
                    else if (sum == -1){
                        expr_lshift(e, i+2, 3);
                    } 
                    // cannot be less that 0 only greater
                    else{
                        if (!(i<=2))  e->data[i - 2] = "+";
                        e->data[i - ((i <= 2) ? 2 : 1)] = strdup(str_buf);
                        expr_lshift(e, i+2, ((i <= 2) ? 3 : 2));
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Neg-Pos Addition (Multi-Multi case)
                else if (lastLastLastVal && nextNextVal && (lastLastLastVal[0] =='-') && isInt(lastLastVal)==1 && isalpha(lastVal[0]) 
                        && isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    if (debug) printf("Neg-Pos Addition (Multi-Multi case)\n");
                    long double sum = (-strtof(lastLastVal, NULL) + strtof(nextVal, NULL));
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));

                    if (sum == 0){
                        e->data[i - ((i <= 3) ? 3 : 2)] = strdup(str_buf);
                        expr_lshift(e, i+3, ((i <= 3) ? 5 : 4));
                    }
                    else if (sum == 1){
                        if (!(i<=2)) e->data[i - 3] = "+";
                        expr_lshift(e,  i+2, (i<=2) ? 5 : 4);
                    }
                    else if (sum == -1){
                        expr_lshift(e, i+2, 4);
                    }
                    else if (sum < 0){
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    else{
                        if (i <=3){
                            e->data[i-3] = strdup(str_buf);
                            expr_lshift(e, i+2, 4);
                        }
                        else{
                            e->data[i-3] = "+";
                            e->data[i-2] = strdup(str_buf);
                            expr_lshift(e, i+2, 3);
                        }
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Pos-Pos Addition (Multi-Multi case)
                if (lastLastVal && nextVal && nextNextVal && isInt(lastLastVal)==1 && isalpha(lastVal[0]) &&
                        isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    long double sum = (strtof(lastLastVal, NULL) + strtof(nextVal, NULL));
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));
                    if (debug) printf("Pos-Pos Addition (Multi-Multi case)\n");

                    if (sum == 0){
                        e->data[i - ((i <= 3) ? 3 : 2)] = strdup(str_buf);
                        expr_lshift(e, i+3, ((i <= 3) ? 5 : 4));
                    }
                    else if (sum == 1){
                        expr_lshift(e, i+2, 4);
                    }
                    else{
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Pos-Pos Addition (Multi-1 case)
                else if (lastLastVal && nextVal && isInt(lastLastVal)==1 && isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    long double sum = (strtof(lastLastVal, NULL) + 1.0);
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));
                    if (debug) printf("Pos-Pos Addition (Multi-1 case)\n");

                    
                    if (sum == 1){
                        expr_lshift(e, i+1, 3);
                    }
                    else{ 
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+1, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }                   
                
                // Pos-Pos Addition (1-Multi case)
                else if (lastVal && nextNextVal  && isalpha(lastVal[0]) && isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    long double sum = (1.0 + strtof(nextVal, NULL));
                    longToString(fabsl(sum), str_buf, SIZE(str_buf));
                    if (debug) printf("Pos-Pos Addition (1-Multi case)\n");

                    if (sum == 1){
                        expr_lshift(e, i+2, 3);
                    }                  
                    else{
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }              
                
                // Pos-Pos Addition (1-1 case)
                else if (lastVal && nextVal && isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Pos-Pos Addition (1-1 case)\n");
                    e->data[i-1] = "2";
                    expr_lshift(e, i+1, 1);
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }

                /////////////////////////////
                // Integer Addition
                if (!nextNextVal || !isalpha((unsigned char)nextNextVal[0])){                                                                    
                    // Neg-Pos Addition
                    if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                        if(debug) printf("Neg-Pos Addition \n");
                        long double sum = (-1 * strtof(lastVal, NULL)) + (strtof(e->data[i+1], NULL));
                        if (sum < 0 ){
                            longToString(fabsl(sum), str_buf, SIZE(str_buf));
                            e->data[i-1] = strdup(str_buf);
                            expr_lshift(e, i+2, 2);
                        }
                        else{
                            longToString(sum, str_buf, SIZE(str_buf));
                            e->data[i-2] = strdup(str_buf);
                            expr_lshift(e, i+3, 3);
                        }
                        if(debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }
                    // Neg-Neg Addition
                    else if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                        if(debug) printf("Neg-Neg Addition \n");
                        long double sum = strtof(lastVal, NULL) + (strtof(e->data[i+2], NULL));
                        longToString(sum, str_buf, SIZE(str_buf));
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+3, 3);
                        if(debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }
                    // Pos-Pos Addition
                    else if (lastVal && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                        if(debug) printf("Pos-Pos Addition \n");
                        long double sum = strtof(lastVal, NULL) + strtof(nextVal, NULL);
                        longToString(sum, str_buf, SIZE(str_buf));
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                        if(debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }
                    // Pos-Neg Addition
                    else if (lastVal && nextVal && isInt(lastVal)==1 && nextVal[0] == '-'){
                        if(debug) printf("Pos-Neg Addition \n");
                        e->data[i] = strdup("-");
                        expr_lshift(e, i+1, 1);
                        if(debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }

                }
            
            }
            if (!strcmp(v, "-")){
                ////////////////////////////
                // Variable Subtraction
                // Neg-Pos Subtraction (Multi-Multi case)
                if (lastLastLastVal && nextNextVal && (lastLastLastVal[0] =='-') && isInt(lastLastVal)==1 &&
                    isalpha(lastVal[0]) && isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    long double difference = (-strtof(lastLastVal, NULL) - strtof(nextVal, NULL));
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    if (debug) printf("Neg-Pos Subtraction (Multi-Multi case)\n");

                    if (difference == 0){
                        e->data[i - ((i <= 3) ? 3 : 2)] = strdup(str_buf);
                        expr_lshift(e, i+3, ((i <= 3) ? 5 : 4));
                    }
                    else if (difference == 1){
                        expr_lshift(e, i+2, 5);
                    }
                    else if (difference == -1){
                        expr_lshift(e, i+2, 4);
                    }
                    else if (difference < 0){                 
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }                 
                
                // Neg-Pos Subtraction (Multi-1 case)
                else if (lastLastLastVal && nextVal && (lastLastLastVal[0] =='-') && isInt(lastLastVal)==1 &&
                    isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Neg-Pos Subtraction (Multi-1 case)\n");
                    
                    long double difference = (-strtof(lastLastVal, NULL) - 1.0);
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    
                    if (difference == 0){
                        e->data[i - ((i <= 3) ? 3 : 2)] = strdup(str_buf);
                        expr_lshift(e, i+3, ((i <= 3) ? 4 : 3));
                    }
                    else if (difference == -1){
                        expr_lshift(e, i+1, 3);
                    }
                    else{ 
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+1, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Neg-Pos Subtraction (1-Multi case)
                else if (lastLastVal && nextNextVal && (lastLastVal[0] =='-')  &&
                    isalpha(lastVal[0]) && isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    long double difference = (-1.0 - strtof(nextVal, NULL));
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    if (debug) printf("Neg-Pos Subtraction (1-Multi case)\n");
                    if (difference == 1 ){
                        expr_lshift(e, i+2, 4);
                    }
                    else if (difference == -1){
                        expr_lshift(e, i+2, 3);
                    }
                    else if (difference < 0){
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                    }
                    else{
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }
                
                // Neg-Pos Subtraction (1-1 case)
                else if (lastLastVal && nextVal && (lastLastVal[0] =='-') && 
                    isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Neg-Pos Subtraction (1-1 case)\n");
                    e->data[i-1] = "2";
                    expr_lshift(e, i+1, 1);
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;          
                }
                
                // Pos-Pos Subtraction (Multi-Multi case)
                if (lastLastVal && nextNextVal && isInt(lastLastVal)==1 && isalpha(lastVal[0]) &&
                        isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    if (debug) printf("Pos-Pos Subtraction (Multi-Multi case)\n");
                    
                    long double difference = (strtof(lastLastVal, NULL) - strtof(nextVal, NULL));
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    if (difference == 0){
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+3, 4);
                    }
                    else if (difference == 1){
                        expr_lshift(e, i+2, 4);
                    }
                    else if (difference == -1){
                        e->data[i-3] = "-";
                        expr_lshift(e, i+2, 4);
                    }
                    else if (difference < 0){
                        e->data[i-3] = "-";
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    else{
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+2, 3);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;                   
                }
                
                // Pos-Pos Subtraction (Multi-1 case)
                else if (lastLastVal && nextVal && isInt(lastLastVal)==1 && isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Pos-Pos Subtraction (Multi-1 case)\n");
                    long double difference = (strtof(lastLastVal, NULL) - 1.0);
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    if (difference == 0){
                        if (i <= 3) e->data[i - 2] = strdup(str_buf);
                        expr_lshift(e, i+2, ((i <= 3) ? 3 : 5));
                    }
                    else if (difference == 1){
                        expr_lshift(e, i+1, 3);
                    }
                    else if (difference == -1){
                        expr_lshift(e, i, 3);
                    }
                    else{ 
                        e->data[i-2] = strdup(str_buf);
                        expr_lshift(e, i+1, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }                   
                
                // Pos-Pos Subtraction (1-Multi case)
                else if (lastVal && nextNextVal  && isalpha(lastVal[0]) && isInt(nextVal)==1 && isalpha(nextNextVal[0]) && lastVal[0] == nextNextVal[0]){
                    if (debug) printf("Pos-Pos Subtraction (1-Multi case)\n");
                    long double difference = (1.0 - strtof(nextVal, NULL));
                    longToString(fabsl(difference), str_buf, SIZE(str_buf));
                    if (difference == 0){
                        if (i <= 3) e->data[i - 2] = strdup(str_buf);
                        expr_lshift(e, i+2, ((i <= 3) ? 3 : 5));
                    }
                    else if (difference == 1){
                        expr_lshift(e, i+2, 3);
                    }  
                    else if (difference == -1){
                        e->data[i - 2] = "-";
                        expr_lshift(e, i+2, 3);
                    } 
                    else if (difference < 0){
                        e->data[i - 2] = "-";
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                    }                       
                    else{
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                    }
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;
                }              
                
                // Pos-Pos Subtraction (1-1 case)
                else if (lastVal && nextVal && isalpha(lastVal[0]) && isalpha(nextVal[0]) && lastVal[0] == nextVal[0]){
                    if (debug) printf("Pos-Pos Subtraction (1-1 case)\n");
                    
                    if (i <= 2) e->data[i-1] = "0";
                    expr_lshift(e, i+2, ((i <= 2) ? 2 : 4));
                    if (debug) expr_print(e);
                    continueAtI = 0;
                    break;            
                }

                ////////////////////////////
                // Integer Subtraction                      
                if (!nextNextVal || !isalpha((unsigned char)nextNextVal[0])){
                    // Neg-Pos Subtraction
                    if (lastLastVal && (lastLastVal[0] =='-') && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1 ){
                        long double difference = strtof(lastVal, NULL) + strtof(nextVal, NULL);
                        longToString(fabsl(difference), str_buf, SIZE(str_buf));
                        if (debug) printf("Neg-Pos Subtraction \n");
                        e->data[i-2] = "-";
                        e->data[i-1] = strdup(str_buf);
                        expr_lshift(e, i+2, 2);
                        if (debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }
                    // Pos-Pos Subtraction
                    else if (lastVal && nextVal && isInt(lastVal)==1 && isInt(nextVal)==1){
                        long double difference = strtof(lastVal, NULL) - strtof(nextVal, NULL);

                        if(debug) printf("Pos-Pos Subtraction <0 \n");
                        if (difference < 0 ){
                            longToString(fabsl(difference), str_buf, SIZE(str_buf));
                            e->data[i-1] = "-";
                            e->data[i] = strdup(str_buf);
                            expr_lshift(e, i+2, 1);
                        }
                        else{
                            longToString(difference, str_buf, SIZE(str_buf));
                            e->data[i-1] = strdup(str_buf);
                            expr_lshift(e, i+2, 2);
                        }
                        if (debug) expr_print(e);
                        continueAtI=0;
                        break;
                    }
                }
            }

            continueAtI=i;
            if (i>1) lastLastLastVal = lastLastVal;
            if (i>0) lastLastVal = lastVal;
            lastVal = v;
        }
    }

    return 0;
}

char *expr_curr(expression *e){
    return e->data[e->len-1];
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
                else if (validationError == 4){
                    printf("Error: Missing operator between integers\n");
                }
                else if(validationError != -1) {
                    // Evaluate Expression
                    int evaluationError = 0;//expr_eval(&e, 0);
                    if (evaluationError){
                    }
                    else expr_print(&e);
                }
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
    strtof(check, &end);
    if (end != check) return 1;
    return 0;
}

void longToString(long double l, char *out, size_t out_sz) {
    snprintf(out, out_sz, "%Lg", l);
}
