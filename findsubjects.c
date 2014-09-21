/* Takes in JSON metadata, pulls all "subject" entries, and writes them to a file as JSON.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"

void find_subjects(FILE *in_file, FILE *out_file);
char** get_matches(char *string, int stringlen, int *m_pos, int m_count);
void get_match_positions(char *string, int stringlen, char *pattern,
			 int **m_pos, int *m_count);

int main(int argc, char *argv[])
{
  if (argc!=3)
    {
      printf("Usage: in_file out_file\n");
      return 1;
    }

  char in[100], out[100];
  strcpy(in, argv[1]);
  strcpy(out, argv[2]);
  
  FILE *in_file = fopen(in, "r");
  if (in_file == NULL)
    {
      printf("failed to open in_file\n");
      return 1;
    }
  
  FILE *out_file = fopen(out, "w");
  if (out_file == NULL)
    {
      printf("failed to open out_file\n");
      return 1;
    }

  find_subjects(in_file, out_file);

  fclose(in_file);
  fclose(out_file);
  return 0;
}


void find_subjects(FILE *in_file, FILE *out_file)
{
  int count = 0;
  int buf_size = 1000;
  char *buf = malloc(sizeof(char)*buf_size);
  char c;

  char *string_pattern = "\"subject\":\"";
  char *list_pattern = "\"subject\":[";
  char *dict_pattern = "\"subject\":{";

  int str_c, lst_c, dct_c;
  int t_str_c, t_lst_c, t_dct_c;
  t_str_c = t_lst_c = t_dct_c = 0;
  int *str_m, *lst_m, *dct_m;
  
  fprintf(out_file, "%s", "[");

  c = getc(in_file);
  while(c != EOF)
    {
      while(c != '\n' && c != EOF)
	{
	  c = getc(in_file);
	  buf[count] = c;
	  count++;
	  if (count == buf_size)
	    {
	      buf = realloc(buf, sizeof(char)*(buf_size+1000));
	      if (buf == NULL)
		{
		  printf("failed to increase buffer\n");
		}
	      buf_size += 1000;
	    }
	}
          
      if (count == buf_size)
	{
	  buf = realloc(buf, sizeof(char)*(buf_size+1));
	  if (buf == NULL)
	    {
	      printf("failed to increase buffer\n");
	    }
	}

      buf[count] = '\0';

      get_match_positions(buf, count, string_pattern, &str_m, &str_c);
      get_match_positions(buf, count, list_pattern, &lst_m,  &lst_c);
      get_match_positions(buf, count, dict_pattern, &dct_m, &dct_c);

      char **str_matches, **lst_matches, **dct_matches;

      int i;
      if (str_c > 0) 
	{
	  str_matches = get_matches(buf, count, str_m, str_c);
	  for (i=0; i<str_c; i++) 
	    {
	      fprintf(out_file, "{%s},\n", str_matches[i]);
	    }
	}

      if (lst_c > 0)
	{
	  lst_matches = get_matches(buf, count, lst_m, lst_c);
	  for (i=0; i<lst_c; i++) 
	    {
	      fprintf(out_file, "{%s},\n",lst_matches[i]);
	    }
	}
      if (dct_c > 0)
	{
	  dct_matches = get_matches(buf, count, dct_m, dct_c);
	  for (i=0; i<dct_c; i++) 
	    {
	      fprintf(out_file, "{%s},\n", dct_matches[i]);
	    }
	}

      t_str_c += str_c;
      t_lst_c += lst_c;
      t_dct_c += dct_c;

      c = getc(in_file);
      count = 0;
      str_c = lst_c = dct_c = 0;
      //free(buf);
      buf_size=1000;
      buf = realloc(buf, sizeof(char)*buf_size);
      if (buf == NULL)
	{
	  printf("failed to realloc buf\n");
	  exit(1);
	}
    }

  fseek(out_file, -2, SEEK_END);
  fprintf(out_file, "%s", "]");

  printf("strings:%d lists:%d dicts:%d\n", t_str_c, t_lst_c, t_dct_c);

  free(buf);
}


char** get_matches(char *string, int stringlen, int *m_pos, int m_count)
{
  int i, j, k, p;
  char open_char, close_char;	  
  int open, close, in_quotes;
  int subjbufsize = 1000;
  char *subjbuf = malloc(sizeof(char) * subjbufsize);

  char **matches = malloc(sizeof(int) * m_count);

  for (j=0; j<m_count; j++)
    {
      in_quotes = 0;
      open = close = 0;
      p = m_pos[j];
      open_char = string[p+10];
      if (open_char=='[')
	close_char = ']';
      else if (open_char=='{')
	close_char = '}';
      else if (open_char=='"')
	close_char = '"';
      
      for (i=p, k=0; i<stringlen; i++, k++)
	{
	  if (k == subjbufsize-1)
	    {
	      subjbuf = realloc(subjbuf, sizeof(char) * (subjbufsize+1000));
	      if (subjbuf==NULL)
		{
		  printf("failed to allocate for subjbuf\n");
		}
	      subjbufsize+=1000;
	    }
 
	  if (k>=10) 
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
		      strncpy(matches[j], subjbuf, k+1);		      
		      matches[j][k+1] = '\0';
		      subjbuf = realloc(subjbuf, sizeof(char) * 1000);
		      if (subjbuf==NULL)
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

  int m_size = 2;
  *m_pos = malloc(sizeof(int) * m_size);
  if (*m_pos == NULL)
    {
      printf("malloc failed for m_pos\n");
      exit(1);
    }

  char *buf;
  int pos;
  int start = 0;
  pos = boyer_moore(string, stringlen, pattern, 11);
  if (pos)
    {
      (*m_pos)[*m_count] = pos;
      *m_count+=1;
      if (*m_count == m_size) 
	{
	  *m_pos = realloc(*m_pos, sizeof(int) * (*m_count + m_size));
	  if (*m_pos == NULL)
	    {
	      printf("failed to realloc m_pos\n");
	      exit(1);
	    }
	  m_size += *m_count;
	}
      
      while (pos)
	{
	  start = pos + 11;
	  buf  = malloc(sizeof(char) * (4 + stringlen-start));
	  int i, j;
	  for (i=start, j=0; i<stringlen; j++, i++)
	    {
	      buf[j] = string[i]; 
	    }
	  buf[j+1] = '\0';
	 	  
	  pos = boyer_moore(buf, stringlen-start, pattern, 11);
	  if (pos)
	    {
	      pos += start;
	      	      
	      (*m_pos)[*m_count] = pos;
	      
	      *m_count+=1;
	      if (*m_count == m_size) 
		{
		  *m_pos = realloc(*m_pos, sizeof(int) * (*m_count + m_size));
		  if (*m_pos == NULL)
		    {
		      printf("failed to realloc m_pos\n");
		      exit(1);
		    }
		  m_size += *m_count;
		}
	    } 	    	  
	  free(buf);
	}
    }

}
