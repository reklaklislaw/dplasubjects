
struct ntriple {
  char *subject;
  char *predicate;
  char *object;
  int subjectSize;
  int predicateSize;
  int objectSize;
};


struct subjectObject {
  char **altLabel;
  char **narrower;
  char **broader;
  char **closeMatch;  
  char **related;
  char *id;
  char *url;
  char *prefLabel;
  int altCount;
  int altSize;
  int narrowerCount;
  int narrowerSize;
  int broaderCount;
  int broaderSize;
  int closeMatchCount;
  int closeMatchSize;
  int relatedCount;
  int relatedSize;
};

struct bucket {
  struct subjectObject *subjObj;
  int size;
  int free;
};


void parse(FILE *in_file, FILE *out_file, int bucket_count, int bucket_size);
struct ntriple get_next_ntriple(FILE *in_file, int *eof);
char *get_id(char *subject);
size_t get_hash(char *id, int bucket_size);
void init_new_entry(struct subjectObject *subjObj, struct ntriple nt, char *id);
void alloc_new_entry(struct subjectObject *subjObj);
void add_ntriple_data(struct subjectObject *subjObj, struct ntriple nt);
int match_tag(char *predicate, char *tag);
