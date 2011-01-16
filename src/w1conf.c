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
        w1->rcfile = NULL;
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
            char lbuf[512];
            
            if(1 == sscanf(buf,"log = %512[^\n]", lbuf))
            {
                dll_parse(w1, 'l', lbuf);
            }
            else if (1 == sscanf(buf,"init = %512[^\n]", lbuf))
            {
                dll_parse(w1, 'c', lbuf);
            }
            else if(1 == sscanf(buf,"rep = %512[^\n]", lbuf))
            {
                dll_parse(w1, 'r', lbuf);
            }
            else if(1 == sscanf(buf,"device = %512[^\n]", lbuf))
            {
                w1->iface = g_strdup(lbuf);
            }
            else if(1 == sscanf(buf,"pidfile = %512[^\n]", lbuf))
            {
                w1->pidfile = g_strdup(lbuf);
            }
            else if(1 == sscanf(buf,"log_time_t = %512[^\n]", lbuf))
            {
                w1->log_timet =  (lbuf[0] == 't' || lbuf[0] == 'T' ||
                                  lbuf[0] == 'y' || lbuf[0] == 'Y' ||
                                  lbuf[0] == '1');
            }
            else if(1 == sscanf(buf,"release_if = %512[^\n]", lbuf))
            {
                w1->release_me =  (lbuf[0] == 't' || lbuf[0] == 'T' ||
                                  lbuf[0] == 'y' || lbuf[0] == 'Y' ||
                                  lbuf[0] == '1');
            }
            else if(1 == sscanf(buf,"log_delimiter = %512[^\n]", lbuf))
            {
                if(*lbuf == '\\' && *(lbuf+1) == 't')
                    w1->log_delim[0] = '\t';
                else
                    w1->log_delim[0] = *lbuf;
            }
            else if (1 == sscanf(buf,"pressure_reduction_temp = %512[^\n]", lbuf))
            {
                w1->pres_reduction_temp = malloc(sizeof(double));
                *w1->pres_reduction_temp = strtod(lbuf,NULL);
            }
            else
            {
                (void)(sscanf(buf,"delay = %d", &w1->delay) ||    
                       sscanf(buf,"demonise = %d", &w1->daemonise) ||
                       sscanf(buf,"altitude = %d", &w1->altitude) ||
                       sscanf(buf,"vane_offset = %d", &w1->vane_offset) ||
                       sscanf(buf,"timestamp = %d", &w1->timestamp) ||
                       sscanf(buf,"logtemp = %d", &w1->logtmp) ||
                       sscanf(buf,"temp_scan = %d", &w1->temp_scan));
            }
        }
        fclose(fp);
    }
}

