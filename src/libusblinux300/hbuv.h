/*
 * hbuv.h -- HobbyBoards UVI interface for w1retap
 *  (c) 2011 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * Depends on the Dallas PD kit, so let's assume their PD licence (or
 * MIT/X if that works for you)
 *
 */


#ifndef __HBUV_H
#define __HBUV_H 1

typedef struct 
{
    uchar has_case;
    char  uvi_offset;
    char  temp_offset;
    unsigned char uv_type;
    short raw_temp;
    unsigned char raw_uvi;
    unsigned char is_valid;
    char version[2];
} hbuv_t;

#define HBUV_READVERSION 	(0x11)
#define HBUV_READTYPE 		(0x12)
#define HBUV_READTEMPERATURE 	(0x21)
#define HBUV_SETTEMPOFFSET 	(0x22) 
#define HBUV_READTEMPOFFSET 	(0x23)
#define HBUV_READUVI 		(0x24)
#define HBUV_SETUVIOFFSET	(0x25)
#define HBUV_READUVIOFFSET	(0x26)
#define HBUV_SETINCASE		(0x27)
#define HBUV_READINCASE		(0x28)

extern int HBUV_read_data (int,  uchar *, hbuv_t *);
extern void HBUV_read_version (int,  uchar *, char *);
extern int HBUV_setup (int, uchar *, hbuv_t *, char *);
extern void HBUV_set_case (int,  uchar *, uchar);
extern void HBUV_get_case (int,  uchar *, uchar *);
extern void HBUV_set_temp_offset (int,  uchar *, char);
extern void HBUV_get_temp_offset (int,  uchar *, char *);
extern void HBUV_set_uvi_offset (int,  uchar *, char);
extern void HBUV_get_uvi_offset (int,  uchar *, char *);

#endif
