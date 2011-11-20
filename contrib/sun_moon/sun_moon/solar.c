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


A simple example showing some solar calculations.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <libnova/solar.h>
#include <libnova/julian_day.h>
#include <libnova/rise_set.h>
#include <libnova/transform.h>

void print_date (char * title, struct ln_zonedate* date)
{
        printf ("\n%s\n",title);
        printf (" Year    : %d\n", date->years);
        printf (" Month   : %d\n", date->months);
        printf (" Day     : %d\n", date->days);
        printf (" Hours   : %d\n", date->hours);
        printf (" Minutes : %d\n", date->minutes);
        printf (" Seconds : %f\n", date->seconds);
}

int main (int argc, char * argv[])
{
        struct ln_equ_posn equ;
        struct ln_rst_time rst;
        struct ln_zonedate rise, set, transit;
        struct ln_lnlat_posn observer;
        struct ln_helio_posn pos;
        double JD;
        int c;
        time_t t = 0;
        
        observer.lat = 50.9153;
        observer.lng = -1.53036;

        while((c = getopt(argc, argv, "p:f:t:h?")) != EOF)
        {
            char *ptr;
            
            switch(c)
            {
                case 't':
                    t = strtol(optarg, NULL, 0);
                    break;
                case 'p':
                    observer.lat=strtod(optarg, &ptr);                
                    observer.lng=strtod(ptr, NULL);
                    break;
                case '?':
                case 'h':
                    fputs("sun_moon [-p \"lat long\"] [-f outfile] [-t time_t]\n", stderr);
                exit (0);
                break;    
            }
        }

        if (t == 0)
            t = time(NULL);
    
        JD=ln_get_julian_from_timet(&t);
        printf ("JD %f\n", JD);
        
        /* geometric coordinates */
        ln_get_solar_geom_coords (JD, &pos);
        printf("Solar Coords longitude (deg) %f\n", pos.L);
        printf("             latitude (deg) %f\n", pos.B);
        printf("             radius vector (AU) %f\n", pos.R);
        
        /* ra, dec */
        ln_get_solar_equ_coords (JD, &equ);
        printf("Solar Position RA %f\n", equ.ra);
        printf("               DEC %f\n", equ.dec);
        
        /* rise, set and transit */
        if (ln_get_solar_rst (JD, &observer, &rst) == 1) 
                printf ("Sun is circumpolar\n");
        else {
                ln_get_local_date (rst.rise, &rise);
                ln_get_local_date (rst.transit, &transit);
                ln_get_local_date (rst.set, &set);
                print_date ("Rise", &rise);
                print_date ("Transit", &transit);
                print_date ("Set", &set);
        }
        
        return 0;
}
