/*
 * Copyright (C) 2005 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "w1retap.h"

void read_config(w1_devlist_t *w1)
{
    FILE *fp;
    char buf[256];
    if(w1->rcfile == NULL)
    {
	w1->rcfile = w1_get_from_home(".config/w1retap/rc");
    }
    
    if(w1->rcfile && access(w1->rcfile,R_OK) != 0)
    {
        free(w1->rcfile);
    }

    if(w1->rcfile == NULL)
    {
        w1->rcfile = strdup("/etc/default/w1retap");
    }

    fp = fopen(w1->rcfile,"r");
    if(fp)
    {
        while(fgets(buf, sizeof(buf), fp))
        {
            char *l = NULL,*c = NULL,*r=NULL;
            
            sscanf(buf,"log = %a[^\n]", &l) ||
                sscanf(buf,"init = %a[^\n]", &c) ||
                sscanf(buf,"rep = %a[^\n]", &r) ||                
                sscanf(buf,"device = %a[^\n]", &w1->iface) ||
                sscanf(buf,"delay = %d", &w1->delay) ||
                sscanf(buf,"demonise = %hd", &w1->daemonise) ||
                sscanf(buf,"logtemp = %hd", &w1->logtmp);

            if(l)
            {
                dll_parse(w1, 'l', l);
                free(l);
            }

            if(c)
            {
                dll_parse(w1, 'c', c);
                free(c);
            }
            if(r)
            {
                dll_parse(w1, 'r', r);
                free(r);
            }
        }
        fclose(fp);
    }
}

