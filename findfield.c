/* Takes in JSON metadata, pulls all entries of a field, and writes them to a file as JSON.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "pthread.h"

#include "fileaccess.c"
#include "filesearch.c"
#include "findfield.h"



int main(int argc, char *argv[])
{
  if (argc!=5)
    {
      fprintf(stderr, "Usage: in_file search_field id_field out_basename\n");
      return 1;
    }
  
  char *in, *outbase;
  int in_len = strlen(argv[1]), out_len = strlen(argv[4]);
  
  in = malloc((in_len + 1) * sizeof(char));
  if (!in)
    {
      fprintf(stderr, "failed to alloc 'in'\n");
      return 1;
    }

  out_len += 1;
  outbase = malloc(out_len * sizeof(char));
  if (!outbase) 
    {
      fprintf(stderr, "failed to alloc 'outbase'\n");
      return 1;
    }

  strcpy(in, argv[1]);
  strcpy(outbase, argv[4]);
  
  char *search_field, *id_field;
  int sf_len = strlen(argv[2]), idf_len = strlen(argv[3]);
  search_field = (char *) malloc((sf_len + 1)  * sizeof(char));
  if (!search_field)
    {
      fprintf(stderr, "failed to alloc 'search_field'\n");
      return 1;
    }
  id_field = (char *) malloc((idf_len + 1)  * sizeof(char));
  if (!id_field)
    {
      fprintf(stderr, "failed to alloc 'id_field'\n");
      return 1;
    }

  strcpy(search_field, argv[2]);
  strcpy(id_field, argv[3]);

  fprintf(stderr, "building indexes for %s...", in);
  struct chunks *chunks = NULL;
  struct indexes *indexes = NULL;

  if (!build_indexes(in, &indexes, &chunks, -1))
    {
      fprintf(stderr, "failed to build indexes\n");
      return 1;
    }

  fprintf(stderr, "done.\n");  
  
  char **out_files = malloc(sizeof(char *) * NUMCORES);
  if (!out_files)
    {
      fprintf(stderr, "failed to alloc out files\n");
      return 1;
    }

  pthread_t *threads = malloc(sizeof(pthread_t) * NUMCORES);
  if (!threads)
    {
      fprintf(stderr, "failed to alloc threads\n");
      return 1;
    }
  int *pt_ret = malloc(sizeof(int) * NUMCORES);
  if (!pt_ret)
    {
      fprintf(stderr, "failed to alloc pt_ret\n");
      return 1;
    }

  struct find_field_args **args = malloc(sizeof(struct find_field_args *) * NUMCORES);
  if (!args)
    {
      fprintf(stderr, "failed to allocate args\n");
      return 1;
    }

  char corestr[3];
  int i, j;
  for (i=0; i<NUMCORES; i++)
    {
      sprintf(corestr, "%d", i);
      out_files[i] = malloc(sizeof(char) * (out_len + strlen(corestr) + 1));
      if (!out_files[i])
	{
	  fprintf(stderr, "failed to alloc out file");
	  return 1;
	}

      strcpy(out_files[i], outbase);
      strcat(out_files[i], corestr);

      args[i] = malloc(sizeof(struct find_field_args));
      args[i]->ioargs = malloc(sizeof(struct ioargs));
      args[i]->ioargs->in_file = in;
      args[i]->ioargs->out_file = out_files[i];
      args[i]->ioargs->chunk = &chunks[i];
      args[i]->search_field = search_field;
      args[i]->id_field = id_field;

      int mb = args[i]->ioargs->chunk->size / (1024*1024);
      fprintf(stderr, "creating new thread[%d] to process %dMB of data\n", i, mb);
      pt_ret[i] = pthread_create(&threads[i], NULL, find_field, (void *) args[i]);
    }
  
  for (i=0; i<NUMCORES; i++) 
    {
      pthread_join(threads[i], NULL);
      fprintf(stderr, "thread[%d] returned with status %d\n", i, pt_ret[i]);
      free(out_files[i]);
      free(args[i]->ioargs);
      free(args[i]);
      free_line_positions(chunks[i].lp);
    }

  if (indexes) 
    {
      free_index(indexes->index);
      free_line_positions(indexes->lp);
      free(indexes);
    }

  free(chunks);
  free(out_files);
  free(args);
  free(in);
  free(outbase);
  free(search_field);
  free(id_field);
  free(pt_ret);
  free(threads);

  return 0; 
}

struct search_patterns get_search_patterns(char *search_field, char *id_field)
{
  struct search_patterns sp;
  sp.pcount = 0;
  sp.match_count = 0;

  add_search_pattern(&sp, "\"", id_field, "\": \"", "id"); 
  add_search_pattern(&sp, "\"", search_field, "\": \"", "string"); 
  add_search_pattern(&sp, "\"", search_field, "\": [", "list");
  add_search_pattern(&sp, "\"", search_field, "\": {", "dict");
  
  return sp;
}


void *find_field(void *ptr)
{

  struct find_field_args *args;
  args = (struct find_field_args *) ptr;

  FILE *in_file = fopen(args->ioargs->in_file, "r");
  if (!in_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", args->ioargs->in_file);
      return NULL;
    }

  fprintf(stderr, "opening %s for writing...\n", args->ioargs->out_file);
  
  FILE *out_file = fopen(args->ioargs->out_file, "w");
  if (!out_file)
    {
      fprintf(stderr, "failed to open %s for reading\n", args->ioargs->out_file);
      return NULL;
    }

  struct search_patterns *sp = malloc(sizeof(struct search_patterns));
  *sp = get_search_patterns(args->search_field, args->id_field);
  
  int i, pos = 0, consumed = 0;
  char *line = NULL;
  char *input_buffer = malloc(sizeof(char) * CHUNK);
  if (!input_buffer)
    {
      fprintf(stderr, "failed to alloc input buffer\n");
      return NULL;
    }

  fprintf(out_file, "%s", "[");  
  while(1)
    {

      line = get_next_line(&input_buffer, &consumed, &pos, in_file, args->ioargs);
      if (!line)
	break;

      int count = strlen(line);
      int p, m = 0;
      for (p=0; p<sp->pcount; p++) 
	{
	  get_match_positions(line, count, &sp->patterns[p]);	  
	  if (sp->patterns[p].match_count > 0 )
	    {
	      if (strcmp(sp->patterns[p].type, "id")!=0) 
		{
		  m += sp->patterns[p].match_count;
		  sp->match_count += sp->patterns[p].match_count;
		}
	      
	      get_matches(line, count, &sp->patterns[p]);
	    }
	}             

      if (m > 0)
	fprintf(out_file, "{");

      int l;
      for (p=0; p<sp->pcount; p++) 
	{
	  if (strcmp(sp->patterns[p].type, "id")==0 && m > 0) 
	    {
	      fprintf(out_file, "\"identifier\":");
	      for (l=0; l<sp->patterns[p].match_count; l++)
		{
		  if (sp->patterns[p].matches[l].depth == 1)
		    fprintf(out_file, "%s,", sp->patterns[p].matches[l].string);
		}
	    }
	}

      if (m>0)
	fprintf(out_file, "\"fields\":[");
      
      int t = 1;
      for (p=0; p<sp->pcount; p++) 
	{
	  if (strcmp(sp->patterns[p].type, "id")!=0 && m > 0) 
	    {	
	      for (l=0; l<sp->patterns[p].match_count; l++)
		if (sp->patterns[p].matches[l].string) 
		  {
		    fprintf(out_file, "%s", sp->patterns[p].matches[l].string);
		    if (t==m)
		      fprintf(out_file, "]");
		    else
		      fprintf(out_file, ",");
		    t++;
		  } 
	    }
	  
	  for (l=0; l<sp->patterns[p].match_count; l++)
	    if (sp->patterns[p].matches[l].string)
	      {
		free(sp->patterns[p].matches[l].string);
		sp->patterns[p].matches[l].string = NULL;
	      }
	  free(sp->patterns[p].matches);
	  sp->patterns[p].matches = NULL;
	}

      if (m > 0)
	fprintf(out_file, "},\n");

      free(line);
      line = NULL;
    }	
  
  fseek(out_file, -2, SEEK_END);
  fprintf(out_file, "%s", "]");
  
  fclose(in_file);
  fclose(out_file);

  for (i=0; i<sp->pcount; i++)
    {
      if (strcmp(sp->patterns[i].type, "id")!=0)
	fprintf(stderr,"%s matches: %d\n", sp->patterns[i].type, sp->patterns[i].total_match_count);
      free(sp->patterns[i].string);
      free(sp->patterns[i].name);
      free(sp->patterns[i].type);
    }

  fprintf(stderr, "total matches: %d\n\n", sp->match_count);
  free(sp->patterns);
  free(sp);
  free(input_buffer);
  
}
