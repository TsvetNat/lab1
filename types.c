#include "types.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

const char *type_names[] = {"NULL", "INT", "DOUBLE", "STRING"};

int get_type(const char *value, int scientific) {
    // Убираем пробелы и \r в начале
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
        value++;
    }
    
    if (value[0] == '\0') return TYPE_NULL;
    
    const char *p = value;
    if (*p == '+' || *p == '-') p++;
    if (*p == '\0') return TYPE_STRING;
    
    // Проверяем INT
    int has_digit = 0;
    const char *q = p;
    while (*q >= '0' && *q <= '9') {
        has_digit = 1;
        q++;
    }
    // Убираем пробелы в конце
    while (*q == ' ' || *q == '\t' || *q == '\r') q++;
    
    if (*q == '\0' && has_digit) {
        errno = 0;
        strtoll(value, NULL, 10);
        if (errno == 0) return TYPE_INT;
    }
    
    // Проверяем DOUBLE
    if (scientific) {
        char *endptr;
        double val = strtod(value, &endptr);
        while (*endptr == ' ' || *endptr == '\t' || *endptr == '\r') endptr++;
        if (*endptr == '\0' && endptr != value && isfinite(val)) {
            return TYPE_DOUBLE;
        }
    } else {
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
            while (*q == ' ' || *q == '\t' || *q == '\r') q++;
            
            if (*q == '\0' && digits_before > 0 && digits_after > 0) {
                double val = strtod(value, NULL);
                if (isfinite(val)) return TYPE_DOUBLE;
            }
        }
    }
    
    return TYPE_STRING;
}