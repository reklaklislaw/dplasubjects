
struct inputitem {
  char *field;
  char **ids;
  int id_count;
  int ids_size;
  struct ontologyitem *exact;
  struct ontologyitem **partial;
  int partial_count;
  int partial_size;
};

struct ontologyitem {
  char *subject;
  char *object;
  char **tokens;
  int token_count;
};

int EXACT = 0;
int PARTIAL = 0;
int NONE = 0;

int CACHE_COUNT = 0;
struct ontologyitem *CACHED = NULL;

struct searchargs {
  char *in_file;
  struct chunks *input_chunks;
  char *ont_file;
  struct chunks *ontol_chunks;
  
};

void *search(void *ptr);

void free_inputitem(struct inputitem *inputitem);

struct inputitem *get_input_item(char *line);

void *load_ontology(char *ont_file, struct chunks *ontol_chunks);

void *get_ontology_item(char *line);

void *match_in_ontology(struct inputitem *inputitem);

char **tokenize(char *string);
