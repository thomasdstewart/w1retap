#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include <netinet/in.h>
#include <sys/socket.h>

int main (int argc, char **argv)
{
    char data[4096];
    int n;
    int skt = 0;
    short port = 6660;
    short nodet = 0;
    char *fnam = "/tmp/.wx_static.dat";
    struct sockaddr_in name;
    int c;
    char auth[256] = "foo";
    char *p;
    
    if((p = getenv("HOME")))
    {
        char fbuf[256];
        FILE *fp;
        
        strcpy(fbuf, p);
        strcat(fbuf,"/.config/w1retap/applet");
        if((fp = fopen(fbuf, "r")))
        {
            while(fgets(fbuf, sizeof(fbuf),fp))
            {
                sscanf(fbuf, "uauth = %[^\n]", auth);
            }
            fclose(fp);
        }
    }

    while ((c = getopt(argc, argv, "p:f:D")) != EOF)
    {
        switch (c)
        {
            case 'D':
                nodet =1;
                break;
            case 'p':
                port = strtol(optarg,NULL,0);
                break;
            case 'f':
                fnam = strdup(optarg);
                break;
        }
    }

    if(-1 == (skt = socket(AF_INET, SOCK_DGRAM, 0)))
    {
        perror("socket");
        exit (0);
    }
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if(-1 == bind(skt, (struct sockaddr *)&name, sizeof(name)))
    {
        perror("bind");
        exit (0);
    }

    if(!nodet)
        daemon(0,0);
    
    do
    {
        socklen_t len;
        len = sizeof(name);
        if ((n = recvfrom(skt, data, sizeof(data), 0,
                          (struct sockaddr *)&name, &len)) > 0)
        {
            *(data+n) = 0;
            if(nodet)
                printf("Data %s Auth %s\n", data, auth);
            
            if(0 == strcmp(data,auth))
            {
                int fd;
		strcpy(data,"No data, alas");
		n = sizeof ("No data, alas") -1;
                if((fd = open(fnam, O_RDONLY)) != -1)
                {
                    n = read(fd, data, sizeof(data));
                    close(fd);
                }
            }
            else
            {
                strcpy(data,"Unwelcome, alas");
                n = sizeof("Unwelcome, alas")-1;
            }
            
            if(n > 0)
            {
                n = sendto(skt, data, n, 0, (struct sockaddr *) &name, len);
            }
        }
    } while (skt > 0);
    return 0;
}

