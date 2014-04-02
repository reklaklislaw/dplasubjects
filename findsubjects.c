#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ALPHABET_LEN 256
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)

void make_delta1(int *delta1, uint8_t *pat, int32_t patlen);
int is_prefix(uint8_t *word, int wordlen, int pos);
int suffix_length(uint8_t *word, int wordlen, int pos);
void make_delta2(int *delta2, uint8_t *pat, int32_t patlen);
int boyer_moore (uint8_t *string, uint32_t stringlen, uint8_t *pat, uint32_t patlen);

void find_subjects(FILE *in_file, FILE *out_file);
char** get_matches(char *string, int stringlen, int *m_pos, int m_count);
void get_match_positions(char *string, int stringlen, char *pattern,
			 int **m_pos, int *m_count);

 
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurrence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up 
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
void make_delta1(int *delta1, uint8_t *pat, int32_t patlen) {
  int i;
  for (i=0; i < ALPHABET_LEN; i++) {
    delta1[i] = NOT_FOUND;
  }
  for (i=0; i < patlen-1; i++) {
    delta1[pat[i]] = patlen-1 - i;
  }
}
 
// true if the suffix of word starting from word[pos] is a prefix 
// of word
int is_prefix(uint8_t *word, int wordlen, int pos) {
  int i;
  int suffixlen = wordlen - pos;
  // could also use the strncmp() library function here
  for (i = 0; i < suffixlen; i++) {
    if (word[i] != word[pos+i]) {
      return 0;
    }
  }
  return 1;
}
 
// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
int suffix_length(uint8_t *word, int wordlen, int pos) {
  int i;
  // increment suffix length i to the first mismatch or beginning
  // of the word
  for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
  return i;
}
 
// delta2 table: given a mismatch at pat[pos], we want to align 
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with 
// pat[patlen-1].
// 
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
// 
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as 
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
void make_delta2(int *delta2, uint8_t *pat, int32_t patlen) {
  int p;
  int last_prefix_index = patlen-1;
 
  // first loop
  for (p=patlen-1; p>=0; p--) {
    if (is_prefix(pat, patlen, p+1)) {
      last_prefix_index = p+1;
    }
    delta2[p] = last_prefix_index + (patlen-1 - p);
  }
 
  // second loop
  for (p=0; p < patlen-1; p++) {
    int slen = suffix_length(pat, patlen, p);
    if (pat[p - slen] != pat[patlen-1 - slen]) {
      delta2[patlen-1 - slen] = patlen-1 - p + slen;
    }
  }
}
 
int boyer_moore (uint8_t *string, uint32_t stringlen, uint8_t *pat, uint32_t patlen) {
  int i;
  int delta1[ALPHABET_LEN];
  int *delta2 = (int *)malloc(patlen * sizeof(int));
  make_delta1(delta1, pat, patlen);
  make_delta2(delta2, pat, patlen);
  i = patlen-1;
  while (i < stringlen) {
    int j = patlen-1;
    while (j >= 0 && (string[i] == pat[j])) {
      --i;
      --j;
    }
    if (j < 0) {
      free(delta2);
      return i+1; 
      // return pos of match instead
	//return (string + i+1);
    }
 
    i += max(delta1[string[i]], delta2[j]);
  }
  free(delta2);
  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc!=3)
    {
      printf("invalid number of args\n");
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
  if (pos != NULL)
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
      
      while (pos != NULL)
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
	  if (pos != NULL)
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
