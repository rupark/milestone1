//lang::CwC
#include <stdlib.h> 
#include <stdio.h>      /* files */
#include "../string.h"     /* strcmp */

#include "sorer.h"
#include "../column.h"
#include "../helper.h"
#include "../dataframe.h"

#include "../parser.h"
#include "../column.h"

int main(int argc, char** argv) {

    // have these flags appeared
    bool a_file, a_from, a_len;
    a_file = a_from = a_len = false;
    int a_type, a_print, a_missing;
    a_type = a_print = a_missing = 0;
    char *file_name;
    unsigned int from, len;
    unsigned int col_arg1, col_arg2;

    // if user passes in non integers for places where there should be integers
    // just default the value to 0
    int index = 1; // skipping the first argument
    while (index < argc) {
        if (strcmp(argv[index], "-f") == 0) {
            check_in_bounds(argc, index, 1);
            file_name = argv[index + 1];
            a_file = true;
            index+=2;
        }
        else if (strcmp(argv[index], "-from") == 0) {
            check_in_bounds(argc, index, 1);
            from = check_positive(atoi(argv[index + 1]));
            a_from = true;
            index+=2;
        }
        else if (strcmp(argv[index], "-len") == 0) {
            check_in_bounds(argc, index, 1);
            len = check_positive(atoi(argv[index + 1]));
            a_len = true;
            index+=2;
        }
        else {
            affirm(false, "Invalid input argument");
        }
    }

    affirm(a_file, "Missing file");
    affirm(a_from, "Missing from");
    affirm(a_len, "Missing length");

    // opening the file
    FILE *f = fopen(file_name, "r");
    affirm(f != NULL, "File is null pointer");
    size_t file_size = ftell(file);

    //////////////////////////////////////////////

    // Run parsing
    SorParser parser{f, (size_t)from, (size_t)from + len, file_size};
    parser.guessSchema();
    parser.parseFile();
    DataFrame* d = new DataFrame(parser.getColumnSet(), parser._num_columns);

    ////////////////////////////////////////////

    std::cout << d->nrow << std::endl;
    std::cout << d->ncol << std::endl;

    delete reader;
    fclose(f);
    return 0;
}
