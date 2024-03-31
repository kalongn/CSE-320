#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>
#include "const.h"
#include "test_common.h"

void assert_equal_headers(AUDIO_HEADER *act, AUDIO_HEADER *exp) {
	cr_assert_eq(act->magic_number, exp->magic_number,
		     "Wrong magic number (0x%x != expected value 0x%x)",
		     act->magic_number, exp->magic_number);

	cr_assert_eq(act->data_offset, exp->data_offset,
		     "Wrong data offset (0x%d != expected value 0x%d)",
		     act->data_offset, exp->data_offset);

	cr_assert_eq(act->data_size, exp->data_size,
		     "Wrong data size (%u != expected value %u)",
		     act->data_size, exp->data_size);

	cr_assert_eq(act->sample_rate, exp->sample_rate,
		     "Wrong sample rate (%u != expected value %u)",
		     act->sample_rate, exp->sample_rate);

	cr_assert_eq(act->encoding, exp->encoding,
		     "Wrong value in encoding field (%u != expected value %u)",
		     act->encoding, exp->encoding);

	cr_assert_eq(act->channels, exp->channels,
		     "Wrong value in channels field (%u != expected value %u)",
		     act->channels, exp->channels);
}


Test(audio_header_suite, read_header, .timeout=10) {
	int time_duration= 20000;
	uint32_t length = time_duration * AUDIO_FRAME_RATE / 1000;

	//Create input data
	AUDIO_HEADER hdr = const_hdr;
	hdr.data_size = length * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE;

	char buf[sizeof(AUDIO_HEADER)+1];  // Leave space for '\0' written by fflush().

	//Create a file with input data allocated in it
	FILE *data = fmemopen(buf, sizeof(buf), "w");
	ref_audio_write_header(data, &hdr);
	fclose(data);

	//Read back the data
	AUDIO_HEADER header_res;
	data = fmemopen(buf, sizeof(buf), "r");
	int err = audio_read_header(data, &header_res);
	cr_assert_eq(err, 0, "Read back header did not succeed (ret = %d, expected 0)", err);

	//Perform criterion test
	assert_equal_headers(&header_res, &hdr);

	fclose(data);
	data = NULL;
}

Test(audio_header_suite, write_header, .timeout=10) {
	int time_duration= 20000;
	uint32_t length = time_duration * AUDIO_FRAME_RATE / 1000;

	//Create input data
	AUDIO_HEADER audio_header_in = const_hdr;
	audio_header_in.data_size = length * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE;

	size_t size;
	char *content;
	//Create an empty file (in a stream fashion)
	FILE *file_res = open_memstream(&content, &size);

	//audio write header test
	audio_write_header(file_res, &audio_header_in);
	fclose(file_res);

	//Check that correct number of bytes was written.
	cr_assert_eq(size, sizeof(AUDIO_HEADER),
		     "Number of bytes written (%ld) does not match expected (%ld)",
		     size, sizeof(AUDIO_HEADER));

	//Re-open data for reading
	file_res = fmemopen(content, size, "r");

	//Read back using reference code for header content confirmation
	AUDIO_HEADER header_res;
	if(ref_audio_read_header(file_res, &header_res))
	    cr_skip_test("Read back header did not succeed\n");
	fclose(file_res);
	free(content);

	//Perform criterion test
	assert_equal_headers(&header_res, &audio_header_in);
}

/* normal */
Test(audio_suite, read_sample_normal, .timeout=10){
    void* buf = "\x82\x40";
    int16_t exp_sample = 0x8240;
    int n_byte = 3;
    FILE* in = fmemopen(buf, n_byte, "r");
    int16_t sample = 0x0000;
    int16_t *samplep = &sample;
    int ret = audio_read_sample(in, samplep);
    int exp_ret = 0;
    cr_assert_eq(ret, exp_ret,
		 "Invalid return for audio_read_sample.  Got: %d | Expected: %d", ret, exp_ret);
    cr_assert_eq(sample, exp_sample,
		 "Correct sample not read FILE*in. Got: 0x%hx | Expected: 0x%hx", sample, exp_sample);
    if(fclose(in)==EOF) printf("====flose ERROR 01\n");
}

/* read from /dev/null */
Test(audio_suite, read_sample_EOF, .timeout=10){
    FILE* in = fopen("/dev/null", "r");
    int16_t sample = 0x8888;
    int16_t *samplep = &sample;
    int ret = audio_read_sample(in, samplep);
    int exp_ret = EOF;
    if(fclose(in)==EOF) printf("====flose ERROR 02\n");
    cr_assert_eq(ret, exp_ret, "Invalid return for audio_read_sample.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

/* write to file */
Test(audio_suite, write_sample_normal, .timeout=10){
    char* buf = malloc(2+2); // make space of fmemopen \0 byte
    int16_t sample = 0x8240;
    char* exp_buf = "\x82\x40";
    int n_byte = 2;
    FILE* out = fmemopen(buf, n_byte, "w");
    int ret = audio_write_sample(out, sample);
    if(fputc('\x00', out) == EOF) printf("*******ERROR\n"); // leave a byte for fmemopen ending \0
    fclose(out); // need to close it to show the result
    int exp_ret = 0;

    cr_assert_eq(ret, exp_ret,
		 "Invalid return for audio_write_sample.  Got: %d | Expected: %d", ret, exp_ret);
    cr_assert_eq(*buf, *exp_buf,
		 "FILE*out buf not written correctly. Got: 0x%hhx | Expected: 0x%hhx", *buf, *exp_buf);
    cr_assert_eq(*(buf+1), *(exp_buf+1),
		 "FILE*out buf not written correctly. Got: 0x%hhx | Expected: 0x%hhx", *(buf+1), *(exp_buf+1));

    free(buf);
}
