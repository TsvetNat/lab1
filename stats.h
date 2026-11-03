#ifndef STATS_H
#define STATS_H
#include "types.h"
typedef struct {
    int type;
    char *name;
    
    long long int_min, int_max, int_sum;
    int int_count, int_positive, int_negative, int_even;
    
    double double_min, double_max, double_sum;
    int double_count, double_positive, double_negative, double_integer;
    
    int str_min_len, str_max_len;
    long long str_sum_len;
    int str_count, str_empty;
    
    int nulls;
    int first_value;
} ColumnStats;

void stats_init(ColumnStats *stats);
void stats_update(ColumnStats *stats, const char *value, int type);
void stats_free(ColumnStats *stats);
void stats_print(ColumnStats *stats, int col_index);

#endif