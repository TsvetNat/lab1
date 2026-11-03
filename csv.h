#ifndef CSV_H
#define CSV_H

#include "stats.h"

#define MAX_LINE 1048576
#define MAX_FIELDS 100

typedef struct {
    ColumnStats *columns;
    int col_count;
    int row_count;
    char delimiter;
    int scientific;
} CSVParser;

void csv_init(CSVParser *parser, char delimiter, int scientific);
int csv_parse_file(CSVParser *parser, const char *filename);
void csv_free(CSVParser *parser);

#endif