#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"
#include "ntriple2json.h"

int main(int argc, char *argv[])
{
  if (argc!=4) 
    {
      printf("invalid arguments\n");
      exit(1);
    }
  
  char in[100], out[100];
  strcpy(in, argv[1]);
  strcpy(out, argv[2]);

  int bucket_size = atoi(argv[3]);

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
  
  parse(in_file, out_file, bucket_size);
      
  fclose(in_file);
  fclose(out_file);
  return 0;
}


void parse(FILE *in_file, FILE *out_file, int bucket_size)
{ 
  struct ntriple nt;
  int so_count = 0;
  int so_size = bucket_size;
  struct subjectObject *subjObj = malloc(so_size * sizeof(struct subjectObject));
  int hash_size = 10000;
  size_t *hashes = malloc(hash_size * sizeof(size_t));
  int hash_count = 0;
  size_t *collisions = malloc(so_size * sizeof(size_t));
  int eof = 0;
  while (1) 
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


      if (hash_count == hash_size)
	{
	  hashes = realloc(hashes, (hash_size+10000) * sizeof(size_t));
	  if (hashes==NULL)
	    {
	      printf("failed to realloc hashes\n");
	      exit(1);
	    }
	  hash_size += 10000;
	}
	  
      nt = get_next_ntriple(in_file, &eof);
      char *id = get_id(nt.subject);
      if (id=='\0')
	continue;

      size_t hash = get_hash(id, bucket_size);
      //printf("%lu %s\n", hash, id);
      
      if (subjObj[hash].id != '\0') 
	{ 
	  if (strcmp(subjObj[hash].id, id) == 0) 
	    {
	      //printf("adding to hash %lu:  id:%s url:%s\n", hash, id, subjObj[hash].url);
	      add_ntriple_data(&subjObj[hash], nt);
	    }
	  else {
	    //TODO something about collisions
	    //init_new_entry(&subjObj[p], nt, id);
	    //add_ntriple_data(&subjObj[p], nt);
	    //hashes[hash_count] = p;
	    //hash_count += 1;
	    collisions[hash] += 1;
	    //so_count++;
	  }
	}

      else
	{	  
	  //printf("creating new at hash %d: %s\n", hash, id);
	  init_new_entry(&subjObj[hash], nt, id);
	  add_ntriple_data(&subjObj[hash], nt);
	  hashes[hash_count] = hash;
	  hash_count += 1;
	  collisions[hash] = 0;
	  so_count++;
	}


      free(nt.subject);
      free(nt.predicate);
      free(nt.object);

      if (eof)
	break;
    }

  int i;
  size_t h;
  float hc = 0.0;
  float cc = 0.0;
  for (i=0; i<hash_count; i++)
    {
      h = hashes[i];
      if (subjObj[h].id == '\0')
	continue;
      else {
	hc += 1.0;
	cc += collisions[h];
	printf("id: %s  url: %s  hash:%lu  collisions:%lu \n ", 
	       subjObj[h].id, subjObj[h].url, h, collisions[h]);
      }
    }
  
  printf("average collisions per hash:%f\n", cc/hc);

}

size_t get_hash(char *id, int bucket_size)
{
  const bsize = 32;
  size_t b = rand() % 5000000;
  while (*id)
    {
      b ^= *id++;
      b = ((b << 2) | (b >> (bsize - 2)));
    }

  //printf("hash:%lu\n", b % 300007);
  return b % bucket_size;
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
	  *subjObj->altLabel = realloc(*subjObj->altLabel, 
				       (subjObj->altSize + 10) * sizeof(char));
	  if (*subjObj->altLabel==NULL) 
	    {
	      printf("failed to realloc altLabel\n");
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
	  *subjObj->narrower = realloc(*subjObj->narrower, 
				       (subjObj->narrowerSize + 10) * sizeof(char));
	  if (*subjObj->narrower==NULL) 
	    {
	      printf("failed to realloc narrower\n");
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
	  *subjObj->broader = realloc(*subjObj->broader, 
				      (subjObj->broaderSize + 10) * sizeof(char));
	  if (*subjObj->broader==NULL) 
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
	  *subjObj->closeMatch = realloc(*subjObj->closeMatch, 
					 (subjObj->closeMatchSize + 10) * sizeof(char));
	  if (subjObj->closeMatch==NULL) 
	    {
	      printf("failed to realloc closeMatch\n");
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
	  *subjObj->related = realloc(*subjObj->related, 
				      (subjObj->relatedSize + 10) * sizeof(char));
	  if (subjObj->related==NULL) 
	    {
	      printf("failed to realloc related\n");
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


void init_new_entry(struct subjectObject *subjObj, struct ntriple nt, char *id)
{
  alloc_new_entry(subjObj);
  strcpy(subjObj->id, id);
  subjObj->url = malloc(nt.subjectSize * sizeof(char) + 2);
  
  int i, j;
  for (i=1, j=0; i<nt.subjectSize-1; i++, j++)
    subjObj->url[j] = nt.subject[i];
  subjObj->url[j] = '\0';
}

void alloc_new_entry(struct subjectObject *subjObj)
{
  int size = 25;
  subjObj->id = malloc(size * sizeof(char));
  subjObj->altLabel = malloc(size * sizeof(char));
  subjObj->narrower = malloc(size * sizeof(char));
  subjObj->broader = malloc(size * sizeof(char));
  subjObj->closeMatch = malloc(size * sizeof(char));
  subjObj->related = malloc(size * sizeof(char));
  subjObj->altCount = 0;
  subjObj->altSize = size;
  subjObj->narrowerCount = 0;
  subjObj->narrowerSize = size;
  subjObj->broaderCount = 0;
  subjObj->broaderSize = size;
  subjObj->closeMatchCount = 0;
  subjObj->closeMatchSize = size;
  subjObj->relatedCount = 0;
  subjObj->relatedCount = size;
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
  if (subject[0] == '_')
    return '\0';
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
  id[k] = '\0';
  free(buf);
  return id;
}



struct ntriple get_next_ntriple(FILE *in_file, int *eof)
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
	  *ntepointer = realloc(*ntepointer, (entry_size+51) * sizeof(char));
	  if (*ntepointer==NULL)
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
	if (c==EOF) {
	  *eof = 1;
	  break;
	} else {
	  while (c!='\n')
	    c = getc(in_file);
	  break;
	}
      }      
    }

  ntepointer = '\0';
  return nt;
}
