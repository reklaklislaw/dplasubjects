/* Takes in JSON metadata, pulls all entries of a field, and writes them to a file as JSON.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"
#include "findfield.h"

int main(int argc, char *argv[])
{
  if (argc!=7)
    {
      printf("Usage: in_file start_byte end_byte search_field id_field out_file\n");
      return 1;
    }
  
  char *in, *out;
  int in_len = strlen(argv[1]), out_len = strlen(argv[6]);
  
  in = (char *) malloc((in_len + 1) * sizeof(char));
  if (!in)
    {
      printf("failed to alloc 'in'\n");
      exit(1);
    }
  out = (char *) malloc((out_len + 1) * sizeof(char));
  if (!out) 
    {
      printf("failed to alloc 'out'\n");
      exit(1);
    }

  strcpy(in, argv[1]);
  strcpy(out, argv[6]);
  
  int start_byte, end_byte;
  start_byte = atoi(argv[2]);
  end_byte = atoi(argv[3]);

  char *search_field, *id_field;
  int sf_len = strlen(argv[4]), idf_len = strlen(argv[5]);
  search_field = (char *) malloc((sf_len + 1)  * sizeof(char));
  if (!search_field)
    {
      printf("failed to alloc 'search_field'\n");
      exit(1);
    }
  id_field = (char *) malloc((idf_len + 1)  * sizeof(char));
  if (!id_field)
    {
      printf("failed to alloc 'id_field'\n");
      exit(1);
    }

  strcpy(search_field, argv[4]);
  strcpy(id_field, argv[5]);

  FILE *in_file = fopen(in, "r");
  if (!in_file)
    {
      printf("failed to open in_file\n");
      return 1;
    }
  
  //get chunk here


  FILE *out_file = fopen(out, "w");
  if (!out_file)
    {
      printf("failed to open out_file\n");
      return 1;
    }

  find_field(in_file, search_field, id_field, out_file);
  
  free(in);
  free(out);
  free(search_field);
  free(id_field);

  fclose(in_file);
  fclose(out_file);

  return 0;
}


struct search_patterns {
  char *search_string_pattern;
  char *search_list_pattern;
  char *search_dict_pattern;
  char *id_pattern;
};


struct search_patterns get_search_patterns(char *search_field, char *id_field)
{
  struct search_patterns p;

  p.search_string_pattern = (char *) malloc((5 + strlen(search_field)) * sizeof(char));
  if (!p.search_string_pattern)
    {
      printf("failed to alloc 'search_string_pattern'\n");
      exit(1);
    }
  strcpy(p.search_string_pattern, "\"");
  strcat(p.search_string_pattern, search_field);
  strcat(p.search_string_pattern, "\":\"");

  p.search_list_pattern = (char *) malloc((5 + strlen(search_field)) * sizeof(char));
  if (!p.search_list_pattern)
    {
      printf("failed to alloc 'search_list_pattern'\n");
      exit(1);
    }
  strcpy(p.search_list_pattern, "\"");
  strcat(p.search_list_pattern, search_field);
  strcat(p.search_list_pattern, "\":[");

  p.search_dict_pattern = (char *) malloc((5 + strlen(search_field)) * sizeof(char));
  if (!p.search_dict_pattern)
    {
      printf("failed to alloc 'search_dict_pattern'\n");
      exit(1);
    }
  strcpy(p.search_dict_pattern, "\"");
  strcat(p.search_dict_pattern, search_field);
  strcat(p.search_dict_pattern, "\":{");

  p.id_pattern = (char *) malloc((5 + strlen(id_field)) * sizeof(char));
  if (!p.id_pattern)
    {
      printf("failed to alloc 'id_pattern'\n");
      exit(1);
    }
  strcpy(p.id_pattern, "\"");
  strcat(p.id_pattern, id_field);
  strcat(p.id_pattern, "\":\"");

  return p;
}


void find_field(FILE *in_file, char *search_field, char *id_field, FILE *out_file)
{
  int count = 0;
  int buf_size = 1000;
  char *buf = malloc(sizeof(char)*buf_size);
  if (!buf)
    {
      printf("Failed to malloc buf\n");
      exit(1);
    }
  char c;

  struct search_patterns p = get_search_patterns(search_field, id_field);

  int id_c, s_str_c, s_lst_c, s_dct_c;
  int total_s_str_c, total_s_lst_c, total_s_dct_c;
  total_s_str_c = total_s_lst_c = total_s_dct_c = 0;
  int *id_m, *s_str_m, *s_lst_m, *s_dct_m;
  
  fprintf(out_file, "%s", "[");

  c = getc(in_file);
  while(c != EOF)
    {
      
      //get next line
      while(c != '\n' && c != EOF)
	{
	  c = getc(in_file);
	  buf[count] = c;
	  count++;
	  if (count == buf_size)
	    {
	      buf = realloc(buf, sizeof(char)*(buf_size+1000));
	      if (!buf)
		{
		  printf("failed to increase buffer\n");
		}
	      buf_size += 1000;
	    }
	}
      buf[count] = '\0';

      get_match_positions(buf, count, p.id_pattern, &id_m, &id_c);
      get_match_positions(buf, count, p.search_string_pattern, &s_str_m, &s_str_c);
      get_match_positions(buf, count, p.search_list_pattern, &s_lst_m,  &s_lst_c);
      get_match_positions(buf, count, p.search_dict_pattern, &s_dct_m, &s_dct_c);

      char **id_matches, **s_str_matches, **s_lst_matches, **s_dct_matches;
      int *id_depth, *s_depth;
      
      int i, j=0, t = s_str_c +s_lst_c +s_dct_c;
      char e;
      if (s_str_c > 0 || s_lst_c > 0 || s_dct_c > 0)
	{
	  id_depth = (int *) malloc(sizeof(int) * id_c);
	  id_matches = get_matches(buf, count, id_m, id_c, p.id_pattern, &id_depth);
	  for (i=0; i<id_c; i++) 
	    {
	      if (id_depth[i] == 1)
		fprintf(out_file, "{\"identifier\":{%s}", id_matches[i]);
	      free(id_matches[i]);
	    }
	  
	  free(id_depth);
	  free(id_matches);
	  free(id_m);

	  fprintf(out_file, "%s", ",\"fields\":[");
 	  
	  if (s_str_c > 0) 
	    {
	      s_depth = (int *) malloc(sizeof(int) * s_str_c);
	      s_str_matches = get_matches(buf, count, s_str_m, s_str_c, 
					  p.search_string_pattern, &s_depth);
	      for (i=0; i<s_str_c; i++, j++) 
		{
		  if (j<t-1)
		    e = ',';
		  else
		    e = ']';
		  fprintf(out_file, "{%s}%c", s_str_matches[i], e);
		  free(s_str_matches[i]);
		}

	      free(s_str_matches);
	      free(s_depth);
	      free(s_str_m);
	    }

	  if (s_lst_c > 0)
	    {
	      s_depth = (int *) malloc(sizeof(int) * s_lst_c);
	      s_lst_matches = get_matches(buf, count, s_lst_m, s_lst_c, 
					  p.search_list_pattern, &s_depth);
	      for (i=0; i<s_lst_c; i++, j++) 
		{
		  if (j<t-1)
		    e = ',';
		  else
		    e = ']';
		  fprintf(out_file, "{%s}%c", s_lst_matches[i], e);
		  free(s_lst_matches[i]);
		}
	      
	      free(s_lst_matches);
	      free(s_depth);
	      free(s_lst_m);
	    }
	  
	  if (s_dct_c > 0)
	    {
	      s_depth = (int *) malloc(sizeof(int) * s_dct_c);
	      s_dct_matches = get_matches(buf, count, s_dct_m, s_dct_c, 
					  p.search_dict_pattern, &s_depth);
	      for (i=0; i<s_dct_c; i++, j++) 
		{
		  if (j<t-1)
		    e = ',';
		  else
		    e = ']';
		  fprintf(out_file, "{%s}%c", s_dct_matches[i], e);
		  free(s_dct_matches[i]);
		}

	      free(s_dct_matches);
	      free(s_depth);
	      free(s_dct_m);
	    }
	  
	  fprintf(out_file, "%s", "},\n");
	}

      total_s_str_c += s_str_c;
      total_s_lst_c += s_lst_c;
      total_s_dct_c += s_dct_c;

      c = getc(in_file);
      count = 0;
      id_c = s_str_c = s_lst_c = s_dct_c = 0;

      buf_size=1000;
      buf = realloc(buf, sizeof(char)*buf_size);
      if (!buf)
	{
	  printf("failed to realloc buf\n");
	  exit(1);
	}
      
    }

  fseek(out_file, -2, SEEK_END);
  fprintf(out_file, "%s", "]");

  printf("\nstrings:%d lists:%d dicts:%d\n", total_s_str_c, total_s_lst_c, total_s_dct_c);

  free(buf);
  free(p.id_pattern);
  free(p.search_string_pattern);
  free(p.search_list_pattern);
  free(p.search_dict_pattern);
  
}


char** get_matches(char *string, int stringlen, int *m_pos, int m_count, char *pattern, int **depth)
{
  int i, j, k, p;
  char open_char, close_char;	  
  int open, close, in_quotes;
  int subjbufsize = 1000;
  char *subjbuf = malloc(sizeof(char) * subjbufsize);
  if (!subjbuf)
    {
      printf("Failed to malloc subjbuf\n");
      exit(1);
    }

  char **matches = (char **) malloc(sizeof(int *) * m_count);
  if (!matches)
    {
      printf("Failed to malloc matches\n");
      exit(1);
    }

  for (j=0; j<m_count; j++)
    {

      (*depth)[j] = 0;

      in_quotes = 0;
      open = close = 0;
      p = m_pos[j];
      open_char = string[p+(strlen(pattern)-1)];
      if (open_char=='[')
	close_char = ']';
      else if (open_char=='{')
	close_char = '}';
      else if (open_char=='"')
	close_char = '"';
      
      for (i=0; i<stringlen; i++) {
	if (string[i] == '{') 
	  (*depth)[j]+=1;
	else if (string[i] == '}')
	  (*depth)[j]-=1;
	if (i == p) 
	  break;
      }
   
      for (i=p, k=0; i<stringlen; i++, k++)
	{
	  if (k == subjbufsize-1)
	    {
	      subjbuf = realloc(subjbuf, sizeof(char) * (subjbufsize+1000));
	      if (!subjbuf)
		{
		  printf("failed to allocate for subjbuf\n");
		}
	      subjbufsize+=1000;
	    }
 
	  if (k>=strlen(pattern)-1) 
	    {
	      if (string[i] == '"' && 
		  (string[i-1] != '\\' || (string[i-1] == '\\' && 
					   string[i-2] == '\\')))
		{
		  if (in_quotes)
		    in_quotes = 0;
		  else
		    in_quotes = 1;
		}
		
	      if (!in_quotes || open_char=='"')
		{
		  if (open_char=='"' && 
		      (string[i-1] != '\\' || (string[i-1] == '\\' && 
					       string[i-2] == '\\')))
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
		      matches[j] = malloc(sizeof(char) * (k+2));
		      if (!matches[j])
			{
			  printf("Failed to malloc matches[j]\n");
			  exit(1);
			}
		      strncpy(matches[j], subjbuf, k+1);		      
		      matches[j][k+1] = '\0';
		      subjbuf = realloc(subjbuf, sizeof(char) * 1000);
		      if (!subjbuf)
			{
			  printf("failed to allocate for subjbuf\n");
			  exit(1);
			}
		      subjbufsize = 1000;
		      open = close = 0;
		      in_quotes = 0;
		      break;
		    } 
		}
	    }

	  subjbuf[k] = string[i];
	}            
      
    }

  free(subjbuf); 
  return matches;
}




void get_match_positions(char *string, int stringlen, char *pattern,
			 int **m_pos, int *m_count)
{

  *m_count = 0;
  int m_size = 10;
  *m_pos = (int *) malloc(sizeof(int) * m_size);
  if (!*m_pos)
    {
      printf("malloc failed for m_pos\n");
      exit(1);
    }

  int pos = -1;
  int start = 0;
  while (1)    
    {
       
      pos = boyer_moore(string, start, stringlen, pattern, strlen(pattern));
      
      if (pos == -1)
	break;
      
      start = pos + strlen(pattern);

      (*m_pos)[*m_count] = pos;
      *m_count+=1;
      if (*m_count == m_size) 
	{
	  *m_pos = realloc(*m_pos, sizeof(int) * (*m_count + m_size));
	  if (!*m_pos)
	    {
	      printf("failed to realloc m_pos\n");
	      exit(1);
	    }
	  m_size += *m_count;
	}
    }

}
