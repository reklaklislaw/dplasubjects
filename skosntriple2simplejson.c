/* Takes SKOS N-Triple metadata and reformats as simplied JSON (just uri and prefLabel)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "fileaccess.c"
#include "filesearch.c"
#include "skosntriple2simplejson.h"

int main(int argc, char *argv[])
{
  if (argc!=4) 
    {
      fprintf(stderr, "Usage: in_file predicate out_file\n");
      exit(1);
    }
  
  char *in, *out;
  in = malloc(sizeof(char) * (strlen(argv[1]) + 1));
  out = malloc(sizeof(char) * (strlen(argv[3]) + 1));

  strcpy(in, argv[1]);
  strcpy(out, argv[3]);

  char *predicate = malloc(sizeof(char) * strlen(argv[2]) + 1);
  if (!predicate)
    {
      fprintf(stderr, "failed to alloc predicate string\n");
      return 1;
    }
  strcpy(predicate, argv[2]);

  fprintf(stderr, "building indexes for %s...", in);
  int threads = 1, i; 
  struct chunks *chunks = NULL;
  struct indexes *indexes = NULL;
  if (is_gzip(in))
    {
      indexes = get_indexes(in);
      if (!determine_chunks(indexes->lp, &chunks, threads))
	{
	  fprintf(stderr, "failed to determine chunks -- aborting\n");
	  return 1;
	}      

      for (i=0; i<threads; i++)
	{
	  chunks[i].index = indexes->index;
	  chunks[i].index->list = indexes->index->list;
	}
    } 

  else  
    {
      FILE *in_file = fopen(in, "r");
      if (!in_file)
	{
	  fprintf(stderr, "failed to open in_file\n");
	  return 1;
	}

      int read;
      char *buf = malloc(sizeof(char) * CHUNK);
      if (!buf)
	{
	  fprintf(stderr, "failed to alloc buf\n");
	  return 1;
	}
      
      struct line_positions *lp = NULL;
      int start = 0, end = 0;
      while (1)
	{
	  read = fread(buf, 1, CHUNK, in_file);
	  if (read) 
	    {
	      if (!find_line_byte_positions(buf, 0, &start, &end, read, &lp))
		{
		  fprintf(stderr, "error while finding lines byte positions - aborting\n");
		  return 1;
		}
	    } else break;
	}
      
      lp->markers = realloc(lp->markers, sizeof(struct mark) * lp->have);
      lp->size = lp->have;
      fclose(in_file);
      free(buf);

      if (!determine_chunks(lp, &chunks, 1))
	{
	  fprintf(stderr, "failed to determine chunks -- aborting\n");
	  return 1;
	}
      
      free_line_positions(lp);
    }

  fprintf(stderr, "done.\n");  

  fprintf(stderr, "determining hash requirements...\n");

  struct ioargs args;
  args.in_file = in;
  args.out_file = out;
  args.chunk = &chunks[0];
    
  int bucket_count, bucket_size;
  if (!determine_hash_requirements(&args, predicate, &bucket_count, &bucket_size))
    {
      fprintf(stderr, "failed to determine hashing requirements\n");
      return 1;
    }
  if (bucket_count == 0)
    {
      fprintf(stderr, "no such predicate could be found\n");
      return 1;
    }

  fprintf(stderr, "beginning parsing...\n");
  parse(&args, predicate, bucket_count, bucket_size);

  free(predicate);
  free(in);
  free(out);
  
  for (i=0; i<1; i++)
    {
      free_line_positions(chunks[i].lp);
    }
  free(chunks);

  if (indexes) 
    {
      free_index(indexes->index);
      free_line_positions(indexes->lp);
      free(indexes);
    }
  
  return 0;
}


void *determine_hash_requirements(struct ioargs *args, char *predicate, 
				  int *bucket_count, int *bucket_size)
{
  FILE *f = fopen(args->in_file, "r");
  char *line = NULL;
  int consumed = 0;
  char *input_buffer = malloc(sizeof(char) * CHUNK);
  if (!input_buffer)
    {
      fprintf(stderr, "failed to alloc input buffer");
      return NULL;
    }
  int pos = 0;

  struct search_patterns sp;
  sp.pcount = 0;
  sp.match_count = 0;
  add_search_pattern(&sp, NULL, predicate, NULL, "prefLabel");

  int plcount = 0;
  while (1)
    {
      line = get_next_line(&input_buffer, &consumed, &pos, f, args);
      if (!line)
	break;
      
      int count = strlen(line);
      get_match_positions(line, count, &sp.patterns[0]);
      free(sp.patterns[0].matches);
      sp.patterns[0].matches = NULL;

      free(line);
      line = NULL;
    }

  *bucket_size = 100;
  *bucket_count = (sp.patterns[0].total_match_count / *bucket_size) * 2.5;

  fprintf(stderr, "will use %d buckets of size %d\n", *bucket_count, *bucket_size);

  fclose(f);
  free(input_buffer);
  free(sp.patterns[0].string);
  free(sp.patterns[0].name);
  free(sp.patterns[0].type);
  free(sp.patterns);

  args->chunk->curpos = 0; //reset

  return (int *) 1;

}



void *parse(struct ioargs *args, char *predicate,
	   int bucket_count, int bucket_size)
{ 
  
  FILE *in_file = fopen(args->in_file, "r");
  if (!args->in_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", args->in_file);
      return NULL;
    }
  
  int b_size = bucket_count;
  struct bucket *buckets = malloc(b_size * sizeof(struct bucket));
  if (!buckets) 
    {
      fprintf(stderr, "Failed to alloc buckets\n");
      return NULL;
    }
    
  int i;
  for (i=0; i<bucket_count; i++)
    {
      buckets[i].items = malloc(bucket_size * sizeof(struct item));
      if (!buckets[i].items) 
	{
	  fprintf(stderr, "Failed to alloc items\n");
	  return NULL;
	}

      buckets[i].size = bucket_size;
      buckets[i].free = bucket_size;
    }
  
  int overflow_count = 0;
  int overflow_size = bucket_count/10;
  struct bucket *overflow = malloc(sizeof(struct bucket));
  if (!overflow)
    {
      fprintf(stderr, "failed to alloc overflow");
      return NULL;
    }
  overflow->items = malloc(overflow_size * sizeof(struct item));
  if (!overflow->items) 
    {
      fprintf(stderr, "Failed to alloc overflow\n");
      return NULL;
    }

  overflow->size = overflow_size;
  overflow->free = overflow_size;
  
  int id_count = 0;  
  int hash_size = bucket_count/10;
  int32_t *hashes = malloc(hash_size * sizeof(int32_t));
  if (!hashes) 
    {
      fprintf(stderr, "Failed to alloc hashes\n");
      return NULL;
    }

  int entry;
  int hash_count = 0;
  int32_t *collisions = malloc(bucket_count * sizeof(int32_t));
  if (!collisions) 
    {
      fprintf(stderr, "Failed to alloc collisions\n");
      return NULL;
    }

  char *line = NULL;
  int pos = 0, consumed = 0;
  char *input_buffer = malloc(sizeof(char) * CHUNK);
  if (!input_buffer)
    {
      fprintf(stderr, "failed to alloc input buffer\n");
      return NULL;
    }

  struct ntriple *nt;

  while (1) 
    {
      line = get_next_line(&input_buffer, &consumed, &pos, in_file, args);
      if (!line)
	break;
      
      if (hash_count == hash_size)
	{
	  hashes = realloc(hashes, (hash_size+(bucket_count/10)) * sizeof(int32_t));
	  if (!hashes)
	    {
	      fprintf(stderr, "failed to realloc hashes\n");
	      return NULL;
	    }
	  hash_size += bucket_count/10;
	}
      
      nt = get_next_ntriple(line);
      char *id = get_id(nt->subject);
      
      if (id!='\0') 
	{
	  int32_t hash = jch(id, bucket_count);

	  entry = has_entry(&buckets[hash], id);

	  if (buckets[hash].free == buckets[hash].size) 
	    {
	      init_new_entry(&buckets[hash].items[0], nt, id);
	      add_ntriple_data(&buckets[hash].items[0], predicate, nt);
	      buckets[hash].free -= 1;
	      hashes[hash_count] = hash;
	      hash_count += 1;
	      collisions[hash] = 0;
	      id_count += 1;  
	    } else {

	    entry = has_entry(&buckets[hash], id);

	    if (entry != -1) {
	      add_ntriple_data(&buckets[hash].items[entry], predicate, nt);	  
	    }
	    else if (entry == -1)
	      {
		if (buckets[hash].free > 0)
		  {
		    entry = buckets[hash].size - buckets[hash].free;
		    init_new_entry(&buckets[hash].items[entry], nt, id);
		    add_ntriple_data(&buckets[hash].items[entry], predicate, nt);	  
		    buckets[hash].free -= 1;
		    collisions[hash] += 1;
		    id_count += 1;
		  }
		else
		  {
		    fprintf (stderr, "OVERFLOW\n");
		    if (overflow_count == overflow_size) 
		      {
			overflow->items = realloc(overflow->items, 
						   (overflow_size+(bucket_count/10)) * 
						   sizeof(struct item));
			if (!overflow->items)
			  {
			    fprintf(stderr, "failed to realloc overflow\n");
			    return NULL;
			  }
			overflow_size += bucket_count/10;
		      }	  

		    entry = has_entry(overflow, id);
		    if (entry == -1 ) {
		      entry = overflow->size - overflow->free;
		      init_new_entry(&overflow->items[overflow_count], nt, id);
		      overflow->free -= 1;
		      overflow_count += 1;
		      id_count += 1;
		    } else {
		      add_ntriple_data(&overflow->items[entry], predicate, nt);
		      collisions[hash] += 1;
		    }
		  }
	      }
	  }
	}

      free(id);
      free(nt->subject); 
      nt->subject=NULL;
      free(nt->predicate); 
      nt->predicate=NULL;
      free(nt->object); 
      nt->object=NULL;
      free(nt);
      nt = NULL;
      free(line);
      line = NULL;
    }

  FILE *out_file = fopen(args->out_file, "w");
  if (!args->out_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", args->out_file);
      return NULL;
    }

  write_json(out_file, buckets, 
   	     hashes, hash_count, 
   	     collisions, overflow, 
   	     overflow_count, id_count);

  fclose(in_file);
  fclose(out_file);
  free(input_buffer);

  free(hashes);
  free(collisions);
  free_buckets(buckets, bucket_count, bucket_size);
  free_buckets(overflow, overflow_count, overflow_size);
}

void free_buckets(struct bucket *buckets, int count, int size)
{
  if (count == 0)
    {
      free(buckets[0].items);
      free(buckets);
      return;
    }
  
  int i, j;
  for (i=0; i<count; i++)
    {
      for (j=0; j<size; j++)
	{
	  if (j==buckets[i].size-buckets[i].free)
	    break;
	  free(buckets[i].items[j].id);
	  free(buckets[i].items[j].object);
	  free(buckets[i].items[j].subject);
	}
      free(buckets[i].items);
    }
  free(buckets);

}


struct ntriple *get_next_ntriple(char *line)
{
  struct ntriple *nt = malloc(sizeof(struct ntriple));
  if (!nt)
    {
      fprintf(stderr, "failed to alloc ntriple\n");
      return NULL;
    }

  int entry_size = strlen(line);  
  nt->subject = malloc(entry_size * sizeof(char));
  if (!nt->subject)
    {
      fprintf(stderr, "Failed to malloc nt->subject\n");
      return NULL;
    }
  nt->predicate = malloc(entry_size * sizeof(char));
  if (!nt->predicate)
    {
      fprintf(stderr, "Failed to malloc nt->predicate\n");
      return NULL;
    }
  nt->object = malloc(entry_size * sizeof(char));  
  if (!nt->object)
    {
      fprintf(stderr, "Failed to malloc nt->object\n");
      return NULL;
    }
  
  int div_count = 0;
  
  int s=0, p=0, o=0;
  int q = 0; //in quotes

  char c = '\0';
  char prev = '\0';
  int i = 0;
  while(i < entry_size)
    {
      c = line[i];
      
      if (c=='"' && prev!='\\')
	{
	  if (q)
	    q = 0;
	  else
	    q = 1;
	}
      	
      if (c == ' ' && prev == '>')
	div_count += 1;

      if (div_count == 0) {
	nt->subject[s] = c;
	s++;
      }
      
      else if (div_count == 1) {
	if (p==0 && c == ' ') {}
	else 
	  {
	    nt->predicate[p] = c;
	    p++;
	  }	
      }
      
      else if (div_count == 2) {
	if (o == 0 && c == ' ') {}
	else 
	  {
	    nt->object[o] = c;
	    o++;	    
	    if (c=='"' && !q)
	      break;//don't include language suffix  
	  }	
      }
      
      prev = c;
      i++;
    }

  nt->subject[s] = '\0';
  nt->subject = realloc(nt->subject, sizeof(char) * s + 1);  
  nt->subjectSize = s;

  nt->predicate[p] = '\0';
  nt->predicateSize = p;
  nt->predicate = realloc(nt->predicate, sizeof(char) * p + 1);

  nt->object[o] = '\0';
  nt->objectSize = o;
  nt->object = realloc(nt->object, sizeof(char) * o + 1);
  
  return nt;
}



void write_json(FILE *out_file, struct bucket *buckets, 
		int32_t *hashes, int hash_count,
		int32_t *collisions, struct bucket *overflow,
		int overflow_count, int id_count)
{
  fprintf(stderr, "Writing %d (+ %d overflow) entries to file...\n", id_count, overflow_count);
  int i, j;
  int32_t h;
  float hc = 0.0;
  float cc = 0.0;

  fprintf(out_file, "[");
  for (i=0; i<hash_count; i++)
    {
      h = hashes[i];
      hc += 1.0;
      cc += collisions[h];
      int end = buckets[h].size - buckets[h].free;
      for (j=0; j<end; j++) 
	{
	  if (buckets[h].items[j].id == '\0')
	    continue;
	  else {
	    write_entry(out_file, &buckets[h].items[j]);
	    if (i<hash_count-1) 
	      fprintf(out_file, ",\n");
	    else
	      if (j<end-1 || overflow_count > 0)
		fprintf(out_file, ",\n");
	  }
	}      
    }
  
  for (i=0; i<overflow_count; i++)
    {
      if (overflow->items[i].id == '\0')
	continue;
      else {
	write_entry(out_file, &overflow->items[i]);
	if (i != overflow_count-1)
	  fprintf(out_file, ",\n");	    
      }
    }

  fprintf(out_file, "]");	    	  
  
  fprintf(stderr, "unique items: %d\n hash count: %d\n average \
collisions per hash:%f\n overflow count:%d\n", 
	  id_count, hash_count, cc/hc, overflow_count);


}


void write_entry(FILE *out_file, struct item *item)
{
  fprintf(out_file, "{\"object\":%s, \"subject\":\"%s\"}", item->object, item->subject);
}


/*see arxiv.org/pdf/1406.2294v1.pdf*/
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
      fprintf(stderr, "Failed to alloc key\n");
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



void *add_ntriple_data(struct item *item, char *predicate, struct ntriple *nt)
{
  int i, j;
  if (match_tag(nt->predicate, predicate)) 
    {
      if (item->object) {
	fprintf(stderr, "WARNING: skipping %s %s... already has %s:%s\n", 
		item->id, nt->object, predicate, item->object);
      	return (int *) 1;
      } 

      item->object = malloc(sizeof(char) * nt->objectSize + 1);
      if (!item->object) 
	{
	  fprintf(stderr, "Failed to alloc item->object\n");
	  return NULL;
	}
      strcpy(item->object, nt->object);
    }
}


int match_tag(char *predicate, char *tag)
{
  
  char *buf = malloc(20 * sizeof(char));
  if (!buf) 
    {
      fprintf(stderr, "Failed to alloc buf\n");
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


void *init_new_entry(struct item *item, struct ntriple *nt, char *id)
{
  int i, j;
  alloc_new_entry(item);
  
  item->id = malloc(sizeof(char) * strlen(id) + 1);
  if (!item->id)
    {
      fprintf(stderr, "Failed to malloc id\n");
      return NULL;
    }
  strcpy(item->id, id);

  item->subject = malloc(sizeof(char) * nt->subjectSize + 1);    
  if (!item->subject)
    {
      fprintf(stderr, "Failed to malloc subject\n");
      return NULL;
    }

  strcpy(item->subject, nt->subject);

  return (int *) 1;
}

void alloc_new_entry(struct item *item)
{
  item->id = NULL;
  item->subject = NULL;
  item->object = NULL;
}


int has_entry(struct bucket *bucket, char *id)
{
  int i;
  int lim = bucket->size - bucket->free;
  for (i=0; i<lim; i++)
    {
      if (bucket->items[i].id != '\0') {	
	if (strcmp(bucket->items[i].id, id) == 0) {
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
  int len = strlen(subject);
  char *buf = malloc(sizeof(char) * len + 1);
  if (!buf)
    {
      fprintf(stderr, "Failed to malloc buf\n");
      exit(1);
    }
  int i = 0;
  int j = 0;
  int k = 0;
  char c;  
  while (i < len)
    {
      c = subject[i];
      if (c != '>')
	i++;
      else
	break;
    }
  
  int p = i;

  while (i < len)
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

  char *id = malloc(sizeof(char) * j + 1);
  if (!id)
    {
      fprintf(stderr, "Failed to malloc id\n");
      return NULL;
    }
  for (k=0, p=p-(j-1); k<j-1; k++, p++) {
    id[k] = buf[p];
  }
  
  id[k] = '\0';

  free(buf);
  buf=NULL;

  return id;
}



