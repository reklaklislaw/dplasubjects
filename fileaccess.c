#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileaccess.h"
#include "zran.c"


#define NUMCORES sysconf( _SC_NPROCESSORS_ONLN )

int is_gzip(char *name)
{
  int len = strlen(name);
  if (name[len-3] == '.' && 
      name[len-2] == 'g' && 
      name[len-1] == 'z')
    return 1;
  else
    return 0;
}


char *get_next_line(char **input_buffer, int *consumed, int *pos,
		    FILE *in_file, struct ioargs *args)
{
  int lsize = 1000;
  char *line = malloc(sizeof(char)*lsize);
  if (!line)
    {
      fprintf(stderr, "Failed to malloc line buffer\n");
      return NULL;
    }
  
  if (*consumed==0) 
    {
      *pos = 0;
      if (!get_more_input(input_buffer, consumed, in_file, args)) 
	{
	  free(line);
	  return NULL;
	}
    }
  
  char c = '\0';
  int count = 0;
  while(c != '\n')
    {
      c = (*input_buffer)[*pos];
      *pos += 1;
      *consumed -= 1;	  
      line[count] = c;
      count++;
      if (count == lsize)
	{
	  line = realloc(line, sizeof(char)*(lsize+1000));
	  if (!line)
	    {
	      fprintf(stderr, "failed to increase line buffer\n");
	    }
	  lsize += 1000;
	}
      
      if (*consumed==0) 
	{
	  *pos = 0;
	  if (!get_more_input(input_buffer, consumed, in_file, args))
	    break;
	}
    }
  
  line[count] = '\0';
  return line;
}


void *get_more_input(char **buffer, int *len, FILE *in_file, struct ioargs *args)
{
  if (args->chunk->curpos >= args->chunk->lp->have)
    return NULL;
  if (args->chunk->lp->markers[args->chunk->curpos].start >= args->chunk->end)
    return NULL;
  int read, start, end, bytes = 0, lim = CHUNK*0.90, i;
  start = args->chunk->lp->markers[args->chunk->curpos].start;
  for (i=args->chunk->curpos; i<args->chunk->lp->size; i++)
    {
      end = args->chunk->lp->markers[i].end + 1; //read to newline char
      bytes = end - start;
      args->chunk->curpos += 1; 
      if (bytes >= lim || end >= args->chunk->end)
	{
	  if (bytes >= CHUNK) 
	    {
	      (*buffer) = realloc((*buffer), sizeof(char) * bytes + 1);
	      if (!(*buffer))
		{
		  fprintf(stderr, "failed to resize input buffer\n");
		  return NULL;
		}
	    }
	  if (is_gzip(args->in_file))
	    {
	      read = extract(in_file, 
			     args->chunk->index, 
			     start, 
			     (*buffer), 
			     bytes);
	      if (read < 0)
		fprintf(stderr, "zran: extraction failed: %s error\n",
			read == Z_MEM_ERROR ? "out of memory" : "input corrupted");	      
	    } 
	  
	  else 
	    {
	      fseek(in_file, start, SEEK_SET);
	      read = fread((*buffer), 1, bytes, in_file);	  	      
	    }
    
	  *len = read;
	  if (read <= 0) {
	    (*buffer)[0] = '\0';
	    return NULL;
	  }
	  else {
	    (*buffer)[read] = '\0';
	    break;
	  }
	}
    }
    
  return (int *) 1;
}


void *build_indexes(char *filename, 
		    struct indexes **indexes, 
		    struct chunks **chunks,
		    int threads)
{
  if (threads < 0)
    threads = NUMCORES;
  if (threads == 0)
    threads = 1;

  if (is_gzip(filename))
    {
      (*indexes) = get_indexes(filename);
      if (!determine_chunks((*indexes)->lp, chunks, threads))
	{
	  fprintf(stderr, "failed to determine chunks -- aborting\n");
	  return NULL;
	}      
      int i;
      for (i=0; i<threads; i++)
	{
	  (*chunks)[i].index = (*indexes)->index;
	  (*chunks)[i].index->list = (*indexes)->index->list;
	}
    } 
  
  else
    {

      FILE *in_file = fopen(filename, "r");
      if (!in_file)
	{
	  fprintf(stderr, "failed to open %s\n", filename);
	  return NULL;
	}

      int read;
      char *buf = malloc(sizeof(char) * CHUNK);
      if (!buf)
	{
	  fprintf(stderr, "failed to alloc buf\n");
	  return NULL;
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
		  return NULL;
		}
	    } else break;
	}
      
      lp->markers = realloc(lp->markers, sizeof(struct mark) * lp->have);
      if (!lp->markers)
	{
	  fprintf(stderr, "failed to realloc markers\n");
	  return NULL;
	}
      lp->size = lp->have;
      fclose(in_file);
      free(buf);
      
      if (!determine_chunks(lp, chunks, threads))
	{
	  fprintf(stderr, "failed to determine chunks -- aborting\n");
	  return NULL;
	}

      free_line_positions(lp);
    }

  return (int *) 1;
}


void* find_line_byte_positions(char *buf, int begin, int *start, int *end, 
			       int len, struct line_positions **lp)
{
  //init struct
  if ((*lp) == NULL)
    {
      (*lp) = malloc(sizeof(struct line_positions));
      (*lp)->have = 0;
      (*lp)->size = 100;
      (*lp)->markers = malloc(sizeof(struct mark) * (*lp)->size);
      if (!(*lp)->markers)
	{
	  fprintf(stderr, "Failed to allocate line markers");
	  return NULL;
	}
      (*lp)->markers[(*lp)->have].start = -1;
      (*lp)->markers[(*lp)->have].end = -1;
    }

  if (begin=='\0' || begin < 0)
    begin = 0;
  
  char c;
  int j = begin, i = 0;
  while (i < len)
      {
	c = buf[j];
	if (c == '\n')
	  {
	    //printf("%d %d\n", *start, *end);
	    (*lp)->markers[(*lp)->have].start = *start;
	    (*lp)->markers[(*lp)->have].end = *end;
	    (*lp)->have += 1;
	    if ((*lp)->have == (*lp)->size)
	      {
		(*lp)->size <<= 1;
		(*lp)->markers = realloc((*lp)->markers, sizeof(struct mark) * (*lp)->size);
		if (!(*lp)->markers)
		  {
		    fprintf(stderr, "Failed to malloc more line markers\n");
		    return NULL;
		  }	  		
	      }	    
	    
	    (*lp)->markers[(*lp)->have].start = -1;
	    (*lp)->markers[(*lp)->have].end = -1;
	    
	    *start = *end + 1;
	  }
	
	*end +=1;
	i++;
	j++;
      }

  return (int *) 1;
}
  
void *determine_chunks(struct line_positions *lp, struct chunks **chunks, int threads)
{
  
  if (threads < 0)
    threads = NUMCORES;
  if (threads == 0)
    threads = 1;
  
  int i, j;
  if ((*chunks) == NULL)
    {
      (*chunks) = malloc(sizeof(struct chunks) * threads);
      if (!(*chunks))
	{
	  fprintf(stderr, "failed to alloc chunks\n");
	  return NULL;
	}
  
      for (i=0; i<threads; i++)
	{
	  (*chunks)[i].start = -1;
	  (*chunks)[i].end = -1;
	  (*chunks)[i].size = 0;
	  (*chunks)[i].curpos = 0;
	  (*chunks)[i].lp = malloc(sizeof(struct line_positions));
	  if (!(*chunks)[i].lp)
	    {
	      fprintf(stderr, "failed to alloc chunk's line positions\n");
	      return NULL;
	    }
	  (*chunks)[i].lp->have = 0;
	  (*chunks)[i].lp->size = 1000;
	  (*chunks)[i].lp->markers = malloc(sizeof(struct mark) * (*chunks)[i].lp->size);
	  if (!(*chunks)[i].lp->markers)
	    {
	      fprintf(stderr, "failed to alloc chunk's markers\n");
	      return NULL;
	    }
	  (*chunks)[i].lp->markers[(*chunks)[i].lp->have].start = -1;
	  (*chunks)[i].lp->markers[(*chunks)[i].lp->have].end = -1;
	  (*chunks)[i].index = NULL;
	}
    }
  
  int final_byte = lp->markers[lp->have-1].end;
  int approx_bytes_per_chunk = final_byte / threads;
  i = 0;
  for (j=0; j<lp->size; j++) 
    {

      (*chunks)[i].lp->markers[(*chunks)[i].lp->have] = lp->markers[j];
      (*chunks)[i].lp->have += 1;

      if ((*chunks)[i].lp->have == (*chunks)[i].lp->size)
	{
	  (*chunks)[i].lp->markers = realloc((*chunks)[i].lp->markers,
					     sizeof(struct mark) * (*chunks)[i].lp->size<<1);
	  if (!(*chunks)[i].lp->markers)
	    {
	      fprintf(stderr, "failed to realloc chunk's line markers\n");
	      return NULL;
	    }
	  (*chunks)[i].lp->size <<= 1;
	}

      if ((*chunks)[i].start == -1) 
	{
	  (*chunks)[i].start = lp->markers[j].start;
	}

      if (i==threads-1)
	{
	  (*chunks)[i].end = lp->markers[j].end;
	  (*chunks)[i].size += lp->markers[j].end - lp->markers[j].start;
	} 
      else {      
	if ((*chunks)[i].size >= approx_bytes_per_chunk)
	  {
	    (*chunks)[i].end = lp->markers[j].end;
	    (*chunks)[i].size += lp->markers[j].end - lp->markers[j].start;

	    //resize
	    (*chunks)[i].lp->markers = realloc((*chunks)[i].lp->markers,
					       sizeof(struct mark) * (*chunks)[i].lp->have);
	    if (!(*chunks)[i].lp->markers)
	      {
		fprintf(stderr, "failed to resize chunk's markers");
		return NULL;
	      }
	    (*chunks)[i].lp->size = (*chunks)[i].lp->have;
	    i++;
	  }
	else
	  (*chunks)[i].size += lp->markers[j].end - lp->markers[j].start;	
      }
    }

  return (int *) 1;
}


