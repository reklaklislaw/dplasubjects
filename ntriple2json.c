#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "boyermoore.c"
#include "ntriple2json.h"

int main(int argc, char *argv[])
{
  if (argc!=5) 
    {
      printf("invalid arguments\n");
      exit(1);
    }
  
  char in[100], out[100];
  strcpy(in, argv[1]);
  strcpy(out, argv[2]);

  int bucket_count = atoi(argv[3]);
  int bucket_size = atoi(argv[4]);

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
  
  parse(in_file, out_file, bucket_count, bucket_size);
      
  fclose(in_file);
  fclose(out_file);
  return 0;
}


void parse(FILE *in_file, FILE *out_file, int bucket_count, int bucket_size)
{ 
  struct ntriple nt;

  int b_count = 0;
  int b_size = bucket_count;
  struct bucket *buckets = malloc(b_size * sizeof(struct bucket));

  int i;
  for (i=0; i<bucket_count; i++)
    {
      buckets[i].subjObj = malloc(bucket_size * sizeof(struct subjectObject));
      buckets[i].size = bucket_size;
      buckets[i].free = bucket_size;
    }

  //int o_count = 0;
  //int o_size = bucket_size/10;
  //struct bucket *overflow = malloc(o_size * sizeof(struct bucket));
  
  int hash_size = 10000;
  size_t *hashes = malloc(hash_size * sizeof(size_t));
  int hash_count = 0;
  size_t *collisions = malloc(bucket_count * sizeof(size_t));
  int overflow = 0;
  
  int eof = 0;
  while (1) 
    {
      if (b_count == b_size)
	{
	  printf("realloc\n");
	  buckets = realloc(buckets, (b_size+1000) * sizeof(struct bucket));
	  if (buckets==NULL) {
	    printf("failed to realloc subjObj\n");
	    exit(1);
	  }
	  b_size += 1000;
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

      size_t hash = get_hash(id, bucket_count);
      //printf("%lu %s\n", hash, id);
      
      if (buckets[hash].free == buckets[hash].size) 
	{
	  init_new_entry(&buckets[hash].subjObj[0], nt, id);
	  add_ntriple_data(&buckets[hash].subjObj[0], nt);
	  buckets[hash].free -= 1;
	  hashes[hash_count] = hash;
	  hash_count += 1;
	  b_count += 1;
	  collisions[hash] = 0;
	}
      
      else if (buckets[hash].free > 0)
	{
	  int next;
	  int entry = has_entry(buckets[hash], id);
	  if (entry == -1) {
	    next = buckets[hash].size - buckets[hash].free;
	    init_new_entry(&buckets[hash].subjObj[next], nt, id);
	    buckets[hash].free -= 1;
	    collisions[hash] += 1;
	  }
	  else {
	    next = entry;
	  }

	  add_ntriple_data(&buckets[hash].subjObj[next], nt);	  
	  //printf("%s\n", buckets[hash].subjObj[next].id);
	}
      
      else if (buckets[hash].free == 0)
	{
	  overflow += 1;
	  collisions[hash] += 1;
	  //printf("bucket %lu is full\n", hash);
	}
            
      free(nt.subject);
      free(nt.predicate);
      free(nt.object);

      if (eof)
	break;
    }
    
  fprintf(out_file, "{");
  int j, a;
  size_t h;
  float hc = 0.0;
  float cc = 0.0;
  for (i=0; i<hash_count; i++)
    {
      h = hashes[i];
      hc += 1.0;
      cc += collisions[h];
      int end = buckets[h].size-buckets[h].free;
      for (j=0; j<end; j++) 
	{
	  if (buckets[h].subjObj[j].id == '\0')
	    continue;
	  else {
	    fprintf(out_file, "\"%s\":{\n", buckets[h].subjObj[j].id);
	    fprintf(out_file, "\t\"url\":\"%s\",\n", buckets[h].subjObj[j].url);
	    fprintf(out_file, "\t\"language\":\"%s\",\n", buckets[h].subjObj[j].language);
	    fprintf(out_file, "\t\"prefLabel\":%s", buckets[h].subjObj[j].prefLabel);
	    
	    if (buckets[h].subjObj[j].altCount > 0)
	      {
		fprintf(out_file, ",\n\t\"altLabel\":[\n");
		for (a=0; a<buckets[h].subjObj[j].altCount; a++)
		  {
		    fprintf(out_file, "\t\t%s", 
			    buckets[h].subjObj[j].altLabel[a]);
		    if (a!=buckets[h].subjObj[j].altCount-1)
		      fprintf(out_file, ",\n");
		    else
		      fprintf(out_file, "]");
		  }
	      }

	    if (buckets[h].subjObj[j].relatedCount > 0)
	      {
		fprintf(out_file, ",\n\t\"related\":[\n");
		for (a=0; a<buckets[h].subjObj[j].relatedCount; a++)
		  {
		    fprintf(out_file, "\t\t\"%s\"", 
			    buckets[h].subjObj[j].related[a]);
		    if (a!=buckets[h].subjObj[j].relatedCount-1)
		      fprintf(out_file, ",\n");
		    else
		      fprintf(out_file, "]");
		  }
	      }
	    
	    if (buckets[h].subjObj[j].narrowerCount > 0)
	      {
		fprintf(out_file, ",\n\t\"narrower\":[\n");
		for (a=0; a<buckets[h].subjObj[j].narrowerCount; a++)
		  {
		    fprintf(out_file, "\t\t\"%s\"", 
			    buckets[h].subjObj[j].narrower[a]);
		    if (a!=buckets[h].subjObj[j].narrowerCount-1)
		      fprintf(out_file, ",\n");
		    else
		      fprintf(out_file, "]");
		  }
	      }

	    
	    if (buckets[h].subjObj[j].broaderCount > 0)
	      {
		fprintf(out_file, ",\n\t\"broader\":[\n");
		for (a=0; a<buckets[h].subjObj[j].broaderCount; a++)
		  {
		    fprintf(out_file, "\t\t\"%s\"", 
			    buckets[h].subjObj[j].broader[a]);
		    if (a!=buckets[h].subjObj[j].broaderCount-1)
		      fprintf(out_file, ",\n");
		    else
		      fprintf(out_file, "]");
		  }
	      }

	    
	    if (buckets[h].subjObj[j].closeMatchCount > 0)
	      {
		fprintf(out_file, ",\n\t\"closeMatch\":[\n");
		for (a=0; a<buckets[h].subjObj[j].closeMatchCount; a++)
		  {
		    fprintf(out_file, "\t\t\"%s\"", 
			    buckets[h].subjObj[j].closeMatch[a]);
		    if (a!=buckets[h].subjObj[j].closeMatchCount-1)
		      fprintf(out_file, ",\n");
		    else
		      fprintf(out_file, "]");
		  }
	      }
	    
	    if (i!=hash_count-1)
	      fprintf(out_file, "},\n");
	    else
	      fprintf(out_file, "}}");
	  }
	}
    }
  
  printf("hash count: %d\n", hash_count);
  printf("average collisions per hash:%f\n", cc/hc);
  printf("overflow count:%d\n", overflow);
  

}


size_t get_hash(char *id, int bucket_size)
{
  const bsize = 32;
  size_t b = 5000000;
  size_t h;
  while (*id)
    {
      b ^= *id++;
      h = (b << 2); 
      if (h == 0)
	b = (b >> (bsize - 2));
      else
	b = h;
    }

  return b % bucket_size;
}


void add_ntriple_data(struct subjectObject *subjObj, struct ntriple nt)
{
  int i, j;
  if (match_tag(nt.predicate, "prefLabel")) 
    {
      subjObj->prefLabel = malloc(nt.objectSize * sizeof(char) + 1);
      subjObj->language = malloc(4 * sizeof(char));
      for (i=0; i<nt.objectSize-1; i++)
	{
	  if (i == nt.objectSize-4) 
	    {
	      subjObj->prefLabel[i] = '\0';
	      subjObj->language[0] = nt.object[i];
	      subjObj->language[1] = nt.object[i+1];
	      subjObj->language[2] = nt.object[i+2];
	      break;
	    }
	  else
	    subjObj->prefLabel[i] = nt.object[i];
	}
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
      if (nt.object[0] != '_')
	{
	  subjObj->altLabel[subjObj->altCount] = malloc(nt.objectSize * sizeof(char));
	  for (i=0; i<nt.objectSize-4; i++)
	    subjObj->altLabel[subjObj->altCount][i] = nt.object[i];
	  subjObj->altLabel[subjObj->altCount][i] = '\0';
	  subjObj->altCount += 1;
	}
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
	  if (*subjObj->closeMatch==NULL) 
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
	  if (*subjObj->related==NULL) 
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
  subjObj->relatedSize = size;
}


int has_entry(struct bucket bucket, char *id)
{

  int i;
  int lim = bucket.size - bucket.free;
  for (i=0; i<lim; i++)
    {
      if (bucket.subjObj[i].id != '\0') {

	if (strcmp(bucket.subjObj[i].id, id) == 0) {
	  return i;
	}
      }
    }
  
  return -1;
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
