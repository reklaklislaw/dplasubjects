#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "boyermoore.c"
#include "filesearch.h"


void *add_search_pattern(struct search_patterns *sp, 
			 char *prefix, char *field, 
			 char *suffix, char *type)
{
  if (!sp)
    {
      fprintf(stderr, "received uninitialized struct search_pattern\n");
      return NULL;
    }

  if (!field || !type)
    {
      fprintf(stderr, "invalid arguments\n");
      return NULL;
    }

  if (!sp->pcount) 
    {
      sp->pcount = 0;
      sp->patterns = NULL;
    }

  sp->patterns = realloc(sp->patterns, sizeof(struct pattern) * (sp->pcount + 1));
  if (!sp->patterns)
    {
      fprintf(stderr, "failed to alloc search patterns\n");
      return NULL;
    } else sp->pcount += 1;

  int plen = 0, slen = 0;
  if (prefix)
    plen = strlen(prefix);
  if (suffix)
    slen = strlen(suffix);

  sp->patterns[sp->pcount-1].string = malloc(sizeof(char) * 
					     (plen + strlen(field) + 
					      slen + 1));
  if (!sp->patterns[sp->pcount-1].string)
    {
      fprintf(stderr, "failed to alloc pattern string\n");
      return NULL;
    }

  sp->patterns[sp->pcount-1].name = malloc(sizeof(char) * (strlen(field) + 1));
  if (!sp->patterns[sp->pcount-1].name)
    {
      fprintf(stderr, "failed to alloc pattern name\n");
      return NULL;
    }

  sp->patterns[sp->pcount-1].type = malloc(sizeof(char) * (strlen(type) + 1));
  if (!sp->patterns[sp->pcount-1].type)
    {
      fprintf(stderr, "failed to alloc pattern type\n");
      return NULL;
    }

  sp->patterns[sp->pcount-1].matches = NULL;
  sp->patterns[sp->pcount-1].total_match_count = 0;
  
  if (prefix) {
    strcpy(sp->patterns[sp->pcount-1].string, prefix);
    strcat(sp->patterns[sp->pcount-1].string, field);
  }
  else
    strcpy(sp->patterns[sp->pcount-1].string, field);
  if (suffix)
    strcat(sp->patterns[sp->pcount-1].string, suffix);
  strcpy(sp->patterns[sp->pcount-1].name, field);
  strcpy(sp->patterns[sp->pcount-1].type, type);
  
  return (int *) 1;
}



int string_to_list(char *string, char ***list)
{
  char prev, cur, next;
  prev = cur = next = '\0';
  int i, j, len, count = 1;
  len = strlen(string);

  //count items
  for (j=0; j<len; j++)
    {
      cur = string[j];
      if (j + 1 < len-1)
	next = string[j+1];
      else
	next = '\0';
      if (cur == ',' && prev == '"' && next == ' ')
	count += 1;
      prev = cur;
    }
  
  *list = malloc(sizeof(char *) * count);
  if (!list)
    {
      fprintf(stderr, "failed to alloc list for string to list conversion\n");
      return -1;
    }

  char *buf = malloc(sizeof(char) * len);
  if (!buf)
    {
      fprintf(stderr, "failed to alloc buf\n", buf);
      return -1;
    }

  prev = cur = next = '\0';
  int l = 0;
  for (i=0, j=1; j<len; i++, j++)
    {
      cur = string[j];
      if (j + 1 < len-1)
	next = string[j+1];
      else
	next = '\0';
      if ((cur == ',' && prev == '"' && next == ' ') || (cur==']' && prev == '"' && next=='\0'))
	{
	  buf[i] = '\0';
	  (*list)[l] = malloc(sizeof(char) * i+1);
	  if (!(*list)[l])
	    {
	      fprintf(stderr, "failed to alloc list entry\n");	
	      return -1;
	    }
	  strncpy((*list)[l], buf, i+1);
	  i = 0;
	  j += 2; //proceed to next item
	  prev = ' ';
	  l++;
	}
      else 
	{
	  buf[i] = string[j];
	  prev = cur;
	}
    }

  free(buf);
  return count;
}


void *get_matches(char *string, int stringlen, struct pattern *pattern)
{
  int i, j, k, p, s;
  char open_char, close_char;	  
  int open, close, in_quotes;
  int subjbufsize = 1000;
  char *subjbuf = malloc(sizeof(char) * subjbufsize);
  if (!subjbuf)
    {
      fprintf(stderr, "Failed to malloc subjbuf\n");
      exit(1);
    }
  
  for (j=0; j<pattern->match_count; j++)
    {
      pattern->matches[j].string = NULL;
      pattern->matches[j].depth = 0;
      
      in_quotes = 0;
      open = close = 0;
      p = pattern->matches[j].pos;
      s = (p + strlen(pattern->string)) - 1;
      
      open_char = string[ s ];
      if (open_char=='[')
	close_char = ']';
      else if (open_char=='{')
	close_char = '}';
      else if (open_char=='"')
	close_char = '"';
      
      for (i=0; i<stringlen; i++) {
	if (are_quotes(string, i))
	  in_quotes = in_quotes ? 0 : 1;
	if (string[i] == '{' && !in_quotes)
	  pattern->matches[j].depth += 1;
	else if (string[i] == '}' && !in_quotes) 
	  pattern->matches[j].depth -= 1;
	if (i == p) 	  
	  break;
      }

      in_quotes = 0;
      for (i=s, k=0; i<stringlen; i++, k++)
	{

	  if (k == subjbufsize-1)
	    {
	      subjbuf = realloc(subjbuf, sizeof(char) * (subjbufsize+1000));
	      if (!subjbuf)
		{
		  fprintf(stderr, "failed to allocate for subjbuf\n");
		}
	      subjbufsize+=1000;
	    }
	  
	  if (are_quotes(string, i))
	    in_quotes = (in_quotes) ? 0 : 1;
	  
	  if (!in_quotes || open_char=='"')
	    {
	      if (open_char=='"' && are_quotes(string, i))
		{
		  if (in_quotes && string[i] == close_char)
		    close++;
		  else if (!in_quotes && string[i] == open_char)
		    open++;
		}
	      else if (open_char!='"')
		{
		  if (string[i] == open_char)
		    open++;
		  else if (string[i] == close_char)
		    close++;
		}
	      
	      if(open == close) 
		{
		  subjbuf[k] = string[i];
		  pattern->matches[j].string = malloc(sizeof(char) * k + 2);
		  if (!pattern->matches[j].string)
		    {
		      fprintf(stderr, "Failed to malloc match string\n");
		      return NULL;
		    }     
		  strncpy(pattern->matches[j].string, subjbuf, k+1);		      
		  pattern->matches[j].string[k+1] = '\0';
		  subjbuf = realloc(subjbuf, sizeof(char) * 1000);
		  if (!subjbuf)
		    {
		      fprintf(stderr, "failed to allocate for subjbuf\n");
		      return NULL;
		    }
		  subjbufsize = 1000;
		  open = close = 0;
		  in_quotes = 0;
		  break;
		} 
	    }
	  

	  subjbuf[k] = string[i];
	}            
      
    }
  
  free(subjbuf); 

  return (int *) 1;
}
 
  
void *get_match_positions(char *string, int stringlen, struct pattern *pattern)
{
  
  if (!pattern->matches)
    {
      pattern->match_size = 10;
      pattern->match_count = 0;
      pattern->matches = malloc(sizeof(struct match) * pattern->match_size);
      if (!pattern->matches)
	{
	  fprintf(stderr, "failed to alloc pattern->matches\n");
	  return NULL;
	}	
    }
    
  int pos = -1;
  int start = 0;
  while (1)    
    {
      pattern->matches[pattern->match_count].string = NULL;
      pattern->matches[pattern->match_count].depth = 0;
      
      pos = boyer_moore(string, start, stringlen, pattern->string, strlen(pattern->string));

      if (pos == -1)
	break;
      
      start = pos + strlen(pattern->string);
      
      pattern->matches[pattern->match_count].pos = pos;
      pattern->match_count += 1;
      pattern->total_match_count += 1;

      if (pattern->match_count == pattern->match_size) 
	{
	  pattern->matches = realloc(pattern->matches, 
				     sizeof(struct match) * pattern->match_size << 1);
	  if (!pattern->matches)
	    {
	      fprintf(stderr, "failed to realloc pattern matches\n");
	      return NULL;
	    }
	  pattern->match_size <<= 1;
	}
    }
  
  return (int *) 1;
}


int are_quotes(char *string, int pos)
{
  if (string[pos] == '"' && 
      (string[pos-1] != '\\' || (string[pos-1] == '\\' && 
				 string[pos-2] == '\\')))
    return 1;
  else
    return 0;

}
