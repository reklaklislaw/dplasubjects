
struct ntriple {
  char *subject;
  char *predicate;
  char *object;
  int subjectSize;
  int predicateSize;
  int objectSize;
};


struct item {
  char *id;
  char *subject;
  char *object;
};

struct bucket {
  struct item *items;
  int size;
  int free;
};

void free_buckets(struct bucket *buckets, int count, int size);

void *determine_hash_requirements(struct ioargs *args,
				  char *predicate, 
				  int *bucket_count,
				  int *bucket_size);

void *parse(struct ioargs *args, 
	    char *predicate,
	    int bucket_count, 
	    int bucket_size);

void write_json(FILE *out_file, 
		struct bucket *buckets, 
		int32_t *hashes, 
		int hash_count,
		int32_t *collisions,
		struct bucket *overflow,
		int overflow_count, 
		int id_count);

int has_entry(struct bucket *bucket, char *id);

void write_entry(FILE *out_file, 
		 struct item *item);

struct ntriple *get_next_ntriple(char *line);

char *get_id(char *subject);

int32_t jch(char *key, int32_t num_buckets);

uint64_t get_ll_id(char *id);

size_t get_hash(char *id, 
		int bucket_size);

void *init_new_entry(struct item *item, 
		     struct ntriple *nt, 
		     char *id);

void alloc_new_entry(struct item *item);

void *add_ntriple_data(struct item *item, 
		       char *predicate,
		       struct ntriple *nt);

int match_tag(char *predicate, 
	      char *tag);
