#include "post_parse.h"
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>
char* parse_post_request(const char *str_request, const char *str_regex)
{
regex_t preg;
	char* a=malloc(1024);
   int err;
	/* (1) */
   err = regcomp (&preg, str_regex, REG_EXTENDED);
   if (err == 0)
   {
      int match;
      size_t nmatch = 0;
      regmatch_t *pmatch = NULL;
      
      nmatch = preg.re_nsub;
      pmatch = malloc (sizeof (*pmatch) * nmatch);
      if (pmatch)
      {
/* (2) */
         match = regexec (&preg, str_request, nmatch, pmatch, 0);
/* (3) */
         regfree (&preg);
/* (4) */
         if (match == 0)
         {
            
            int start = pmatch[0].rm_so;
            int end = pmatch[0].rm_eo;
            size_t size = end - start;
               
            a = malloc (sizeof (*a) * (size + 1));
            if (a)
            {
               strncpy (a, &str_request[start], size);
               a[size] = '\0';
               printf ("%s\n", a);
               return a;
            }
         }
/* (5) */
         else if (match == REG_NOMATCH)
         {
            return NULL;
         }
/* (6) */
         else
         {
			
            char *text;
            size_t size;

/* (7) */
            size = regerror (err, &preg, NULL, 0);
            text = malloc (sizeof (*text) * size);
            if (text)
            {
/* (8) */
               regerror (err, &preg, text, size);
               fprintf (stderr, "%s\n", text);
               free (text);
            }
            else
            {
               fprintf (stderr, "Memoire insuffisante\n");
              return NULL;
            }
         }
      }
      else
      {
         fprintf (stderr, "Memoire insuffisante\n");
         return NULL;
      }
}
}
