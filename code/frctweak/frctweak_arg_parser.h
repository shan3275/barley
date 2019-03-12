#ifndef __FRCTWEAK_ARG_PARSER_H__
#define __FRCTWEAK_ARG_PARSER_H__

typedef struct arg_parser_entry_s
{
    char *key;
    void *var;
    int (*func)(char *vp, void *var);
    struct arg_parser_entry_s *next;
} arg_parser_entry_t;
 
typedef struct 
{
    int entry_number;
    arg_parser_entry_t *head;
    arg_parser_entry_t *tail;
} arg_parser_t;

#define ARG_PARSER_KEY_MAX  32

int frctweak_arg_parser_init(arg_parser_t *parser);
int frctweak_arg_parser_add(arg_parser_t *parser, char *key, void (*func), void * var);
int frctweak_arg_parsing(arg_parser_t *parser, int argc, char **argv);
void frctweak_arg_parser_free(arg_parser_t *parser);
arg_parser_entry_t  *frctweak_arg_parser_entry_lookup(arg_parser_t *parser, char *key);

#endif /* !__FRCTWEAK_ARG_PARSER_H__ */
