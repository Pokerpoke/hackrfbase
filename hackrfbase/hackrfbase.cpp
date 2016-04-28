// ReadHackRF.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "readheader.h"

#ifdef _test
FILE* fp1;
FILE* fp2;
#endif

static hackrf_device* device = NULL;
Buffer buf1, buf2;

void error_msg(char* msg)
{
#ifdef _DEBUG

	printf("%s\n", msg);

#else
	/////////////////   

#endif

}
void init_buf(int bufsize)
{

	buf1.buf = (char*)malloc(bufsize);
	buf1.buflen = bufsize;
	buf1.flag = FLAG_EMPTY;
	buf1.writepos = 0;
	buf1.bufnum = 1;

	buf2.buf = (char*)malloc(bufsize);
	buf2.buflen = bufsize;
	buf2.flag = FLAG_EMPTY;
	buf2.writepos = 0;
	buf2.bufnum = 2;


}

Buffer* GetFreeBuffer()
{
	if (buf1.flag == FLAG_RECVING)return &buf1;
	else if (buf2.flag == FLAG_RECVING)return &buf2;
	else if (buf1.flag == FLAG_EMPTY)
	{
#ifdef _test
		if (buf2.flag == FLAG_FULL)
		{
			//fwrite(buf2.buf,1,buf2.buflen,fp2);
			FlushBuffer(&buf2);
		}
#endif
		return &buf1;
	}
	else if (buf2.flag == FLAG_EMPTY)
	{
#ifdef _test
		if (buf1.flag == FLAG_FULL)
		{
			//fwrite(buf1.buf,1,buf1.buflen,fp1);
			FlushBuffer(&buf1);
		}
#endif
		return &buf2;
	}
	else { error_msg("all full buf"); FlushBuffer(&buf1); FlushBuffer(&buf2); return NULL; }
}

void FlushBuffer(Buffer* buf)
{
	memset(buf->buf, 0, buf->buflen);
	buf->flag = FLAG_EMPTY;
	buf->writepos = 0;

}
void writetobuf(uint8_t * buf, int buflen)
{
	Buffer* pbuf;
	pbuf = GetFreeBuffer();
	if (!pbuf)return;
	if (pbuf->buflen - pbuf->writepos >= buflen * sizeof(uint8_t))
	{
		memcpy(pbuf->buf + pbuf->writepos, buf, buflen * sizeof(uint8_t));
		pbuf->writepos += buflen * sizeof(uint8_t);
#ifdef _test
		printf("write buf%d writelen:%d totalsize:%d\n", pbuf->bufnum, buflen * sizeof(uint8_t), pbuf->writepos);
#endif
		pbuf->flag = FLAG_RECVING;
	}
	else
	{
		int writebuflen = pbuf->buflen - pbuf->writepos;
		memcpy(pbuf->buf + pbuf->writepos, buf, writebuflen);
		pbuf->writepos = pbuf->buflen;
		pbuf->flag = FLAG_FULL;
#ifdef _test
		printf("write buf%d writelen:%d totalsize:%d\n", pbuf->bufnum, writebuflen, pbuf->writepos);
#endif
		writetobuf(buf + writebuflen, buflen * sizeof(uint8_t) - writebuflen);

	}
}
int rx_callback(hackrf_transfer* transfer) {

	//test
	// 	printf("buffer_length:%d\n",transfer->buffer_length);
	// 	printf("valid_length:%d\n",transfer->valid_length);
	// 	printf("uint8_t:%d\n",sizeof(uint8_t));
	// 	printf("buflen*sizeof(uint8_t):%d\n",sizeof(uint8_t)*transfer->buffer_length);
	// 	printf("strlen*sizeof(uint8_t):%d\n",strlen((char*)transfer->buffer));

	uint8_t * buf = transfer->buffer;
	int len = transfer->buffer_length;
	writetobuf(transfer->buffer, transfer->buffer_length);
	return 0;
}

int SetExplicit(uint64_t iffreq, uint64_t lofreq, uint32_t image_reject_selection)
{
	if (!lofreq && !image_reject_selection)
	{
		error_msg("need lofreq>0 image_reject_selection = RF_PATH_FILTER_LOW_PASS or RF_PATH_FILTER_HIGH_PASS");
		return 0;
	}
	if (IF_MIN_HZ>iffreq || IF_MAX_HZ<iffreq)
	{
		error_msg("argument error: freq_hz shall be between 2150000000 and 2750000000");
		return 0;
	}
	if (LO_MIN_HZ>lofreq || LO_MAX_HZ<lofreq)
	{
		error_msg("argument error: freq_hz shall be between 5400000000");
		return 0;
	}
	int res = hackrf_set_freq_explicit(device, iffreq, lofreq, (rf_path_filter)image_reject_selection);
	if (res != HACKRF_SUCCESS)
	{
		error_msg("hackrf_set_freq_explicit() failed:");
		return res;

	}

}
int StartHackRF()
{
	int result;

	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		error_msg("init_error");
		return result;
	}

	result = hackrf_open(&device);
	if (result != HACKRF_SUCCESS) {
		error_msg("device open_error");
		return result;
	}
	///////////////////////////////////
	//////参数设置部分////////
	result |= hackrf_set_sample_rate_manual(device, 10e6, 1); //设置采样率为 
	result |= hackrf_set_baseband_filter_bandwidth(device, 1.45e6); //设置基带滤波器带宽
	result |= hackrf_set_vga_gain(device, 62); //设置可变增益放大器VGA的增益。最大值62dB，步进2dB. 
	result |= hackrf_set_lna_gain(device, 40); //设置低噪放LNA的增益。最大值40dB，步进6dB 
	result |= hackrf_set_freq(device, 300e6);//< 7250MHz 
											 /*
											 //PPM方案还需进一步确定
											 float ppm = -1.9;
											 uint64_t iffreq = 2160e6;
											 result |= SetExplicit((uint64_t)(iffreq * (1 + ppm * 0.000001)));//设置中心接收频率2150000000 ~ 2750000000
											 */
	result |= SetExplicit(2160e6);//设置中心接收频率2150000000 ~ 2750000000
								  ////////////////////////////////////
	result |= hackrf_start_rx(device, rx_callback, NULL);
	if (result != HACKRF_SUCCESS) {

		return result;
	}
	return result;
}


int _tmain(int argc, _TCHAR* argv[])
{

	// 	fp1 = fopen("buf1.txt","wb");
	// 	fp2 = fopen("buf2.txt","wb");
	init_buf(20000000);
	int res = StartHackRF();
	if (res != HACKRF_SUCCESS)
	{
		printf(hackrf_error_name((hackrf_error)res));
		return 0;
	}
	while (hackrf_is_streaming(device) == HACKRF_TRUE)
	{

	}
	return 0;
}

