/* Name: Pooja Sivakumar
 * CS login: sivakumar
 * Section(s): CS 354 LEC 002
 *
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 *
 * The function print_summary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * This is crucial for the driver to evaluate your work.
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/****************************************************************************/
/***** DO NOT MODIFY THESE VARIABLE NAMES ***********************************/

/* Globals set by command line args */
int s = 0; /* set index bits */
int E = 0; /* associativity */
int b = 0; /* block offset bits */
int verbosity = 0; /* print trace if set */
char* trace_file = NULL;

/* Derived from command line args */
int B; /* block size (bytes) B = 2^b */
int S; /* number of sets S = 2^s In C, you can use the left shift operator */

/* Counters used to record cache statistics */
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;
/*****************************************************************************/


/* Type: Memory address 
 * Use this type whenever dealing with addresses or address masks
 */                    
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
 * TODO 
 * 
 * NOTE: 
 * You might (not necessarily though) want to add an extra field to this struct
 * depending on your implementation
 * 
 * For example, to use a linked list based LRU,
 * you might want to have a field "struct cache_line * next" in the struct 
 */                    
typedef struct cache_line {                    
    char valid;
    mem_addr_t tag;
    struct cache_line * next;
} cache_line_t;

typedef cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;


/* The cache we are simulating */
cache_t cache;  

/* init_lines-
 * Recursively allocates cache lines and sets their tag and valid members to 0nhg
 */
cache_line_t* init_lines(cache_line_t * curr, int count ) {
    if (count == E){
		curr = NULL;  //the Eth line is NULL (when counting from 0)
	}
	else {
		curr = malloc(sizeof(cache_line_t));  //allocate a cache_line_t to curr
		//check malloc success
		if (curr == NULL) {
			printf("Error: Allocation failed\n");
			exit(1);
		}
		curr->next = init_lines(curr->next, ++count);  //allocate a cache_line_t to curr's next
		curr->tag = 0;  //set deafult tag
		curr->valid = '0';  //set deafult valid
	}
	return curr; //return the allocated curr
}

/* TODO - COMPLETE THIS FUNCTION
 * init_cache - 
 * Allocate data structures to hold info regrading the sets and cache lines
 * use struct "cache_line_t" here
 * Initialize valid and tag field with 0s.
 * use S (= 2^s) and E while allocating the data structures here
 */  

 void init_cache() {
	S = (int) pow(2, s); // S = 2^s
	cache = malloc(sizeof(cache_set_t ) * S); //allocate an array of sets (2^s) to cache
	//check malloc success
	if (cache == NULL) {
		printf("Error: Allocation failed\n");
		exit(1);
	}
	for(int i = 0; i < S; i++) {
		int count = 0;
		cache[i] = init_lines(cache[i], count); //allocate a linked list of lines to each set
	}
 }
 
/* free_recursive-
 * Recursively frees malloced entities
 */ 
 
void free_recursive(cache_line_t * curr) {
	if (curr == NULL) { //don't free the NULL pointer in the linked list
		return;
	}
	else {
			free_recursive(curr->next); //free the cache_line_t that curr's next is pointing to
			free(curr); //free curr
	}
}

/* TODO - COMPLETE THIS FUNCTION 
 * free_cache - free each piece of memory you allocated using malloc 
 * inside init_cache() function
 */                    
void free_cache() {
	for (int i = 0; i < S; i++) { 
		free_recursive(cache[i]); //free the linked list of lines and the set pointers of each set
	}
	free(cache); //free the cache pointer
	cache = NULL;
}

/* TODO - COMPLETE THIS FUNCTION 
 * access_data - Access data at memory address addr.
 *   If it is already in cache, increase hit_cnt
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase evict_cnt if a line is evicted.
 *   you will manipulate data structures allocated in init_cache() here
 */                    
void access_data(mem_addr_t addr) {
	mem_addr_t setAddr = addr>>b; //shift addr by b bits to make LSBs s-bits
	mem_addr_t tagAddr = setAddr>>s; //shift the addr again by s bits to make LSBs t-bits (this is the tag)

	int setMask = 0; //to create mask to isolate s bits from setAddr
	for(int i = s-1; i >= 0; i--) {
		setMask += (int)pow(2,i); //create a decimal number that is the sum of s powers of 2
	}

	setAddr = setAddr & setMask; //isolate the s-bits from setAddr

	cache_line_t * curr = cache[(int)setAddr]; //to keep track of the line we are checking
	cache_line_t * prev = NULL; //to keep track of the line we previously checked

	//stop checking when curr reaches NULL (end of set)
	while(curr != NULL) {
		if (curr->tag == tagAddr) { 
			if (curr->valid == '1') { //if tag matches and valid is 1, it's a hit
				hit_cnt++;  //increment hit count
				if (prev!= NULL) {  //to move matching line to MRU position 
					prev->next = curr->next;  //make previous line point to next line
					curr->next = cache[(int)setAddr];  //make current line point to the current MRU line
					cache[(int)setAddr] = curr;  //make current line MRU
				}
				break;  //do not continue checking other lines
			}
			else if (curr->valid == '0') {  //if tag matches but invalid, it's a miss
				miss_cnt++;  //increment miss count
				curr->valid = '1';  //simply switch that line to valid (the line is now occupied)
				if (prev!= NULL) {  //to move accessed line to MRU position
					prev->next = curr->next;  //make previous line point to next line
					curr->next = cache[(int)setAddr];  //make current line point to next line
					cache[(int)setAddr] = curr;  //make current line MRU
				}
				break;  //do not continue checking other lines
			}
		}
		else if (curr->tag != tagAddr) {  //if tag doesn't match, check the next line
			prev = curr;  //previous is now current line
			curr = curr->next;  //current is now the next line
		}
	}
	// if curr is NULL, there were no lines in the set that matched the tag, so it's a miss
	if (curr == NULL) {
		miss_cnt++;  //increment miss count
		int lineCount = 0;  //to hold number of full lines
		cache_line_t *temp = cache[(int)setAddr];  //to keep track of lines traversed
		cache_line_t * prevTemp = NULL; //to keep track of the previous line accessed
		if (temp->valid == '1')  //if the first line is full, incr line count
			lineCount++;
		while(temp->next != NULL) {  //stop at the LRU line
			prevTemp = temp;  //previous now has the current line
			temp = temp->next;  //current line now has the next line
			if (temp->valid == '1') {  //if the next line is full, incr line count
				lineCount++;
			}
		}
		if (lineCount == E) {  //if number of full lines is E, the set is full, 
							  //and we have to evict a line
			evict_cnt++;  // incr evict count
		}
		temp->tag = tagAddr;  //update LRU line's tag
		temp->valid = '1';  //update LRU line's valid to 1
		if (prevTemp != NULL) {  //if LRU != MRU, move line to MRU posiiton
			temp->next = cache[(int)setAddr];  //make LRU move point to the current MRU
			cache[(int)setAddr] = temp;  //make the current line point the MRU line
			prevTemp->next = NULL;  //make new LRU point to NULL
		}
	}
		
}

/* TODO - FILL IN THE MISSING CODE
 * replay_trace - replays the given trace file against the cache 
 * reads the input trace file line by line
 * extracts the type of each memory access : L/S/M
 * YOU MUST TRANSLATE one "L" as a load i.e. 1 memory access
 * YOU MUST TRANSLATE one "S" as a store i.e. 1 memory access
 * YOU MUST TRANSLATE one "M" as a load followed by a store i.e. 2 memory accesses 
 */                    
void replay_trace(char* trace_fn) {                      
    char buf[1000];
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if (!trace_fp) {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
      
            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

            // TODO - MISSING CODE
            // now you have:
            // 1. address accessed in variable - addr 
            // 2. type of acccess(S/L/M)  in variable - buf[1] 
            // call access_data function here depending on type of access
            
                access_data(addr);
				if (buf[1] == 'M') {
					access_data(addr); //Data modify accesses cache twice
				}
          
            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}

/*
 * print_usage - Print usage info
 */                    
void print_usage(char* argv[]) {                 
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * print_summary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded.
 */                    
void print_summary(int hits, int misses, int evictions) {                
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/*
 * main - Main routine 
 */                    
int main(int argc, char* argv[]) {                      
    char c;
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }

    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    /* Initialize cache */
    init_cache();

    replay_trace(trace_file);

    /* Free allocated memory */
   free_cache();

    /* Output the hit and miss statistics for the autograder */
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}




