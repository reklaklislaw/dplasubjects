
struct ntriple {
  char *subject;
  int subjectSize;
  char *predicate;
  int predicateSize;
  char *object;
  int objectSize;
};


struct subjectObject {
  char *id;
  char *url;
  char *prefLabel;
  char **altLabel;
  int altCount;
  int altSize;
  char **narrower;
  int narrowerCount;
  int narrowerSize;
  char **broader;
  int broaderCount;
  int broaderSize;
  char **closeMatch;
  int closeMatchCount;
  int closeMatchSize;
  char **related;
  int relatedCount;
  int relatedSize;
};



void parse(FILE *in_file, FILE *out_file);
struct ntriple get_next_ntriple(FILE *in_file, int *eof);
char *get_id(char *subject);
void alloc_new_entry(struct subjectObject *subjObj);
void add_ntriple_data(struct subjectObject *subjObj, struct ntriple nt);
int match_tag(char *predicate, char *tag);
