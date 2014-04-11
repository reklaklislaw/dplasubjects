#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"
#include "ntriple2json.h"

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
  
  struct ntriple nt;
  int so_count = 0;
  int so_size = 1000;
  struct subjectObject *subjObj = malloc(so_size * sizeof(struct subjectObject));
  
  int pos = 0;
  char c;
  c = getc(in_file);
  while (c != EOF) 
    {
      if (so_count == so_size)
	{
	  subjObj = realloc(subjObj, (so_size+1000) * sizeof(struct subjectObject));
	  if (subjObj==NULL) {
	    printf("failed to realloc subjObj\n");
	    exit(1);
	  }
	  so_size += 1000;
	}

      nt = get_next_ntriple(in_file);
      char *id = get_id(nt.subject);
      
      int prev_entry = has_entry(subjObj, so_count, id);
      if (prev_entry)
	{
	  add_ntriple_data(&subjObj[prev_entry], nt);
	}
      else 
	{	  
	  alloc_new_entry(&subjObj[so_count]);
	  subjObj[so_count].id = id;
	  subjObj[so_count].url = malloc(nt.subjectSize * sizeof(char) + 2);
	  int i, j;
	  for (i=1, j=0; i<nt.subjectSize-1; i++, j++)
	    subjObj[so_count].url[j] = nt.subject[i];
	  subjObj[so_count].url[j] = '\0';
	  add_ntriple_data(&subjObj[so_count], nt);
	  so_count++;
	}
      
    }


}


void add_ntriple_data(struct subjectObject *subjObj, struct ntriple nt)
{
  int i, j;
  if (match_tag(nt.predicate, "prefLabel")) 
    {
      subjObj->prefLabel = malloc(nt.objectSize * sizeof(char) + 1);
      strcpy(subjObj->prefLabel, nt.object);
    }
  
  else if (match_tag(nt.predicate, "altLabel")) 
    {
      if (subjObj->altCount == subjObj->altSize) 
	{
	  subjObj->altLabel = realloc(subjObj->altLabel, 
				      (subjObj->altSize + 10) * sizeof(char));
	  if (subjObj->altCount==NULL) 
	    {
	      printf("failed to realloc altCount\n");
	      exit(1);
	    }
	  subjObj->altSize += 10;
	}
      subjObj->altLabel[subjObj->altCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->altLabel[subjObj->altCount][j] = nt.object[i];
      subjObj->altLabel[subjObj->altCount][j] = '\0';
      subjObj->altCount += 1;
    }
  
  else if (match_tag(nt.predicate, "narrower")) 
    {
      if (subjObj->narrowerCount == subjObj->narrowerSize) 
	{
	  subjObj->narrower = realloc(subjObj->narrower, 
				      (subjObj->narrowerSize + 10) * sizeof(char));
	  if (subjObj->narrowerCount==NULL) 
	    {
	      printf("failed to realloc narrowerCount\n");
	      exit(1);
	    }
	  subjObj->narrowerSize += 10;
	}
      subjObj->narrower[subjObj->narrowerCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->narrower[subjObj->narrowerCount][j] = nt.object[i];
      subjObj->narrower[subjObj->narrowerCount][j] = '\0';
      subjObj->narrowerCount += 1;
    }

  else if (match_tag(nt.predicate, "narrower")) 
    {
      if (subjObj->narrowerCount == subjObj->narrowerSize) 
	{
	  subjObj->narrower = realloc(subjObj->narrower, 
				      (subjObj->narrowerSize + 10) * sizeof(char));
	  if (subjObj->narrowerCount==NULL) 
	    {
	      printf("failed to realloc narrowerCount\n");
	      exit(1);
	    }
	  subjObj->narrowerSize += 10;
	}
      subjObj->narrower[subjObj->narrowerCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->narrower[subjObj->narrowerCount][j] = nt.object[i];
      subjObj->narrower[subjObj->narrowerCount][j] = '\0';
      subjObj->narrowerCount += 1;
    }

  else if (match_tag(nt.predicate, "broader")) 
    {
      if (subjObj->broaderCount == subjObj->broaderSize) 
	{
	  subjObj->broader = realloc(subjObj->broader, 
				      (subjObj->broaderSize + 10) * sizeof(char));
	  if (subjObj->broaderCount==NULL) 
	    {
	      printf("failed to realloc broaderCount\n");
	      exit(1);
	    }
	  subjObj->broaderSize += 10;
	}
      subjObj->broader[subjObj->broaderCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->broader[subjObj->broaderCount][j] = nt.object[i];
      subjObj->broader[subjObj->broaderCount][j] = '\0';
      subjObj->broaderCount += 1;
    }

  else if (match_tag(nt.predicate, "closeMatch")) 
    {
      if (subjObj->closeMatchCount == subjObj->closeMatchSize) 
	{
	  subjObj->closeMatch = realloc(subjObj->closeMatch, 
				      (subjObj->closeMatchSize + 10) * sizeof(char));
	  if (subjObj->closeMatchCount==NULL) 
	    {
	      printf("failed to realloc closeMatchCount\n");
	      exit(1);
	    }
	  subjObj->closeMatchSize += 10;
	}
      subjObj->closeMatch[subjObj->closeMatchCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->closeMatch[subjObj->closeMatchCount][j] = nt.object[i];
      subjObj->closeMatch[subjObj->closeMatchCount][j] = '\0';
      subjObj->closeMatchCount += 1;
    }

  else if (match_tag(nt.predicate, "related")) 
    {
      if (subjObj->relatedCount == subjObj->relatedSize) 
	{
	  subjObj->related = realloc(subjObj->related, 
				      (subjObj->relatedSize + 10) * sizeof(char));
	  if (subjObj->relatedCount==NULL) 
	    {
	      printf("failed to realloc relatedCount\n");
	      exit(1);
	    }
	  subjObj->relatedSize += 10;
	}
      subjObj->related[subjObj->relatedCount] = malloc(nt.objectSize * sizeof(char));
      for (i=1, j=0; i<nt.objectSize-1; i++, j++)
	subjObj->related[subjObj->relatedCount][j] = nt.object[i];
      subjObj->related[subjObj->relatedCount][j] = '\0';
      subjObj->relatedCount += 1;
    }


}

int match_tag(char *predicate, char *tag)
{

  char *buf = malloc(20 * sizeof(char));
  int t = 0;
  int i, j;
  i = j = 0;
  while (1) 
    {
      if (predicate[i] == '>')
	break;
      if (predicate[i] == '#')
	t = 1;
      else if (t) 
	{
	  buf[j] = predicate[i];
	  j++;
	}
      i++;
    }

  if (strcmp(buf, tag) == 0)
    return 1;
  else
    return 0;
 
}


void alloc_new_entry(struct subjectObject *subjObj)
{
  subjObj->altLabel = malloc(10 * sizeof(char));
  subjObj->altCount = 0;
  subjObj->altSize = 10;
  subjObj->narrower = malloc(10 * sizeof(char));
  subjObj->narrowerCount = 0;
  subjObj->narrowerSize = 10;
  subjObj->broader = malloc(10 * sizeof(char));
  subjObj->broaderCount = 0;
  subjObj->broaderSize = 10;
  subjObj->closeMatch = malloc(10 * sizeof(char));
  subjObj->closeMatchCount = 0;
  subjObj->closeMatchSize = 10;
  subjObj->related = malloc(10 * sizeof(char));
  subjObj->relatedCount = 0;
  subjObj->relatedCount = 10;
}


int has_entry(struct subjectObject *subjObj, int so_count, char *id)
{
  int i;
  for (i=0; i<so_count; i++)
    {
      if (strcmp(subjObj[i].id, id) == 0) {
	return i;
      }
    }
  return 0;
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
	nt.subjectSize = s;
	nt.predicate[p] = '\0';
	nt.predicateSize = s;
	nt.object[o] = '\0';
	nt.objectSize = o;
	while (c!='\n')
	  c = getc(in_file);
	break;
      }      
    }
    
  ntepointer = '\0';
  return nt;
}
