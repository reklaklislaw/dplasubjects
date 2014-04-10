#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"

void parse(FILE *in_file, FILE *out_file);
struct ntriple get_next_ntriple(FILE *in_file);
char *get_id(char *subject);

struct ntriple {
  char *subject;
  char *predicate;
  char *object;
};


struct subjectObject {
  char *id;
  char *url;
  char *prefLabel;
  char **altLabel;
  int altCount;
  char **narrower;
  int narrowerCount;
  char **broader;
  int broaderCount;
  char **closeMatch;
  int closeMatchCount;
  char **related;
  int relatedCount;
};


int main(int argc, char *argv[])
{
  if (argc!=3) 
    {
      printf("invalid arguments\n");
      exit(1);
    }
  
  char in[100], out[100];
  strcpy(in, argv[1]);
  strcpy(out, argv[2]);

  FILE *in_file = fopen(in, "r");
  if (in_file==NULL) 
    {
      printf("failed to open in_file\n");
      exit(1);
    }

  FILE *out_file = fopen(out, "w");
  if (out_file==NULL) 
    {
      printf("failed to open out_file\n");
      exit(1);
    }
  
  parse(in_file, out_file);
      
  fclose(in_file);
  fclose(out_file);
  return 0;
}


void parse(FILE *in_file, FILE *out_file)
{
  
  struct ntriple *nt = malloc( 1000 * sizeof(struct ntriple));
  int ntcount = 0;
  int ntsize = 1000;

  int pos = 0;
  char c;
  c = getc(in_file);
  while (c != EOF) 
    {
      if (ntcount == ntsize)
	{
	  nt = realloc(nt, (ntsize+1000) * sizeof(struct ntriple));
	  if (nt==NULL) {
	    printf("failed to realloc ntriple\n");
	    exit(1);
	  }
	  ntsize += 1000;
	}

      nt[ntcount] = get_next_ntriple(in_file);
      char *id = get_id(nt[ntcount].subject);

      //printf("SUBJECT:%s\n", nt[ntcount].subject);
      //printf("PREDICATE:%s\n", nt[ntcount].predicate);
      //printf("OBJECT:%s\n", nt[ntcount].object);      

      //free(nt[ntcount].subject);
      //free(nt[ntcount].predicate);
      //free(nt[ntcount].object);

      ntcount++;
            
      
    }

}


char *get_id(char *subject) 
{
  char *buf = malloc(100 * sizeof(char));
  int i = 0;
  int j = 0;
  int k = 0;
  char c;
  
  while (1)
    {
      c = subject[i];
      if (c != '>')
	i++;
      else
	break;
    }
  
  int p = i;

  while (1)
    { 
      c = subject[i];     
      if (c != '/')
	{
	  buf[i] = c;
	  i--;
	  j++;
	} 
      else
	break;
    }

  char *id = malloc(j * sizeof(char));
  for (k=0, p=(p-j)+1; k<j-1; k++, p++) {
    id[k] = buf[p];
  }

  free(buf);
  return id;
}



struct ntriple get_next_ntriple(FILE *in_file)
{
  
  int entry_size = 80;
  struct ntriple nt;
  nt.subject = malloc(entry_size * sizeof(char));
  nt.predicate = malloc(entry_size * sizeof(char));
  nt.object = malloc(entry_size * sizeof(char));
  
  int div_count = 0;
  int counter = 0;
  char **ntepointer;
  int s, p, o;
  s = p = o = 0;
  char c = getc(in_file);
  char prev = c;
  while(1)
    { 
      if (counter==entry_size) 
	{
	  *ntepointer = realloc(*ntepointer, (entry_size+50) * sizeof(char));
	  if (ntepointer==NULL)
	    {
	      printf("failed to realloc ntriple entry\n");
	      exit(1);
	    }
	  entry_size += 50;
	}      
      
      if (c == ' ' && prev == '>')
	div_count += 1;

      if (div_count == 0) {
	nt.subject[s] = c;
	s++;
	ntepointer = &nt.subject;
	counter = s;
      }
      
      else if (div_count == 1) {
	nt.subject[s] = '\0';
	if (p==0 && c == ' ') {}
	else 
	  {
	    nt.predicate[p] = c;
	    p++;
	  }
	ntepointer = &nt.predicate;
	counter = p;
      }
      else if (div_count == 2) {
	nt.predicate[p] = '\0';
	if (o == 0 && c == ' ') {}
	else 
	  {
	    nt.object[o] = c;
	    o++;
	  }
	ntepointer = &nt.object;
	counter = o;
      }

      prev = c;
      c = getc(in_file);
      if (c == EOF || (c == '.' && prev == ' ') ) {
	nt.subject[s] = '\0';
	nt.predicate[p] = '\0';
	nt.object[o] = '\0';
	while (c!='\n')
	  c = getc(in_file);
	break;
      }      
    }
    
  ntepointer = '\0';
  return nt;
}
