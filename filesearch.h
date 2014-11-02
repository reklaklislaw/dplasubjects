
struct match {
  int pos;
  int depth;
  char *string;
};

struct pattern {
  char *string;
  char *name;
  char *type;
  struct match *matches;
  int match_size;
  int match_count;
  int total_match_count;
};

struct search_patterns {
  struct pattern *patterns;
  int pcount;
  int match_count;
};


void *get_matches(char *string, 
		  int stringlen, 
		  struct pattern *pattern);
		   
void *get_match_positions(char *string, 
			  int stringlen, 
			  struct pattern *pattern);

void *add_search_pattern(struct search_patterns *sp, 
			 char *prefix, 
			 char *field, 
			 char *suffix,
			 char *type);
