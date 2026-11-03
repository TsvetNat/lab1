#include "stats.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

void stats_init(ColumnStats *stats) {
    stats->type = TYPE_NULL;
    stats->name = NULL;
    stats->nulls = 0;
    stats->first_value = 1;
    stats->int_count = stats->double_count = stats->str_count = 0;
    stats->int_positive = stats->int_negative = stats->int_even = 0;
    stats->double_positive = stats->double_negative = stats->double_integer = 0;
    stats->str_empty = 0;
}

void stats_update(ColumnStats *stats, const char *value, int type) {
    if (type > stats->type) stats->type = type;
    
    if (type == TYPE_NULL) {
        stats->nulls++;
        return;
    }
    
    if (type == TYPE_INT) {
        long long val = strtoll(value, NULL, 10);
        if (stats->first_value) {
            stats->int_min = stats->int_max = val;
            stats->first_value = 0;
        } else {
            if (val < stats->int_min) stats->int_min = val;
            if (val > stats->int_max) stats->int_max = val;
        }
        stats->int_sum += val;
        stats->int_count++;
        if (val > 0) stats->int_positive++;
        else if (val < 0) stats->int_negative++;
        if (val % 2 == 0) stats->int_even++;
        
    } else if (type == TYPE_DOUBLE) {
        double val = strtod(value, NULL);
        if (stats->first_value) {
            stats->double_min = stats->double_max = val;
            stats->first_value = 0;
        } else {
            if (val < stats->double_min) stats->double_min = val;
            if (val > stats->double_max) stats->double_max = val;
        }
        stats->double_sum += val;
        stats->double_count++;
        if (val > 0) stats->double_positive++;
        else if (val < 0) stats->double_negative++;
        if (val == floor(val)) stats->double_integer++;
        
    } else if (type == TYPE_STRING) {
        int len = strlen(value);
        if (stats->first_value) {
            stats->str_min_len = stats->str_max_len = len;
            stats->first_value = 0;
        } else {
            if (len < stats->str_min_len) stats->str_min_len = len;
            if (len > stats->str_max_len) stats->str_max_len = len;
        }
        stats->str_sum_len += len;
        stats->str_count++;
        if (len == 0) stats->str_empty++;
    }
}

void stats_free(ColumnStats *stats) {
    if (stats->name != NULL) {
        free(stats->name);
        stats->name = NULL;
    }
}

static void print_double(double val) {
    if (val == 0.0) val = 0.0;
    printf("%.6f", val);
}

void stats_print(ColumnStats *stats, int col_index) {
    printf("column %d\n", col_index);
    printf("name \"");
    if (stats->name) {
        for (const char *p = stats->name; *p; p++) {
            if (*p == '"') printf("\"\"");
            else putchar(*p);
        }
    }
    printf("\"\n");
    
    printf("type %s\n", type_names[stats->type]);
    printf("nulls %d\n", stats->nulls);
    
    if (stats->type == TYPE_INT) {
        if (stats->int_count > 0) {
            printf("min %lld\n", stats->int_min);
            printf("max %lld\n", stats->int_max);
            printf("mean ");
            print_double((double)stats->int_sum / stats->int_count);
            printf("\n");
        } else {
            printf("min 0\nmax 0\nmean 0.000000\n");
        }
        printf("positive %d\n", stats->int_positive);
        printf("negative %d\n", stats->int_negative);
        printf("even %d\n", stats->int_even);
        
    } else if (stats->type == TYPE_DOUBLE) {
        if (stats->double_count > 0) {
            printf("min ");
            print_double(stats->double_min);
            printf("\n");
            printf("max ");
            print_double(stats->double_max);
            printf("\n");
            printf("mean ");
            print_double(stats->double_sum / stats->double_count);
            printf("\n");
        } else {
            printf("min 0.000000\nmax 0.000000\nmean 0.000000\n");
        }
        printf("positive %d\n", stats->double_positive);
        printf("negative %d\n", stats->double_negative);
        printf("integer %d\n", stats->double_integer);
        
    } else if (stats->type == TYPE_STRING) {
        if (stats->str_count > 0) {
            printf("min_length %d\n", stats->str_min_len);
            printf("max_length %d\n", stats->str_max_len);
            printf("mean_length ");
            print_double((double)stats->str_sum_len / stats->str_count);
            printf("\n");
        } else {
            printf("min_length 0\nmax_length 0\nmean_length 0.000000\n");
        }
        printf("empty %d\n", stats->str_empty);
    }
}