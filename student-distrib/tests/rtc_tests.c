#include "rtc_tests.h"
#include "tests.h"

#include "../devices/rtc.h"
#include "../lib/lib.h"

/* RTC frequency change working appropriately
 *
 * Tests the rtc "write" function that changes the frequency to the one provided in the buffer
 * Inputs: none
 * Outputs: PASS/FAIL
 * Side Effects: Prints out 1 every time an interrupt is called, clears screen after changing the frequency
 * Coverage: RTC
 * Files: rtc.h/c
 */
int rtc_r_w_test(){
	/* Variables to call rtc_write */
	int temp = 0;
	#define NUM_FREQS 10
	int32_t freqs[NUM_FREQS] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    int32_t freq_invalid = 12;
	int wait_secs = 1;

	// Fake file to use for test
	file_t file;

    /* Checking if rtc_open functions correctly */
    if(-1 == rtc_open(&file)){
		printf("rtc_open failed\n");
		return FAIL;
	}

	// Test valid freqs
	int i, counter;
	for(i = 0; i < NUM_FREQS; i++){
		/* Reset for next frequency */
	    counter = 0;
	    clear();

	    /* Capturing the return value of rtc_write to evaluate PASS/FAIL */
	    temp = rtc_write(&file, freqs + i, 4);
	    printf("rtc_write: freq = %d \n", freqs[i]);
	    if (temp == -1){
			printf("rtc_write failed\n");
	    	return FAIL;
		}

	    /* Delaying before updating the RTC frequency again */
	   	while(counter < wait_secs*freqs[i]){
			if(rtc_read(&file, NULL, 0) == -1){
				printf("rtc_read failed\n");
				return FAIL;
			}

			printf("1");
			counter++;
		}
	}
	clear();

	// Test invalid freq
	/* Capturing the return value of rtc_write to evaluate PASS/FAIL */
    temp = rtc_write(&file, &freq_invalid, 4);
    if (temp != -1){
		printf("rtc_write: invalid frequency = %d got return value = %d \n", freq_invalid, temp);
    	return FAIL;
	}

    /* Capturing the return value of rtc_close to evaluate PASS/FAIL */
    temp = rtc_close(&file);
    if (temp == -1){
		printf("rtc_close: return value = %d \n", temp);
    	return FAIL;
	}

    /* All tests passed */
	#undef NUM_FREQS
    return PASS;
}
