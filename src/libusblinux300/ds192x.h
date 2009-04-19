#ifndef __DS192x_H
#define __DS192x_H 1

typedef struct 
{
    float rh;
    float temp;
    int kill;    
} ds1923_t;

typedef struct 
{
    float temp;
    int kill;    
} ds1921_t;

extern int ReadDS1921(int, uchar *, ds1921_t *);
extern int ReadDS1923(int, uchar *, ds1923_t *);
#endif
