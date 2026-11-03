#include "csv.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) end--;
    *(end + 1) = '\0';
    return s;
}

int parse_line(const char *line, char **fields, int max_fields, char delimiter) {
    int count = 0;
    const char *p = line;
    
    if (*p == '\0') {
        fields[0] = strdup("");
        return 1;
    }
    
    while (*p && count < max_fields) {
        while (*p == ' ' || *p == '\t') p++;
        
        if (*p == '"') {
            p++;
            int pos = 0;
            char *buf = malloc(strlen(line) + 1);
            while (*p) {
                if (*p == '"') {
                    if (*(p + 1) == '"') {
                        buf[pos++] = '"';
                        p += 2;
                    } else {
                        p++;
                        break;
                    }
                } else {
                    buf[pos++] = *p++;
                }
            }
            buf[pos] = '\0';
            while (*p == ' ' || *p == '\t') p++;
            fields[count++] = buf;
        } else {
            int pos = 0;
            char *buf = malloc(strlen(line) + 1);
            while (*p && *p != delimiter && *p != '\n' && *p != '\r') {
                buf[pos++] = *p++;
            }
            buf[pos] = '\0';
            char *trimmed = trim(buf);
            if (trimmed != buf) {
                memmove(buf, trimmed, strlen(trimmed) + 1);
            }
            fields[count++] = buf;
        }
        
        if (*p == delimiter) {
            p++;
        } else if (*p == '\0' || *p == '\n' || *p == '\r') {
            break;
        }
    }
    
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    
    if (len > 0 && line[len - 1] == delimiter) {
        fields[count++] = strdup("");
    }
    
    return count;
}

void csv_init(CSVParser *parser, char delimiter, int scientific) {
    parser->columns = NULL;
    parser->col_count = 0;
    parser->row_count = 0;
    parser->delimiter = delimiter;
    parser->scientific = scientific;
}

int csv_parse_file(CSVParser *parser, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Ошибка: не удалось открыть файл\n");
        return 0;
    }
    
    char *buffer = malloc(MAX_LINE);
    if (!buffer) {
        fclose(file);
        return 0;
    }
    
    char **fields = malloc(MAX_FIELDS * sizeof(char *));
    if (!fields) {
        free(buffer);
        fclose(file);
        return 0;
    }
    
    int is_header = 1;
    int expected_cols = 0;
    int lines_read = 0;
    
    while (fgets(buffer, MAX_LINE, file)) {
        lines_read++;
        
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        
        if (len == 0 && feof(file)) break;
        
        int field_count = parse_line(buffer, fields, MAX_FIELDS, parser->delimiter);
        
        if (is_header) {
            expected_cols = field_count;
            parser->col_count = expected_cols;
            
            parser->columns = malloc(expected_cols * sizeof(ColumnStats));
            if (!parser->columns) {
                free(fields);
                free(buffer);
                fclose(file);
                return 0;
            }
            
            for (int i = 0; i < expected_cols; i++) {
                stats_init(&parser->columns[i]);
                parser->columns[i].name = strdup(fields[i]);
            }
            is_header = 0;
        } else {
            if (field_count != expected_cols) {
                fprintf(stderr, "Ошибка: неверное количество полей в строке %d\n", parser->row_count + 1);
                free(fields);
                free(buffer);
                fclose(file);
                return 0;
            }
            
            for (int i = 0; i < expected_cols; i++) {
                int type = get_type(fields[i], parser->scientific);
                stats_update(&parser->columns[i], fields[i], type);
            }
            parser->row_count++;
        }
        
        for (int i = 0; i < field_count; i++) {
            if (fields[i]) free(fields[i]);
        }
    }
    
    if (lines_read == 0) {
        fprintf(stderr, "Ошибка: пустой файл\n");
        free(fields);
        free(buffer);
        fclose(file);
        return 0;
    }
    
    free(fields);
    free(buffer);
    fclose(file);
    return 1;
}

void csv_free(CSVParser *parser) {
    if (parser->columns) {
        for (int i = 0; i < parser->col_count; i++) {
            stats_free(&parser->columns[i]);
        }
        free(parser->columns);
        parser->columns = NULL;
    }
}