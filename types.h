#ifndef TYPES_H
#define TYPES_H

#define TYPE_NULL 0
#define TYPE_INT 1
#define TYPE_DOUBLE 2
#define TYPE_STRING 3

extern const char *type_names[];

int get_type(const char *value, int scientific);
#endif