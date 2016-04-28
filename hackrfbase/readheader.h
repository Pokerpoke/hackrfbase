#include <hackrf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#define  _test

#define FD_BUFFER_SIZE (8*1024)

#define FREQ_ONE_MHZ (1000000ull)

#define DEFAULT_FREQ_HZ (900000000ull) /* 900MHz */
#define FREQ_MIN_HZ	(0ull) /* 0 Hz */
#define FREQ_MAX_HZ	(7250000000ull) /* 7250MHz */
#define IF_MIN_HZ (2150000000ull)
#define IF_MAX_HZ (2750000000ull)
#define LO_MIN_HZ (84375000ull)
#define LO_MAX_HZ (5400000000ull)
#define DEFAULT_LO_HZ (1000000000ull)

#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */

#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */

#define SAMPLES_TO_XFER_MAX (0x8000000000000000ull) /* Max value */

#define BASEBAND_FILTER_BW_MIN (1750000)  /* 1.75 MHz min value */
#define BASEBAND_FILTER_BW_MAX (28000000) /* 28 MHz max value */

#define FLAG_FULL 0
#define FLAG_EMPTY 1
#define FLAG_RECVING 2

struct Buffer 
{
	char*  buf;
	size_t buflen;
	char   flag;  //0 full 1 empty 2 recving
	int    writepos;
	int    bufnum;
};



void FlushBuffer(Buffer* buf);
Buffer* GetFreeBufferu();
void init_buf(int bufsize);
void error_msg(char* msg);
void writetobuf(uint8_t * buf,int buflen);
int tx_callback(hackrf_transfer* transfer);
int StartHackRF();
int SetExplicit(uint64_t iffreq, uint64_t lofreq = DEFAULT_LO_HZ,uint32_t image_reject_selection = RF_PATH_FILTER_BYPASS);
