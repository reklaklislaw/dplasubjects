
void make_delta1(int *delta1, 
		 uint8_t *pat, 
		 int32_t patlen);

int is_prefix(uint8_t *word, 
	      int wordlen, 
	      int pos);

int suffix_length(uint8_t *word, 
		  int wordlen, 
		  int pos);

void make_delta2(int *delta2, 
		 uint8_t *pat, 
		 int32_t patlen);

int boyer_moore(uint8_t *string, 
		uint32_t start,
		uint32_t stringlen, 
		uint8_t *pat, 
		uint32_t patlen);
		
