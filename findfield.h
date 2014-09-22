

void find_field(FILE *in_file, 
		char *search_field, 
		char *id_field, 
		FILE *out_file);

char** get_matches(char *string, 
		   int stringlen, 
		   int *m_pos, 
		   int m_count, 
		   char *pattern, 
		   int **depth);

void get_match_positions(char *string, 
			 int stringlen, 
			 char *pattern,
			 int **m_pos, 
			 int *m_count);
