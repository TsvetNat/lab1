#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_NULL 0
#define TYPE_INT 1
#define TYPE_DOUBLE 2
#define TYPE_STRING 3

typedef struct{
    char *name;
    int type;
    int count;
    int nulls;

    long long int_sum;
    long long int_max;
    long long int_min;
    int int_positive;
    int int_negative;
    int int_even;
    int int_count;

    double double_sum;
    double double_max;
    double double_min;
    int double_positive;
    int double_negative;
    int double_integer;
    int double_count;
}Column;

Column *columns=NULL;
int col_count=0;
int row_count=0;

int get_type(const char *value){
    if (value[0]=='\0') return TYPE_NULL;
    const char *p=value;
    if (*p=='+' || *p=='-')p++;
    if (*p=='\0') return TYPE_STRING;
    
    int has_digits=0;
    const char *q=p;
    while (*q>='0' && *q<='9'){
        has_digits=1;
        q++;
    }
    if (*q=='\0' && has_digits) return TYPE_INT;
    
    q=p;
    int digits_before=0;
    while (*q>='0' && *q<='9'){
        digits_before++;
        q++;
    }
    
    if (*q=='.'){
        q++;
        int digits_after=0;
        while (*q>='0' && *q<='9'){
            digits_after++;
            q++;
        }
        if (*q=='\0' && digits_before>0 && digits_after>0) return TYPE_DOUBLE;
    }
    return TYPE_STRING;
}

char **parse_line(char *line, int *out_count){
    int count=0;
    int capacity=16;
    char **fields=(char **)malloc(capacity*sizeof(char *));
    char *p=line;
    
    while (*p!='\0'){
        if (count>=capacity){
            capacity*=2;
            fields=(char **)realloc(fields, capacity*sizeof(char *));
        }
        
        char *start=p;
        while (*p && *p!=',')p++;
        
        size_t len=p-start;
        char *field=(char *)malloc(len + 1);
        memcpy(field, start, len);
        field[len]='\0';
        
        fields[count++]=field;
        if (*p==',')p++;
    }
    
    *out_count=count;
    return fields;
}

void free_fields(char **fields, int count) {
    for (int i=0; i<count; i++) free(fields[i]);
    free(fields);
}

void update_stats(int col_idx, const char *value) {
    Column *col=&columns[col_idx];
    int type=get_type(value);
    printf("DEBUG: value=[%s], type=%d\n", value, type);
    
    if (type==TYPE_NULL){
        col->nulls++;
        return;
    }
    if (col->type<type){
        if (col->type==TYPE_INT && type==TYPE_DOUBLE){
            col->double_sum=(double)col->int_sum;
            col->double_min=(double)col->int_min;
            col->double_max=(double)col->int_max;
            col->double_positive=col->int_positive;
            col->double_negative=col->int_negative;
            col->double_count=col->int_count;
            col->double_integer=col->int_count;
        }
        col->type=type;
    }
    if (type>col->type)col->type=type;
    col->count++;
    
    if (col->type==TYPE_INT){
        col->int_count++;
        long long val=atoll(value);
        col->int_sum+=val;
        if (col->int_count==1){
            col->int_min=val;
            col->int_max=val;
        }
        else{
            if (val<col->int_min) col->int_min=val;
            if (val>col->int_max) col->int_max=val;
        }
        if (val>0) col->int_positive++;
        else if (val<0) col->int_negative++;
        if (val%2==0) col->int_even++;
    }else if (col->type==TYPE_DOUBLE){
        col->double_count++;
        double val=atof(value);
        if (val==0.0) val=0.0;
        col->double_sum+=val;
        if (col->double_count==1){
            col->double_min=val;
            col->double_max=val;
        }
        else{
            if (val<col->double_min)col->double_min=val;
            if (val>col->double_max)col->double_max=val;
        }
        if (val>0)col->double_positive++;
        else if (val<0)col->double_negative++;
        if (val==(long long)val)col->double_integer++;
    }
}

char *read_line(FILE *f){
    int cap=256;
    char *buf=(char *)malloc(cap);
    int pos=0;
    int c;
    while ((c=fgetc(f))!=EOF && c!='\n'){
        if (pos+1>=cap){
            cap *= 2;
            buf = (char *)realloc(buf, cap);
        }
        buf[pos++]=(char)c;
    }
    if (pos==0 && c==EOF){
        free(buf);
        return NULL;
    }
    buf[pos]='\0';
    return buf;
}

int main(int argc, char *argv[]){
    if (argc>=2 && strcmp(argv[1], "--help")==0){
        if (argc>2) {
            fprintf(stderr, "Ошибка: лишние аргументы\n");
            return 2;
        }
        printf("Анализатор csv-таблиц\n csvstat --help\n csvstat schema <file.csv>\n csvstat stats <file.csv>\n");
        return 0;
    }
    if (argc!=3){
        fprintf(stderr, "Невозможно использование: csvstat <schema|stats> <file.csv>\n");
        return 2;
    }
    
    char *command=argv[1];
    char *filename=argv[2];

    if (strcmp(command, "schema")!=0 && strcmp(command, "stats")!=0){
        fprintf(stderr, "Ошибка, неизвестная команда\n");
        return 2;
    }
    FILE *file=fopen(filename, "r");
    if (!file){
        fprintf(stderr, "Ошибка: не удалось открыть файл\n");
        return 1;
    }
    
    char *header=read_line(file);
    if (!header){
        fprintf(stderr, "Ошибка-пустой файл\n");
        fclose(file);
        return 1;
    }
    
    int n_fields=0;
    char **fields=parse_line(header, &n_fields);
    
    col_count=n_fields;
    columns=(Column *)calloc(col_count, sizeof(Column));
    for (int i=0; i<col_count; i++){
        columns[i].name=strdup(fields[i]);
        columns[i].type=TYPE_NULL;
    }
    
    free_fields(fields, n_fields);
    free(header);
    
    char *line;
    while ((line=read_line(file))!=NULL){
        int nf = 0;
        char **lf=parse_line(line, &nf);
        
        if (nf==col_count) {
            for (int i=0; i<col_count; i++) update_stats(i, lf[i]);
            row_count++;
        }
        
        free_fields(lf, nf);
        free(line);
    }
    
    if (strcmp(command, "schema")==0){
        printf("rows %d\n", row_count);
        printf("columns %d\n", col_count);
        for (int i=0; i<col_count; i++){
            const char *tn=(columns[i].type==TYPE_INT) ? "INT": (columns[i].type == TYPE_DOUBLE) ? "DOUBLE": "NULL";
            printf("%d \"%s\" %s\n", i, columns[i].name, tn);
        }
    }else if (strcmp(command, "stats")==0){
        printf("rows %d\n", row_count);
        printf("columns %d\n", col_count);
        for (int i=0; i<col_count; i++){
            printf("\ncolumn %d\n", i);
            printf("name \"%s\"\n", columns[i].name);
            printf("type %s\n", (columns[i].type == TYPE_INT) ? "INT": (columns[i].type == TYPE_DOUBLE) ? "DOUBLE" :"NULL");
            printf("nulls %d\n", columns[i].nulls);
            
            if (columns[i].type==TYPE_INT && columns[i].count>0){
                printf("min %lld\n", columns[i].int_min);
                printf("max %lld\n", columns[i].int_max);
                printf("mean %.6f\n", (double)columns[i].int_sum/columns[i].count);
                printf("positive %d\n", columns[i].int_positive);
                printf("negative %d\n", columns[i].int_negative);
                printf("even %d\n", columns[i].int_even);
            }else if (columns[i].type==TYPE_DOUBLE && columns[i].count>0){
                printf("min %.6f\n", columns[i].double_min);
                printf("max %.6f\n", columns[i].double_max);
                printf("mean %.6f\n", columns[i].double_sum / columns[i].count);
                printf("positive %d\n", columns[i].double_positive);
                printf("negative %d\n", columns[i].double_negative);
                printf("integer %d\n", columns[i].double_integer);
            }
        }
    }
    
    for (int i=0; i<col_count; i++)free(columns[i].name);
    free(columns);
    fclose(file);
    return 0;
}