/*
 * hbht.h- HobbyBoards 'new' humidity & temperature interface for w1retap
 *  (c) 2015 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * Depends on the Dallas PD kit, so let's assume their PD licence (or
 * MIT/X if that works for you)
 *
 */


#ifndef __HBHT_H
#define __HBHT_H 1

typedef struct
{
    short humid_offset;
    char version[2];
    uchar poll_freq;
    uchar debug;
} hbht_t;

typedef struct
{
    float temp;
    float humid;
} hbht_res_t;

#define HBHT_READVERSION 	(0x11)
#define HBHT_READTYPE 	        (0x12)
#define HBHT_GETPOLLFREQ 	(0x14)
#define HBHT_SETPOLLFREQ 	(0x94)
#define HBHT_GETHUMIDOFFSET 	(0x24)
#define HBHT_SETHUMIDOFFSET 	(0xA4)

#define HBHT_GETTEMPERATURE_C 	(0x40)
#define HBHT_GETCORRHUMID 	(0x21)

extern int HBHT_read_data (int,  uchar *, hbht_res_t *);
extern void HBHT_read_version (int,  uchar *, char *);
extern int HBHT_setup (int, uchar *, hbht_t *, char *);
extern void HBHT_set_hum_offset (int,  uchar *, short);
extern short HBHT_get_hum_offset (int,  uchar *);
extern void HBHT_set_poll_freq (int,  uchar *, uchar);
extern uchar HBHT_get_poll_freq (int,  uchar *);
#endif
