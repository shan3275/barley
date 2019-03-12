#include <stdlib.h>
#include <string.h>

#include "frc_types.h"
#include "frctweak_arg_parser.h"

int frctweak_arg_parser_init(arg_parser_t *parser)
{
   if (NULL == parser)
   {
      return FRE_PARAM;
   }

   memset(parser, 0, sizeof(arg_parser_t));

   return FRE_SUCCESS;
}

int frctweak_arg_parser_add(arg_parser_t *parser, char *key, void (*func), void * var)
{
   arg_parser_entry_t *entry = NULL;

   if ((parser == NULL) || (NULL == key) || (NULL == var))
   {
      return FRE_PARAM;
   }

   if (strlen(key) > ARG_PARSER_KEY_MAX)
   {
      return FRE_PARAM;
   }

   entry = malloc(sizeof(arg_parser_entry_t));

   if (NULL == entry)
   {
      return FRE_MEMORY;
   }

   memset(entry, 0, sizeof(arg_parser_entry_t));

   entry->key  = strdup(key);
   entry->func =(int *) func;
   entry->var  = var;
   entry->next = NULL;

   if (NULL == parser->head)
   {
      parser->head = entry;
   }
   else
   {
       parser->tail->next = entry;
   }
   parser->tail = entry;
   

   return FRE_SUCCESS;
}

arg_parser_entry_t *
frctweak_arg_parser_entry_lookup(arg_parser_t *parser, char *key)
{
   arg_parser_entry_t *entry = NULL;

   if (NULL == parser)
   {
      printf("%s.%d\n", __func__, __LINE__);
      return NULL;
   }

   entry = parser->head;
    do 
  //for (entry = parser->head; entry->next != NULL; entry = entry->next)
   {
      if (!strcasecmp(key, entry->key))
      {
         return entry;
      }
      entry = entry->next;
   }while(entry);

   return NULL;
}


int frctweak_arg_parsing(arg_parser_t *parser, int argc, char **argv)
{
   int i;
   int rv;
   char *vp = NULL;
   char *ep = NULL;
   char key[ARG_PARSER_KEY_MAX];

   arg_parser_entry_t *entry = NULL;

   for (i = 0; i < argc; i++)
   {
      int key_sz = 0;
      printf("argv[%d] = %s\n", i, argv[i]);
      ep = strchr(argv[i], '=');

      if (NULL != ep)
      {
         vp = ep + 1;
         if (strlen(vp) <= 0)
         {
            return FRE_FAIL;
         }
         printf("vp = %s\n", vp);

         memset(key, 0, ARG_PARSER_KEY_MAX);

         key_sz = ep - argv[i];

         if (key_sz > ARG_PARSER_KEY_MAX)
         {
            return FRE_NOTFOUND;
         }

         memcpy(key, argv[i], key_sz);
         printf("key = %s\n", key);

         entry = frctweak_arg_parser_entry_lookup(parser, key);

         if (NULL == entry)
         {
            return FRE_NOTFOUND;
         }
         
         rv = entry->func(vp, entry->var);

         if (FRE_SUCCESS != rv)
         {
            return rv;
         }
         printf("%s.%d\n", __func__, __LINE__);
      }
   }

   return FRE_SUCCESS;
}

void
frctweak_arg_parser_free(arg_parser_t *parser)
{
   arg_parser_entry_t *entry = NULL, *next = NULL;

   if (NULL == parser)
   {
      return ;
   }

   entry = parser->head;
   while (entry)
   {
      next = entry->next;

      if (entry->key)
      {
         free(entry->key);
      }

      free(entry);

      entry = next;
   }

   //free(parser);
}

/* End of file */
