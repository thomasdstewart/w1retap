typedef struct
{
    void *srv;
    char *url;
    char *user;
    char *pass;
    char *pass;
} W1RPC_t;

extern int get_xmlrpc_wx(W1RPC_t *, char **, double *);
extern int init_xmlrpc(W1RPC_t *);
extern int close_xmlrpc(W1RPC_t *);
