
#ifndef _TEST_H_
#define _TEST_H_


int I_InitEMS(void);



#ifndef __FIXEDTYPES__
#define __FIXEDTYPES__
typedef signed char				int8_t;
typedef unsigned char			uint8_t;
typedef short					int16_t;
typedef unsigned short			uint16_t;
typedef long					int32_t;
typedef unsigned long			uint32_t;
typedef long long				int64_t;
typedef unsigned long long		uint64_t;
#endif

typedef  char					byte;


typedef union _fixed_t_union {
	uint32_t wu;
	int32_t w;

	struct dual_int16_t {
		int16_t fracbits;
		int16_t intbits;
	} h;

	struct dual_uuint16_t {
		uint16_t fracbits;
		uint16_t intbits;
	} hu;

	struct quad_int8_t {
		int8_t fracbytelow;
		int8_t fracbytehigh;
		int8_t intbytelow;
		int8_t intbytehigh;
	} b;

	struct quad_uint8_t {
		uint8_t fracbytelow;
		uint8_t fracbytehigh;
		uint8_t intbytelow;
		uint8_t intbytehigh;
	} bu;

	struct productresult_mid_t {
		int8_t throwawayhigh;
		int16_t usemid;
		int8_t throwawaylow;
	} productresult_mid;

} fixed_t_union;


typedef union _int16_t_union {
	uint16_t hu;
	int16_t h;

	struct dual_int8_t {
		int8_t bytelow;
		int8_t bytehigh;
	} b;

	struct dual_uint8_t {
		uint8_t bytelow;
		uint8_t bytehigh;
	} bu;

} int16_t_union;

#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define true (1 == 1)
#define false (!true)

#endif
