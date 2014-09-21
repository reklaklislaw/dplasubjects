/* Takes SKOS N-Triple metadata and reformats as simplied JSON (just uri and prefLabel)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "boyermoore.c"
#include "skosntriple2simplejson.h"

int main(int argc, char *argv[])
{
  if (argc!=5) 
    {
      printf("Usage: in_file out_file bucket_count bucket_size\n");
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

  int b_size = bucket_count;
  struct bucket *buckets = malloc(b_size * sizeof(struct bucket));
  if (!buckets) 
    {
      printf("Failed to alloc buckets\n");
      exit(1);
    }
    
  int i;
  for (i=0; i<bucket_count; i++)
    {
      buckets[i].subjObj = malloc(bucket_size * sizeof(struct subjectObject));
      if (!buckets[i].subjObj) 
	{
	  printf("Failed to alloc subjObj\n");
	  exit(1);
	}

      buckets[i].size = bucket_size;
      buckets[i].free = bucket_size;
    }
  
  int overflow_count = 0;
  int overflow_size = bucket_count/10;
  struct bucket overflow;
  overflow.subjObj = malloc(overflow_size * sizeof(struct subjectObject));
  if (!overflow.subjObj) 
    {
      printf("Failed to alloc overflow\n");
      exit(1);
    }

  overflow.size = overflow_size;
  overflow.free = overflow_size;
  
  int id_count = 0;  
  int hash_size = bucket_count/10;
  int32_t *hashes = malloc(hash_size * sizeof(int32_t));
  if (!hashes) 
    {
      printf("Failed to alloc hashes\n");
      exit(1);
    }

  int entry;
  int hash_count = 0;
  int32_t *collisions = malloc(bucket_count * sizeof(int32_t));
  if (!collisions) 
    {
      printf("Failed to alloc collisions\n");
      exit(1);
    }

  int eof = 0;
  while (1) 
    {
      if (eof)
	break;

      if (hash_count == hash_size)
	{
	  hashes = realloc(hashes, (hash_size+(bucket_count/10)) * sizeof(int32_t));
	  if (!hashes)
	    {
	      printf("failed to realloc hashes\n");
	      exit(1);
	    }
	  hash_size += bucket_count/10;
	}

      nt = get_next_ntriple(in_file, &eof);
      char *id = get_id(nt.subject);
            
      if (id=='\0')
	continue;

      int32_t hash = jch(id, bucket_count);

      entry = has_entry(buckets[hash], id);
      
      if (buckets[hash].free == buckets[hash].size) 
	{
	  init_new_entry(&buckets[hash].subjObj[0], nt, id);
	  add_ntriple_data(&buckets[hash].subjObj[0], nt);
	  buckets[hash].free -= 1;
	  hashes[hash_count] = hash;
	  hash_count += 1;
	  collisions[hash] = 0;
	  id_count += 1;  
	} else {

	entry = has_entry(buckets[hash], id);

	if (entry != -1) {
	  add_ntriple_data(&buckets[hash].subjObj[entry], nt);	  
	}
	else if (entry == -1)
	  {
	    if (buckets[hash].free > 0)
	      {
		entry = buckets[hash].size - buckets[hash].free;
		init_new_entry(&buckets[hash].subjObj[entry], nt, id);
		add_ntriple_data(&buckets[hash].subjObj[entry], nt);	  
		buckets[hash].free -= 1;
		collisions[hash] += 1;
		id_count += 1;
	      }
	    else
	      {
		printf ("OVERFLOW\n");
		if (overflow_count == overflow_size) 
		  {
		    overflow.subjObj = realloc(overflow.subjObj, 
					       (overflow_size+(bucket_count/10)) * 
					       sizeof(struct subjectObject));
		    if (!overflow.subjObj)
		      {
			printf("failed to realloc overflow\n");
			exit(1);
		      }
		    overflow_size += bucket_count/10;
		  }	  
		
		entry = has_entry(overflow, id);
		if (entry == -1 ) {
		  entry = overflow.size - overflow.free;
		  init_new_entry(&overflow.subjObj[overflow_count], nt, id);
		  overflow.free -= 1;
		  overflow_count += 1;
		  id_count += 1;
		} else {
		  add_ntriple_data(&overflow.subjObj[entry], nt);
		  collisions[hash] += 1;
		}
	      }
	  }
      }

      free(nt.subject); nt.subject=NULL;
      free(nt.predicate); nt.predicate=NULL;
      free(nt.object); nt.object=NULL;
    }

  write_json(out_file, buckets, 
   	     hashes, hash_count, 
   	     collisions, overflow, 
   	     overflow_count, id_count);

}


void write_json(FILE *out_file, struct bucket *buckets, 
		int32_t *hashes, int hash_count,
		int32_t *collisions, struct bucket overflow,
		int overflow_count, int id_count)
{
  printf("Writing %d (+ %d overflow) entries to file...\n", id_count, overflow_count);
  int i, j;
  int32_t h;
  float hc = 0.0;
  float cc = 0.0;

  fprintf(out_file, "{");
  for (i=0; i<hash_count; i++)
    {
      h = hashes[i];
      hc += 1.0;
      cc += collisions[h];
      int end = buckets[h].size - buckets[h].free;
      for (j=0; j<end; j++) 
	{
	  if (buckets[h].subjObj[j].id == '\0')
	    continue;
	  else {
	    write_entry(out_file, buckets[h].subjObj[j]);
	    if (i<hash_count-1) 
	      fprintf(out_file, ",");
	    else
	      if (j<end-1 || overflow_count > 0)
		fprintf(out_file, ",");
	  }
	}      
    }
  
  for (i=0; i<overflow_count; i++)
    {
      if (overflow.subjObj[i].id == '\0')
	continue;
      else {
	write_entry(out_file, overflow.subjObj[i]);
	if (i != overflow_count-1)
	  fprintf(out_file, ",");	    
      }
    }

  fprintf(out_file, "}");	    	  
  
  printf(" unique items: %d\n hash count: %d\n average \
collisions per hash:%f\n overflow count:%d\n", 
	 id_count, hash_count, cc/hc, overflow_count);


}


void write_entry(FILE *out_file, struct subjectObject subjObj)
{
  fprintf(out_file, "\"%s\":{\"uri\":\"%s\",\"prefLabel\":%s}", 
	  subjObj.id, subjObj.uri, subjObj.prefLabel);
}


//see arxiv.org/pdf/1406.2294v1.pdf
int32_t jch(char *key, int32_t num_buckets)
{
  int64_t b = -1, j = 0;
  long long i = 1;			       
  uint64_t k = get_ll_id(key);
  unsigned long long s = 2862933555777941757;
  while (j < num_buckets) 
    {
      b = j;
      k = k * s + 1;
      j = (b + 1) * ( (i << 31) / ((k >> 33) + 1));
    }
  return b;	
}


uint64_t get_ll_id(char *id)
{
  char *key = malloc(64 * sizeof(char));
  if (!key) 
    {
      printf("Failed to alloc key\n");
      exit(1);
    }

  uint64_t i = 0, p = 0;
  uint64_t c;
  while (id[p]!='\0')
    {
      if (p>1) {	
	c = (uint) id[p];
	key[i] = c;
	i++;
      } 
      p++;
    }
  key[i] = '\0';

  uint64_t k = atoi(key);
  free(key);
  key = NULL;
  return k;
}



void add_ntriple_data(struct subjectObject *subjObj, struct ntriple nt)
{
  int i, j;
  if (match_tag(nt.predicate, "prefLabel")) 
    {
      if (subjObj->prefLabel) {
	printf("WARNING: skipping %s  %s... already has prefLabel:%s (%s)\n", 
	       subjObj->id, nt.object, subjObj->prefLabel, subjObj->id);
      	return;
      } 

      subjObj->prefLabel = malloc(nt.objectSize * sizeof(char));
      if (!subjObj->prefLabel) 
	{
	  printf("Failed to alloc prefLabel\n");
	  exit(1);
	}

      for (i=0; i<nt.objectSize; i++)
	{
	  if (i >= nt.objectSize-4) 
	    {
	      subjObj->prefLabel[i] = '\0';
	      break;
	    }
	  else {
	    subjObj->prefLabel[i] = nt.object[i];
	  }
	}
    }
}

int match_tag(char *predicate, char *tag)
{
  
  char *buf = malloc(20 * sizeof(char));
  if (!buf) 
    {
      printf("Failed to alloc buf\n");
      exit(1);
    }

  int t = 0;
  int i, j;
  i = j = 0;
  while (1) 
    {
      if (predicate[i] == '>') {
	buf[j] = '\0';
	break;
      }
      if (predicate[i] == '#')
	t = 1;
      else if (t) 
	{
	  buf[j] = predicate[i];
	  j++;
	}
      i++;
    }

  int match = strcmp(buf, tag);

  free(buf);
  buf = NULL;

  if (match == 0)
    return 1;
  else
    return 0;
 
}


void init_new_entry(struct subjectObject *subjObj, struct ntriple nt, char *id)
{
  int i, j;
  alloc_new_entry(subjObj);
  j = 16;
  subjObj->id = malloc(j * sizeof(char));
  if (!subjObj->id)
    {
      printf("Failed to malloc id\n");
      exit(1);
    }
  i = 0;
  while (1)
    {
      if (i == j) 
	{
	  subjObj->id = realloc(subjObj->id, j+16 * sizeof(char));
	  if (!subjObj->id) 
	    {
	      printf("Failed to realloc subjObj->id\n");
	      exit(1);
	    }
	  j += 16;
	}

      if (!id[i]) 
	{
	  subjObj->id[i] = id[i];
	  break;
	}
      else
	subjObj->id[i] = id[i];

      i++;
    }

  subjObj->uri = malloc(nt.subjectSize * sizeof(char));    
  if (!subjObj->uri)
    {
      printf("Failed to malloc uri\n");
      exit(1);
    }
  for (i=1, j=0; i<nt.subjectSize-1; i++, j++)
    subjObj->uri[j] = nt.subject[i];
  subjObj->uri[j] = '\0';  
}

void alloc_new_entry(struct subjectObject *subjObj)
{
  subjObj->id = NULL;
  subjObj->uri = NULL;
  subjObj->prefLabel = NULL;
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
  if (!buf)
    {
      printf("Failed to malloc buf\n");
      exit(1);
    }
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
  if (!id)
    {
      printf("Failed to malloc id\n");
      exit(1);
    }
  for (k=0, p=(p-j)+1; k<j-1; k++, p++) {
    id[k] = buf[p];
  }
  id[k] = '\0';

  free(buf);
  buf=NULL;

  return id;
}



struct ntriple get_next_ntriple(FILE *in_file, int *eof)
{
  
  int entry_size = 80;
  
  struct ntriple nt;

  nt.subject = malloc(entry_size * sizeof(char));
  if (!nt.subject)
    {
      printf("Failed to malloc nt.subject\n");
      exit(1);
    }
  nt.predicate = malloc(entry_size * sizeof(char));
  if (!nt.predicate)
    {
      printf("Failed to malloc nt.predicate\n");
      exit(1);
    }
  nt.object = malloc(entry_size * sizeof(char));  
  if (!nt.object)
    {
      printf("Failed to malloc nt.object\n");
      exit(1);
    }
  
  nt.subjectSize = nt.predicateSize = nt.objectSize = entry_size;

  int div_count = 0;
  
  int s=0, p=0, o=0;

  int m = 160;
  int bytes_read = 0;
  char *buf = NULL;
  buf = (char*) malloc(m * sizeof(char));  
  if (!buf) 
    {
      printf("Failed to alloc buf\n");
      exit(1);
    }
  
  int seekp;
  bytes_read = fread(buf, 1, m, in_file);
  char c = buf[0];
  int bcount = 1;
  char prev = c;

  int q = 0;
  
  while(1)
    {

      if (c=='"' && prev!='\\')
	{
	  if (q)
	    q = 0;
	  else
	    q = 1;
	}

      if (s == nt.subjectSize)
	{
	  nt.subject = realloc(nt.subject, (s+20) * sizeof(char));
	  if (nt.subject==NULL)
	    {
	      printf("failed to realloc ntriple subject entry\n");
	      exit(1);
	    }
	  nt.subjectSize += 20;
	}      
      
      if (p == nt.predicateSize)
	{
	  nt.predicate = realloc(nt.predicate, (p+20) * sizeof(char));
	  if (nt.predicate==NULL)
	    {
	      printf("failed to realloc ntriple predicate entry\n");
	      exit(1);
	    }
	  nt.predicateSize += 20;
	}      

      if (o == nt.objectSize)
	{
	  nt.object = realloc(nt.object, (o+20) * sizeof(char));
	  if (nt.object==NULL)
	    {
	      printf("failed to realloc ntriple object entry\n");
	      exit(1);
	    } 
	  nt.objectSize += 20;
	}      
	
      if (c == ' ' && prev == '>')
	div_count += 1;

      if (div_count == 0) {
	nt.subject[s] = c;
	s++;
      }
      
      else if (div_count == 1) {
	if (p==0 && c == ' ') {}
	else 
	  {
	    nt.predicate[p] = c;
	    p++;
	  }	
      }
      
      else if (div_count == 2) {
	if (o == 0 && c == ' ') {}
	else 
	  {
	    nt.object[o] = c;
	    o++;
	  }	
      }

      prev = c;
      if (bcount < m) 
	{
	  c = buf[bcount];
	  bcount++;
	}
      else
	{ 
	  bytes_read = fread(buf, 1, m, in_file);
	  seekp = 0;
	  c = buf[0];
	  bcount = 1;
	}

      if (c == EOF || (c == '.' && prev == ' ' && !q) ) {

	
	if (s <= nt.subjectSize) 
	  nt.subject = realloc(nt.subject, s+1 *sizeof(char));
	nt.subject[s] = '\0';
	nt.subjectSize = s;

	if (p <= nt.predicateSize) 
	  nt.predicate = realloc(nt.predicate, p+1 *sizeof(char));
	nt.predicate[p] = '\0';
	nt.predicateSize = p;
		
	if (o <= nt.objectSize)
	  nt.object = realloc(nt.object, o+1 *sizeof(char));
	nt.object[o] = '\0';
	nt.objectSize = o;

	if (c==EOF) {
	  *eof = 1;
	  break;
	} else {
	  while (c!='\n' && c!=EOF) {

	    if (bcount < bytes_read) 
	      {
		c = buf[bcount];
		bcount++;
	      }
	    else
	      { 
		bytes_read = fread(buf, 1, m, in_file);
		c = buf[0];
		bcount = 1;
	      }

	    if (c==EOF || (bytes_read != m)) {
	      *eof = 1;
	      break;
	    }
	    
	    else if (c=='\n') {
	      seekp = 0-(m-bcount);
	      fseek(in_file, seekp, SEEK_CUR);
	      seekp = 0;
	      break;
	    }
	  }	  
	}
	       	
	break;	            
      }
      
    }

  free(buf);
  buf = NULL;
  
  return nt;
}
