/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

Copyright (C) 2003 Liam Girdwood <liam@gnova.org>

Modified for sun and moon:
(C) 2007 Jonathan Hudson

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <libnova/solar.h>
#include <libnova/lunar.h>
#include <libnova/julian_day.h>
#include <libnova/rise_set.h>
#include <libnova/transform.h>

// Current moon image: http://tycho.usno.navy.mil/cgi-bin/phase.gif

void print_date (FILE *fp, char * title, struct ln_zonedate* date)
{
    fprintf(fp, "%s\t%04d-%02d-%02d %02d:%02d:%02.0f\n",
           title,date->years, date->months, date->days,
           date->hours, date->minutes, date->seconds);
}

int main (int argc, char * argv[])
{
    struct ln_rst_time rst;
    struct ln_zonedate rise, set, transit;
    struct ln_lnlat_posn observer;
    struct ln_date ddif;
    double JD;
    int c;
    char *fn = NULL;
    FILE *fp;

        /* observers location (Netley Marsh :) */
    observer.lat = 50.9153;
    observer.lng = -1.53036;

    while((c = getopt(argc, argv, "p:f:")) != EOF)
    {
        char *ptr;
        
        switch(c)
        {
            case 'p':
                observer.lat=strtod(optarg, &ptr);                
                observer.lng=strtod(ptr, NULL);
                break;
            case 'f':
                fn = strdup(optarg);
                break;
        }
    }

    fp = (fn) ? fopen(fn,"w") : stdout;
    
    time_t t = time(NULL);
    t = (t / (24*3600))*(24*3600);
    JD=ln_get_julian_from_timet(&t);
    
	/* rise, set and transit */
    if (ln_get_solar_rst (JD, &observer, &rst) == 1) 
        fputs ("Sun is circumpolar\n", fp);
    else {
        ln_get_local_date (rst.rise, &rise);
        ln_get_local_date (rst.set, &set);
        ln_get_local_date (rst.transit, &transit);        
        print_date (fp, "Sun Rise", &rise);
        print_date (fp, "Sun Set", &set);
        print_date (fp, "Sun Transit", &transit);        
        double dif = rst.set - rst.rise;
        ln_get_date(dif-0.5,&ddif);
        if ( rint(ddif.seconds) == 60)
        {
            ddif.minutes += 1;
            ddif.seconds = 0;
        }
        if ( ddif.minutes == 60)
        {
            ddif.hours += 1;
            ddif.minutes = 0;
        }
        fprintf(fp, "Day Length\t%02d:%02d:%02.0f\n",
               ddif.hours, ddif.minutes, ddif.seconds);

        if (ln_get_solar_rst (JD+1.0, &observer, &rst) == 1) 
            fputs ("Sun is circumpolar\n", fp);
        else {
            double difx = rst.set - rst.rise;
            double delta;
            char *dtype=NULL;
            
            delta = difx-dif;
            if(delta > 0)
            {
                dtype="longer";
            }
            else
            {
                delta=-delta;
                dtype="shorter";
            }

            ln_get_date(delta-0.5,&ddif);
            if ( rint(ddif.seconds) == 60)
            {
                ddif.minutes += 1;
                ddif.seconds = 0;
            }
            if ( ddif.minutes == 60)
            {
                ddif.hours += 1;
                ddif.minutes = 0;
            }
            fprintf(fp, "Tomorrow\t%02d:%02d:%02.0f %s\n",
                   ddif.hours, ddif.minutes, ddif.seconds,dtype);
        }
    }

    if (ln_get_lunar_rst (JD, &observer, &rst) == 1) 
        fputs ("Moon is circumpolar\n", fp);
    else {
        ln_get_local_date (rst.rise, &rise);
#if 0
        if (rst.set < rst.rise)
        {
            ln_get_lunar_rst (JD+1.0, &observer, &rst);
        }
#endif
        ln_get_local_date (rst.set, &set);
        print_date (fp, "Moon Rise", &rise);
        print_date (fp, "Moon Set", &set);        
    }
    if(fn)
    {
        fclose(fp);
    }
    return 0;
}
