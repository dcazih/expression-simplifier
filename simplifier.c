#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE(array) sizeof array / sizeof array[0]

int main()
{
    int in(char x, char list[], unsigned long len);

    char operators[] = {'+', '-', '*', '/', '^', '!'};
    char notation[] = {'(', ')', ','};


    char line[1024];   


    printf("Multinomial Calculator\n");
    while(1){
        
        printf("Enter expression> ");
        if (fgets(line, sizeof line, stdin)){
    
            int isFirstTerm = 1;

            char *p = line;
            while(*p){

                // Skip whitespace 
                // and if end was reached while skipping break
                while(isspace((unsigned char) *p)) p++;
                if (!*p) break;
    
                //
                char *remainderPtr = NULL;
                long x = strtol(p, &remainderPtr, 10);
                
                if (remainderPtr != p){

                    char op;
                    if (!isFirstTerm){
                        if (x >= 0) op = '+';
                        else if (x < 0) {
                            op = '-';
                            x = abs(x);
                        }
                        else break;
                        printf("Operator: %c\n", op);
                    }

                    printf("Integer: %ld\n", x);
                    // printf("p:%s, remptr: %s", p, remainderPtr);
                    p = remainderPtr;
                    
                    isFirstTerm = 0;
                }
                else{
                    while (*p && !isspace((unsigned char) *p)) {
                        char c = (*p++);
                        printf("Operator:%c", c);
                        // if(in(c, operators, SIZE(operators))){
                        //     printf("Operator: %c", c);
                        // }
                        // else if(in(c, notation, SIZE(notation))){
                        //     printf("Notation: %c", c);
                        // }
                        // else if(isalpha(c)){
                        //     printf("Variable: %c", c);
                        // }

                        printf("\n");
                    }
                }  
            }
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
