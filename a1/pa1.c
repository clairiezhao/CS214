#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    char line[1200];
    char *numtok;
    int row = 0, col = 0, target_row, target_col;
    unsigned int curr_num, max, min;
    double mean = 0;

    FILE *input = fopen(argv[1], "r");
    if(input == NULL) {
        puts("Error opening file");
    }

    if(strcmp(argv[2], "r") == 0) {

        target_row = strtol(argv[3], NULL, 0);
        while(fgets(line, 1200, input) != NULL && row != target_row) {
            row ++;
        }

        if(row == target_row) {
            numtok = strtok(line, ",");

            while(numtok != NULL) {
                curr_num = strtol(numtok, NULL, 0);

                if(col == 0) {
                    max = curr_num;
                    min = curr_num;
                }

                if(curr_num > max) {
                    max = curr_num;
                }
                if(curr_num < min) {
                    min = curr_num;
                }

                mean += curr_num;
                col ++;
                numtok = strtok(NULL, ",");
            }

            mean = mean/col;
            printf("%s row %.2f %u %u\n", argv[1], mean, max, min);
        }
        else {
            printf("error in input format at line %d\n", target_row);
            return -1;
        }
    }

    else if(strcmp(argv[2], "c") == 0) {

        target_col = strtol(argv[3], NULL, 0);

        while (fgets(line, 1200, input) != NULL) {
            
            col = 0;
            numtok = strtok(line, ",");
            
            while(numtok != NULL && col != target_col) {
                numtok = strtok(NULL, ",");
                col ++;
            }

            if(numtok != NULL) {
                curr_num = strtol(numtok, NULL, 0);

                if(row == 0) {
                    max = curr_num;
                    min = curr_num;
                    mean = curr_num;
                }
                else {
                    if(curr_num > max) {
                        max = curr_num;
                    }
                    if(curr_num < min) {
                        min = curr_num;
                    }
                    mean += curr_num;
                }

                row ++;
            }
            else {
                printf("error in input format at line %d\n", target_col);
                return -1;
            }
        }

        mean = mean/row;
        printf("%s column %.2f %u %u\n", argv[1], mean, max, min);
    }
    
    return 0;
}