#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1000
#define MAX_COLS 100
#define MAX_ROWS 1000

#define TYPE_NULL 0
#define TYPE_INT 1
#define TYPE_DOUBLE 2
#define TYPE_STRING 3

typedef struct {
    char name[100];
    int type;
    int count;
    int nulls;
    
    long long int_sum;
    long long int_min;
    long long int_max;
    int int_positive;
    int int_negative;
    int int_even;
    
    double double_sum;
    double double_min;
    double double_max;
    int double_positive;
    int double_negative;
    int double_integer;
    
    int str_min_len;
    int str_max_len;
    long long str_sum_len;
    int str_empty;
} Column;

Column columns[MAX_COLS];
int col_count = 0;
int row_count = 0;

/*Функция для определения типа*/
int get_type(const char *value) {
    if (value[0] == '\0') return TYPE_NULL;
    
    const char *p = value;
    if (*p == '+' || *p == '-') p++;
    if (*p == '\0') return TYPE_STRING;
    
    /*INT*/
    int has_digits = 0;
    const char *q = p;
    while (*q >= '0' && *q <= '9') {
        has_digits = 1;
        q++;
    }
    if (*q == '\0' && has_digits) return TYPE_INT;
    
    /*DOUBLE*/
    q = p;
    int digits_before = 0;
    while (*q >= '0' && *q <= '9') {
        digits_before++;
        q++;
    }
    if (*q == '.') {
        q++;
        int digits_after = 0;
        while (*q >= '0' && *q <= '9') {
            digits_after++;
            q++;
        }
        if (*q == '\0' && digits_before > 0 && digits_after > 0) {
            return TYPE_DOUBLE;
        }
    }
    
    return TYPE_STRING;
}

/*разбиение строки*/
int parse_line(char *line, char fields[][100]) {
    int count = 0;
    char *p = line;
    
    while (*p && count < MAX_COLS) {
        /*Пропуск пробелов*/
        while (*p == ' ' || *p == '\t') p++;
        
        if (*p == '"') {
            /*В кавычках*/
            p++;
            int pos = 0;
            while (*p) {
                if (*p == '"') {
                    if (*(p + 1) == '"') {
                        fields[count][pos++] = '"';
                        p += 2;
                    } else {
                        p++;
                        break;
                    }
                } else {
                    fields[count][pos++] = *p++;
                }
            }
            fields[count][pos] = '\0';
            while (*p == ' ' || *p == '\t') p++;
        } else {
            /*Вне кавычек*/
            int pos = 0;
            while (*p && *p != ',') {
                fields[count][pos++] = *p++;
            }
            fields[count][pos] = '\0';
            
            /*Убираем пробелы*/
            char *start = fields[count];
            while (*start == ' ') start++;
            char *end = fields[count] + strlen(fields[count]) - 1;
            while (end > start && *end == ' ') {
                *end = '\0';
                end--;
            }
            if (start != fields[count]) {
                memmove(fields[count], start, strlen(start) + 1);
            }
        }
        
        count++;
        
        if (*p == ',') {
            p++;
        } else {
            break;
        }
    }
    
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == ',') {
        fields[count][0] = '\0';
        count++;
    }
    
    return count;
}

void init_column(Column *col) {
    col->type = TYPE_NULL;
    col->count = 0;
    col->nulls = 0;
    col->int_sum = 0;
    col->int_min = 0;
    col->int_max = 0;
    col->int_positive = 0;
    col->int_negative = 0;
    col->int_even = 0;
    col->double_sum = 0;
    col->double_min = 0;
    col->double_max = 0;
    col->double_positive = 0;
    col->double_negative = 0;
    col->double_integer = 0;
    col->str_min_len = 999999;
    col->str_max_len = 0;
    col->str_sum_len = 0;
    col->str_empty = 0;
}

void update_stats(int col_idx, const char *value) {
    Column *col = &columns[col_idx];
    int type = get_type(value);
    
    if (type == TYPE_NULL) {
        col->nulls++;
        return;
    }
    
    if (type > col->type) col->type = type;
    col->count++;
    
    if (type == TYPE_INT) {
        long long val = atoll(value);
        col->int_sum += val;
        if (col->count == 1) {
            col->int_min = val;
            col->int_max = val;
        } else {
            if (val < col->int_min) col->int_min = val;
            if (val > col->int_max) col->int_max = val;
        }
        if (val > 0) col->int_positive++;
        else if (val < 0) col->int_negative++;
        if (val % 2 == 0) col->int_even++;
    } else if (type == TYPE_DOUBLE) {
        double val = atof(value);
        col->double_sum += val;
        if (col->count == 1) {
            col->double_min = val;
            col->double_max = val;
        } else {
            if (val < col->double_min) col->double_min = val;
            if (val > col->double_max) col->double_max = val;
        }
        if (val > 0) col->double_positive++;
        else if (val < 0) col->double_negative++;
        if (val == (int)val) col->double_integer++;
    } else {
        int len = strlen(value);
        if (len < col->str_min_len) col->str_min_len = len;
        if (len > col->str_max_len) col->str_max_len = len;
        col->str_sum_len += len;
        if (len == 0) col->str_empty++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: csvstat <schema|stats> <file.csv>\n");
        return 1;
    }
    
    char *command = argv[1];
    char *filename = argv[2];
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Ошибка: не удалось открыть файл\n");
        return 1;
    }
    
    /*Динамическое выделение памяти*/
    char *buffer = (char *)malloc(MAX_LINE);
    char **fields = (char **)malloc(MAX_COLS * sizeof(char *));
    for (int i = 0; i < MAX_COLS; i++) {
        fields[i] = (char *)malloc(100);
    }
    
    if (!buffer || !fields) {
        printf("Ошибка выделения памяти\n");
        return 1;
    }
    
    char line[MAX_LINE];
    char temp_fields[MAX_COLS][100];
    
    /*Читаем заголовок*/
    if (fgets(buffer, MAX_LINE, file)) {
        buffer[strlen(buffer) - 1] = '\0';
        strcpy(line, buffer);
        
        col_count = parse_line(line, temp_fields);
        
        for (int i = 0; i < col_count; i++) {
            strcpy(columns[i].name, temp_fields[i]);
            init_column(&columns[i]);
        }
    }
    
    /*Чтение данных*/
    while (fgets(buffer, MAX_LINE, file)) {
        buffer[strlen(buffer) - 1] = '\0';
        strcpy(line, buffer);
        
        int field_count = parse_line(line, temp_fields);
        
        if (field_count == col_count) {
            for (int i = 0; i < col_count; i++) {
                update_stats(i, temp_fields[i]);
            }
            row_count++;
        }
    }
    
    /*Вывод*/
    if (strcmp(command, "schema") == 0) {
        printf("rows %d\n", row_count);
        printf("columns %d\n", col_count);
        
        for (int i = 0; i < col_count; i++) {
            char *type_name;
            if (columns[i].type == TYPE_INT) type_name = "INT";
            else if (columns[i].type == TYPE_DOUBLE) type_name = "DOUBLE";
            else if (columns[i].type == TYPE_STRING) type_name = "STRING";
            else type_name = "NULL";
            
            printf("%d \"%s\" %s\n", i, columns[i].name, type_name);
        }
    } else if (strcmp(command, "stats") == 0) {
        printf("rows %d\n", row_count);
        printf("columns %d\n", col_count);
        
        for (int i = 0; i < col_count; i++) {
            printf("\ncolumn %d\n", i);
            printf("name \"%s\"\n", columns[i].name);
            printf("type %s\n", 
                columns[i].type == TYPE_INT ? "INT" :
                columns[i].type == TYPE_DOUBLE ? "DOUBLE" :
                columns[i].type == TYPE_STRING ? "STRING" : "NULL");
            printf("nulls %d\n", columns[i].nulls);
            
            if (columns[i].type == TYPE_INT && columns[i].count > 0) {
                printf("min %lld\n", columns[i].int_min);
                printf("max %lld\n", columns[i].int_max);
                printf("mean %.6f\n", (double)columns[i].int_sum / columns[i].count);
                printf("positive %d\n", columns[i].int_positive);
                printf("negative %d\n", columns[i].int_negative);
                printf("even %d\n", columns[i].int_even);
            } else if (columns[i].type == TYPE_DOUBLE && columns[i].count > 0) {
                printf("min %.6f\n", columns[i].double_min);
                printf("max %.6f\n", columns[i].double_max);
                printf("mean %.6f\n", columns[i].double_sum / columns[i].count);
                printf("positive %d\n", columns[i].double_positive);
                printf("negative %d\n", columns[i].double_negative);
                printf("integer %d\n", columns[i].double_integer);
            } else if (columns[i].type == TYPE_STRING && columns[i].count > 0) {
                printf("min_length %d\n", columns[i].str_min_len);
                printf("max_length %d\n", columns[i].str_max_len);
                printf("mean_length %.6f\n", (double)columns[i].str_sum_len / columns[i].count);
                printf("empty %d\n", columns[i].str_empty);
            }
        }
    }
    
    free(buffer);
    for (int i = 0; i < MAX_COLS; i++) {
        free(fields[i]);
    }
    free(fields);
    fclose(file);
    
    return 0;
}