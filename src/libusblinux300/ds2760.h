#ifndef __DS270_H
#define __DS270_H 1

typedef struct 
{
    float volts;
    float curr;
    float accum;
    float temp;
} ds2760_t;

extern int ReadDS2760(int, uchar *, ds2760_t *);
#endif
