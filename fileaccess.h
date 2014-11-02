
struct ioargs {
  char *in_file;
  char *out_file;
  struct chunks *chunk;
};

struct mark {
  int start;
  int end;
};

struct indexes {
  struct access *index;
  struct line_positions *lp;
};

struct line_positions {
  int have;
  int size;
  struct mark *markers;
};

struct chunks {
  int start;
  int end;
  int size;
  int curpos;
  struct line_positions *lp;
  struct access *index;
};


int is_gzip(char *name);

char *get_next_line(char **input_buffer, int *consumed, int *pos, 
		    FILE *in_file, struct ioargs *args);

void *get_more_input(char **buffer, 
		     int *len, 
		     FILE *in_file, 
		     struct ioargs *args);

void *build_indexes(char *filename, 
		    struct indexes **indexes, 
		    struct chunks **chunks,
		    int threads);

void *find_line_byte_positions(char *buf, 
			       int begin,
			       int *start,
			       int *end,
			       int len, 
			       struct line_positions **lp);

void *determine_chunks(struct line_positions *lp, 
		       struct chunks **chunks,
		       int threads);
