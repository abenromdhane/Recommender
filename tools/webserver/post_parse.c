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

         match = regexec (&preg, str_request, nmatch, pmatch, 0);

         regfree (&preg);

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
               return a;
            }
         }
      }
   
      else
      {
         return NULL;
      }
}
}


coo_entry_t* get_rating_from_http(const char *str_request, const char *str_regex)
{

	char* header_fields = parse_post_request(str_request, str_regex);
	size_t r;
	if (header_fields==NULL)
	{
		return NULL;
	}
	coo_entry_t* rating= malloc(sizeof(coo_entry_t));		
	if(sscanf(header_fields,"user=%u&item=%u&rating=%d",&rating->row_i,&rating->column_j,&r)!=3)
	{
		free(rating);
		return NULL;
	}
	rating->value = (float) r;
	return rating;
}
