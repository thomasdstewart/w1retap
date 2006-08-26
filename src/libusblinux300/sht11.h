extern int ReadSHT11(int portnum, u_char *ident,float *temp,  float *rh);

#define MATCHROM 0x55
#define CONVERTSENSOR 0x44
#define SKIPROM 0xCC
#define READSENSOR 0x45

