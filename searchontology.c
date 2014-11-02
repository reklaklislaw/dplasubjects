#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "fileaccess.c"
#include "filesearch.c"
#include "searchontology.h"

int main(int argc, char *argv[]) 
{
  if (argc!=4)
    {
      fprintf(stderr, "Usage: search_material ontology out_basename\n");
      exit(0);
    }

  char *in, *ont, *outbase;
  in = malloc(sizeof(char) * strlen(argv[1]) + 1);
  if (!in)
    {
      fprintf(stderr, "failed to alloc in\n");
      return 1;
    }
  strcpy(in, argv[1]);

  ont = malloc(sizeof(char) * strlen(argv[2]) + 1);
  if (!ont)
    {
      fprintf(stderr, "failed to alloc ont\n");
      return 1;
    }
  strcpy(ont, argv[2]);

  int outlen = strlen(argv[3]) + 1;
  outbase = malloc(sizeof(char) * outlen);
  if (!outbase)
    {
      fprintf(stderr, "failed to alloc outbase\n");
      return 1;
    }
  strcpy(outbase, argv[3]);

  char **out_files = malloc(sizeof(char *) * NUMCORES);
  if (!out_files)
    {
      fprintf(stderr, "failed to alloc out files\n");
      return 1;
    }

  struct indexes *input_indexes = NULL;
  struct indexes *ontol_indexes = NULL;
  struct chunks *input_chunks = NULL;
  struct chunks *ontol_chunks = NULL;
  
  fprintf(stderr, "building indexes for %s...", in);
  if (!build_indexes(in, &input_indexes, &input_chunks, -1))
    {
      fprintf(stderr, "failed.\n");
      return 1;
    }
  
  fprintf(stderr, "\nbuilding indexes for %s...", ont);
  if (!build_indexes(ont, &ontol_indexes, &ontol_chunks, 1))
    {
      fprintf(stderr, "failed.\n");
      return 1;
    }
  fprintf(stderr, "done.\n"); 

  fprintf(stderr, "allocating cache...");
  CACHED = malloc(sizeof(struct ontologyitem) * ontol_chunks[0].lp->have);
  if (!CACHED)
    {
      fprintf(stderr, "failed to alloc CACHED\n");
      return 1;
    }
  fprintf(stderr, "done.\n");

  fprintf(stderr, "loading ontology..."); 
  if (!load_ontology(ont, &ontol_chunks[0]))
    {
      fprintf(stderr, "failed to load ontology\n");
      return 1;
    }
  fprintf(stderr, "done.\n");

  free_line_positions(ontol_chunks[0].lp);
  free(ontol_chunks);
  if (ontol_indexes) 
    {
      free_index(ontol_indexes->index);
      free(ontol_indexes);
    }

  pthread_t *threads = malloc(sizeof(pthread_t) * NUMCORES);
  if (!threads)
    {
      fprintf(stderr,"failed to alloc pthreads\n");
      return 1;
    }

  int *pt_ret = malloc(sizeof(int) * NUMCORES);
  if (!pt_ret)
    {
      fprintf(stderr, "failed to alloc pt_ret\n");
      return 1;
    }
  
  struct ioargs **args = malloc(sizeof(struct ioargs *) * NUMCORES);
  if (!args)
    {
      fprintf(stderr, "failed to allocate args\n");
      return 1;
    }

  char corestr[3];
  int i;
  for (i=0; i<NUMCORES; i++)
    {
      sprintf(corestr, "%d", i);
      out_files[i] = malloc(sizeof(char) * (outlen + strlen(corestr) + 1));
      if (!out_files[i])
	{
	  fprintf(stderr, "failed to alloc out file");
	  return 1;
	}

      strcpy(out_files[i], outbase);
      strcat(out_files[i], corestr);

      args[i] = malloc(sizeof(struct ioargs));
      args[i]->in_file = in;
      args[i]->out_file = out_files[i];
      args[i]->chunk = &input_chunks[i];

      fprintf(stderr, "creating new thread[%d]\n", i);
      pt_ret[i] = pthread_create(&threads[i], NULL, search, (void *) args[i]);      
    }
  
  for (i=0; i<NUMCORES; i++) 
    {
      pthread_join(threads[i], NULL);
      free(args[i]);
      free(out_files[i]);
      free_line_positions(input_chunks[i].lp);
    }

  printf("exact:%d  partial:%d  none:%d\n", EXACT, PARTIAL, NONE);

  if (input_indexes) 
    {
      free_index(input_indexes->index);
      free_line_positions(input_indexes->lp);
      free(input_indexes);
    }

  free(threads);
  free(out_files);
  free(args);
  free(input_chunks);
  free(pt_ret);
  free_ontology();
  free(in);
  free(ont);
  free(outbase);

  return 0;
}

void *search(void *ptr)
{
  struct ioargs *args;
  args = (struct ioargs *) ptr;

  FILE *in_file = fopen(args->in_file, "r");
  if (!in_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", args->in_file);
      return NULL;
    }

  FILE *out_file = fopen(args->out_file, "w");
  if (!out_file)
    {
      fprintf(stderr, "failed to open %s for writing\n", args->out_file);
      return NULL;
    }

  int pos = 0, consumed = 0;
  char *line = NULL;
  char *input_buffer = malloc(sizeof(char) * CHUNK);
  if (!input_buffer)
    {
      fprintf(stderr, "failed to alloc input_buffer\n");
      return NULL;
    }

  struct inputitem *inputitem;  

  fprintf(out_file, "[");
  args->chunk->curpos = 0;
  while (1)
    {
      line = get_next_line(&input_buffer, &consumed, &pos, in_file, args);
      if (!line)
	break;
     
      inputitem = get_input_item(line);
      if (!inputitem)
	{
	  fprintf(stderr, "failed to get input item\n");
	  return NULL;
	} 
      else 
	{
	  if (inputitem->field)
	    {	      
	      match_in_ontology(inputitem);
	  
	      fprintf(out_file, "{\"field\": %s, \"ids\": [", inputitem->field);
	      int i;
	      for (i=0; i<inputitem->id_count; i++)
		{
		  fprintf(out_file, "%s,", inputitem->ids[i]);
		}
	      fseek(out_file, -1, SEEK_END);
	      fprintf(out_file, "]");
	      if (inputitem->exact)
		{
		  fprintf(out_file, ", \"matches\": {\"exact\": {\"object\":%s, \"subject\":%s}}},\n", 
			  inputitem->exact->object,
			  inputitem->exact->subject);
		}
	      else if (inputitem->partial_count > 0) 
		{
		  fprintf(out_file, ", \"matches\": {\"partial\": [");	      
		  int i;
		  for (i=0; i<inputitem->partial_count; i++)
		    {
		      fprintf(out_file, "{\"object\": %s, \"subject\": %s}, ", 
			      inputitem->partial[i]->object, 
			      inputitem->partial[i]->subject);
		    }
		  fseek(out_file, -1, SEEK_END);
		  fprintf(out_file, "]}},\n");
		}
	      else
		fprintf(out_file, ", \"matches\": Null}},\n", inputitem->field);	     
	    }
	}

      free_inputitem(inputitem);
      inputitem = NULL;

      free(line);
      line = NULL;
    }
  
  fclose(in_file);
  fclose(out_file);
  free(input_buffer);
  return (int *) 1;
}


void free_inputitem(struct inputitem *inputitem)
{
  int i;
  if (inputitem->field)
    free(inputitem->field);
  if (inputitem->ids)
    {
      for (i=0; i<inputitem->id_count; i++)
	if (inputitem->ids[i])
	  free(inputitem->ids[i]);
      free(inputitem->ids);
    }
  
  if (inputitem->exact)
    free(inputitem->exact);
  
  if (inputitem->partial)
    {
      for (i=0; i<inputitem->partial_count; i++)
	if (inputitem->partial[i])
	  free(inputitem->partial[i]);
      free(inputitem->partial);
    }

  free(inputitem);
}


struct inputitem *get_input_item(char *line)
{
  struct inputitem *inputitem = malloc(sizeof(struct inputitem));
  if (!inputitem)
    {
      fprintf(stderr, "failed to alloc inputitem\n");
      return NULL;
    }

  inputitem->field = NULL;
  inputitem->ids = NULL;
  inputitem->exact = NULL;
  inputitem->partial = NULL;
  inputitem->partial_count = 0;

  int len = strlen(line);
  int j = 0, i = 1;

  struct search_patterns sp;
  sp.pcount = 0;
  sp.match_count = 0;

  add_search_pattern(&sp, "\"", "field", "\": \"", "field");
  add_search_pattern(&sp, "\"", "identifiers", "\": [", "identifiers");
  
  int p, m = 0;
  for (p=0; p<sp.pcount; p++) 
    {
      get_match_positions(line, len, &sp.patterns[p]);	  
      if (sp.patterns[p].match_count > 0)
	{
	  get_matches(line, len, &sp.patterns[p]);
	  for (m=0; m<sp.patterns[p].match_count; m++) 
	    {
	      if (strcmp(sp.patterns[p].name, "field")==0)
		{
		  inputitem->field = \
		    malloc(sizeof(char) * strlen(sp.patterns[p].matches[m].string) + 1);
		  if (!inputitem->field)
		    {
		      fprintf(stderr, "failed to alloc inputitem field\n");
		      return NULL;
		    }
		  strcpy(inputitem->field, sp.patterns[p].matches[m].string);
		}
	      else if (strcmp(sp.patterns[p].name, "identifiers")==0)
		{
		  int num = string_to_list(sp.patterns[p].matches[m].string, &inputitem->ids);
		  inputitem->id_count = num;
		}
	      
	      free(sp.patterns[p].matches[m].string);
	    }
	 
	  
	}

      free(sp.patterns[p].matches);
      free(sp.patterns[p].string);
      free(sp.patterns[p].name);
      free(sp.patterns[p].type);
    }
  
  free(sp.patterns);

  return inputitem;
}


int free_ontology(void)
{
  int i, j;
  for (i=0; i<CACHE_COUNT; i++)
    {
      free(CACHED[i].object);
      free(CACHED[i].subject);
      if (CACHED[i].token_count > 0)
	{
	  for (j=0; j<CACHED[i].token_count; j++)
	    free(CACHED[i].tokens[j]);
	  free(CACHED[i].tokens);
	}
    }

  free(CACHED);
  return 1;
}

void *load_ontology(char *ont, struct chunks *ontol_chunks)
{
  
  FILE *ont_file = fopen(ont, "r");
  if (!ont_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", ont);
      return NULL;
    }

  int pos = 0, consumed = 0;
  char *line = NULL;
  char *input_buffer = malloc(sizeof(char) * CHUNK);
  if (!input_buffer)
    {
      fprintf(stderr, "failed to alloc input_buffer\n");
      return NULL;
    }

  struct ioargs args;
  args.in_file = ont;
  args.chunk = ontol_chunks;
  args.chunk->curpos = 0;

  while (1)
    {
      line = get_next_line(&input_buffer, &consumed, &pos, ont_file, &args);
      if (!line)
	break;

      if (!get_ontology_item(line))
	{
	  fprintf(stderr, "failed to get_input_item\n");
	  return NULL;
	} else CACHE_COUNT += 1;

   	  
      free(line);
      line = NULL;
    }


  fclose(ont_file);
  free(input_buffer);


  return (int *) 1;
}


void *get_ontology_item(char *line)
{
  int len = strlen(line);
  int j = 0, i = 1;
  struct search_patterns sp;
  sp.pcount = 0;
  sp.match_count = 0;

  add_search_pattern(&sp, "\"", "object", "\": \"", "object");
  add_search_pattern(&sp, "\"", "subject", "\": \"", "subject");
  add_search_pattern(&sp, "\"", "tokens", "\": [", "tokens");
  
  int p, m = 0;
  for (p=0; p<sp.pcount; p++) 
    {
      get_match_positions(line, len, &sp.patterns[p]);	  
      if (sp.patterns[p].match_count > 0)
	{
	  get_matches(line, len, &sp.patterns[p]);
	  for (m=0; m<sp.patterns[p].match_count; m++) 
	    {
	      if (strcmp(sp.patterns[p].name, "subject")==0)
		{
		  CACHED[CACHE_COUNT].subject = \
		    malloc(sizeof(char) * strlen(sp.patterns[p].matches[m].string) + 1);
		  if (!CACHED[CACHE_COUNT].subject)
		    {
		      fprintf(stderr, "failed to alloc ontology subject\n");
		      return NULL;
		    }
		  strcpy(CACHED[CACHE_COUNT].subject, sp.patterns[p].matches[m].string);
		}
	      else if (strcmp(sp.patterns[p].name, "object")==0)
		{
		  CACHED[CACHE_COUNT].object = \
		    malloc(sizeof(char) * strlen(sp.patterns[p].matches[m].string) + 1);
		  if (!CACHED[CACHE_COUNT].object)
		    {
		      fprintf(stderr, "failed to alloc ontology object\n");
		      return NULL;
		    }
		  strcpy(CACHED[CACHE_COUNT].object, sp.patterns[p].matches[m].string);
		}
	      
	      else if (strcmp(sp.patterns[p].name, "tokens")==0)
		{
		  int tnum = string_to_list(sp.patterns[p].matches[m].string, &CACHED[CACHE_COUNT].tokens);
		  CACHED[CACHE_COUNT].token_count = tnum;	     
		}
	      
	      free(sp.patterns[p].matches[m].string);
	    }
	 
	  free(sp.patterns[p].matches);
	  free(sp.patterns[p].string);
	  free(sp.patterns[p].name);
	  free(sp.patterns[p].type);
	}
    }

  free(sp.patterns);

  return (int *) 1;
}



void *match_in_ontology(struct inputitem *inputitem)
{
  int i, k;
  for (k=0; k<CACHE_COUNT; k++)
    {
      if (strcasecmp(CACHED[k].object, inputitem->field) == 0)
	{
	  inputitem->exact = malloc(sizeof(struct ontologyitem));
	  if (!inputitem->exact)
	    {
	      fprintf(stderr, "failed to alloc inputitem exact match\n");
	      return NULL;
	    }
	  inputitem->exact->object = CACHED[k].object;
	  inputitem->exact->subject = CACHED[k].subject;
	  EXACT += 1;
	  break;
	} 
    }
    
  if (!inputitem->exact)
    {
      inputitem->partial_count = 0;
      inputitem->partial_size = 10;
      inputitem->partial = malloc(sizeof(struct ontologyitem *) * 10);
      if (!inputitem->partial)
	{
	  fprintf(stderr, "failed to alloc inputitem exact match\n");
	  return NULL;
	}

      for (k=0; k<CACHE_COUNT; k++)
	{
	  for (i=0; i<CACHED[k].token_count; i++)
	    {	      
	      if (strcasecmp(CACHED[k].tokens[i], inputitem->field) == 0)
		{
		  inputitem->partial[inputitem->partial_count] = malloc(sizeof(struct ontologyitem));
		  if (!inputitem->partial[inputitem->partial_count])
		    {
		      fprintf(stderr, "failed to alloc partial item");
		      return NULL;
		    }
		  inputitem->partial[inputitem->partial_count]->object = CACHED[k].object;
		  inputitem->partial[inputitem->partial_count]->subject = CACHED[k].subject;
		  inputitem->partial_count += 1;

		  if (inputitem->partial_count == inputitem->partial_size)
		    {
		      inputitem->partial = \
			realloc(inputitem->partial, sizeof(struct ontologyitem) * 
				inputitem->partial_size << 1);
		      if (!inputitem->partial)
			{
			  fprintf(stderr, "failed to realloc inputitem's partial matches\n");
			  return NULL;
			}
		      inputitem->partial_size <<= 1;
		    }

		} 
	    }	 
	}      

      if (inputitem->partial_count > 0)
	{
	  //resize
	  inputitem->partial = \
	    realloc(inputitem->partial, sizeof(struct ontologyitem) * 
		    inputitem->partial_count);
	  if (!inputitem->partial)
	    {
	      fprintf(stderr, "failed to resize inputitem's partial matches\n");
	      return NULL;
	    }
	  inputitem->partial_size = inputitem->partial_count;
	  PARTIAL += 1;
	} else NONE += 1;
    }


  return (int *) 1;
}

