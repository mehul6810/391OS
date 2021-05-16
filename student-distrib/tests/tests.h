#ifndef TESTS_H
#define TESTS_H

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER() 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


// test launcher
void launch_tests();

#endif /* TESTS_H */
