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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <gmodule.h>
#include <math.h>
#include <assert.h>
#include <syslog.h>

#include "ownet.h"
#include "temp10.h"
#include "atod26.h"
#include "atod20.h"
#include "cnt1d.h"
#include "pressure.h"
#include "sht11.h"
#include "swt1f.h"

#include "w1retap.h"

// Table from DalSemi application note 755A (page four): http://pdfserv.maxim-ic.com/en/an/app755A.pdf
#define WINDVANE_VCC       (1.0)                                // connected to VCC through pull-up resistor
#define WINDVANE_R2        (WINDVANE_VCC / 2.0)                 // connected to 50% voltage divider
#define WINDVANE_R1        (WINDVANE_VCC * (2.0/3.0))           // connected to 66% voltage divider
#define WINDVANE_GND       (0.0)                                // connected to GND
#define WINDVANE_MAX_ERROR ((WINDVANE_R1 - WINDVANE_R2) / 2.0)  // max acceptable error

static float wind_conversion_table[16][4] = { 
    {WINDVANE_GND, WINDVANE_VCC, WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_GND, WINDVANE_GND, WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_GND, WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_GND, WINDVANE_GND, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_GND, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_GND, WINDVANE_GND},  
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_VCC, WINDVANE_GND},  
    {WINDVANE_R2,  WINDVANE_VCC, WINDVANE_VCC, WINDVANE_GND},  
    {WINDVANE_R2,  WINDVANE_VCC, WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_R1,  WINDVANE_R1,  WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_R2,  WINDVANE_VCC, WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_R1,  WINDVANE_R1,  WINDVANE_VCC},  
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_R2,  WINDVANE_VCC}, 
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_R1,  WINDVANE_R1 },  
    {WINDVANE_VCC, WINDVANE_VCC, WINDVANE_VCC, WINDVANE_R2 },  
    {WINDVANE_GND, WINDVANE_VCC, WINDVANE_VCC, WINDVANE_R2 }}; 

static void w1_make_serial(char * asc, unsigned char *bin)
{
    int i,j;
    for(i = j = 0; i < 8; i++)
    {
        bin[i] = ToHex(asc[j]) << 4; j++;
        bin[i] += ToHex(asc[j]); j++;
    }
}

static void w1_set_coupler(w1_devlist_t *w1, w1_device_t *w, int active_lines)
{
  uchar a[4];
  w1_coupler_private_t *priv;

  if((w->stype == W1_COUPLER) && w->private){
    priv = (w1_coupler_private_t*)w->private;
    if(priv->active_lines != active_lines){
      SetSwitch1F(w1->portnum, w->serno, active_lines, 2, a, TRUE); /* Smart-On command requires 2 extra bytes */
      priv->active_lines = active_lines;
    }
  }
}

static int w1_select_device(w1_devlist_t *w1, w1_device_t *w)
{
    u_char thisdev[8];
    int found = 0, i;
    w1_device_t *dev;

    /* disconnect both branches of all couplers on the bus */
    for(dev=w1->devs, i=0; i < w1->numdev; i++, dev++){
      if(dev->stype == W1_COUPLER){
	/* avoid turning off the branch we're about to select */
	if(!(w->coupler && w->coupler->coupler_device == dev))
	  w1_set_coupler(w1, dev, COUPLER_ALL_OFF);
      }
    }

    /* turn on our branch. it is important to turn off the other
       branches first, to ensure that only one branch is ever active
       at any one time */
    if(w->coupler)
      w1_set_coupler(w1, w->coupler->coupler_device, w->coupler->branch);

    /* confirm that the device is present on the bus */
    owFamilySearchSetup(w1->portnum,w->serno[0]);
    while (owNext(w1->portnum,TRUE, FALSE)){
      owSerialNum(w1->portnum, thisdev, TRUE);
      if(memcmp(thisdev, w->serno, sizeof(thisdev)) == 0){
	found = 1;
	break;
      }
    }

    return found;
}

static int w1_validate(w1_devlist_t *w1, w1_sensor_t *s)
{
    int chk = 0;
    float act =  s->value;
    float rate = 0;
    
    s->valid = 1;

    if((s->flags & W1_RMIN) && (s->value < s->rmin))
    {
        s->value = (s->ltime) ? s->lval : s->rmin;
        chk = 1;
    }
    if((s->flags & W1_RMAX) && (s->value > s->rmax))
    {
        s->value = (s->ltime) ? s->lval : s->rmax;
        chk = 2;
    }

    if(chk == 0 && (s->flags & W1_ROC))
    {
        if (s->ltime > 0 && s->ltime != w1->logtime)
        {
            rate = fabs(s->value - s->lval) * 60.0 /
                (w1->logtime -s->ltime);
            if (rate > s->roc)
            {
                s->valid = 0;
                s->value = s->lval;
                chk = 3;
            }
        }
        if(s->valid)
        {
            s->ltime = w1->logtime;
            s->lval = s->value;
        }
    }
    if(chk != 0)
    {
        w1_replog (w1, "%s %.2f %.2f %.2f %.2f %d %d (%d)",
                   s->abbrv, s->value, act, rate,
                   s->lval, s->ltime, w1->logtime, chk);
    }
    return (int)s->valid;
}

static int w1_read_temp(w1_devlist_t *w1, w1_device_t *w)
{
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_select_device(w1, w))
    {
        float temp;
        
        if (ReadTemperature(w1->portnum, w->serno, &temp))
        {
            w->s[0].value = temp;
            w1_validate(w1, &w->s[0]);
        }
    }
    else
    {
        w->s[0].valid = 0;
    }

    return (w->s[0].valid);
}

static int w1_read_counter(w1_devlist_t *w1, w1_device_t *w)
{
    unsigned int cnt;
    int nv = 0;
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_select_device(w1, w))
    {
        if(w->s[0].abbrv)
        {
            if((w->s[0].valid = ReadCounter(w1->portnum, w->serno, 14, &cnt)))
            {
                w->s[0].value = cnt;
                nv += w1_validate(w1, &w->s[0]);
            }
        }
    
        if(w->s[1].abbrv)
        {
            if((w->s[1].valid = ReadCounter(w1->portnum, w->serno, 15, &cnt)))
            {
                w->s[1].value = cnt;
                nv += w1_validate(w1, &w->s[1]);
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

static float pressure_at_msl(float pres, float temp, int altitude)
{
#define __CONST_G (9.80665)
#define __CONST_R (287.04)
    float kt = temp +  273.15;
    float x = (__CONST_G * altitude / (__CONST_R * kt));
    pres *= exp(x);
    return pres;
}

#define BRAY_SLOPE (35.949367089)
#define BRAY_OFFSET (751.075949363)

static int w1_read_bray (w1_devlist_t *w1, w1_device_t *w)
{
    float vdd =0 ,vad =0;
    int nv = 0;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    if(w1_select_device(w1, w))
    {
        vdd = ReadAtoD(w1->portnum,TRUE, w->serno);    
        vad = ReadAtoD(w1->portnum,FALSE,w->serno);
        if (vad > 0.0)
        {
            float pres;
            float temp = Get_Temperature(w1->portnum,w->serno);
            double slope,offset;
	    float ptemp;
            if(w->params && w->params->num >= 2)
            {
                slope = w->params->values[0];
                offset = w->params->values[1];
		ptemp = w->params->values[2];
            }
            else
            {
                slope = BRAY_SLOPE;
                offset = BRAY_OFFSET;
		ptemp = temp;
            }
            pres = slope * vad + offset;
            if(w1->verbose)
            {
		fprintf(stderr,"vad %.3f slope %f, offset %f\n", vad, slope, offset);
            }

            if (w1->altitude)
            {
		if(w1->verbose)
		{
		    fprintf(stderr,"raw %f %f %d\n",pres, temp, w1->altitude);
		}
                pres = pressure_at_msl(pres, ptemp, w1->altitude);
		if(w1->verbose)
		{
		    fprintf(stderr,"msl %f \n",pres);
		}
            }
            
            if(w->s[0].name)
            {
                w->s[0].value = (strcasestr(w->s[0].name, "Pres"))
                    ? pres : temp;
                nv += w1_validate(w1, &w->s[0]);
            }
            else
            {
                w->s[0].valid = 0;
            }
            
            if(w->s[1].name)
            {
                w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                    ? temp : pres;
                nv += w1_validate(w1, &w->s[1]);
            }
            else
            {
                w->s[1].valid = 0;
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

// Values for MSL
#define HB_SLOPE (0.6562)
#define HB_OFFSET (26.0827)

static int w1_read_hb_pressure (w1_devlist_t *w1, w1_device_t *w)
{
    float vdd =0 ,vad =0;
    int nv = 0;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    if(w1_select_device(w1, w))
    {
        vdd = ReadAtoD(w1->portnum,TRUE, w->serno);    
        vad = ReadAtoD(w1->portnum,FALSE,w->serno);
        if (vad > 0.0)
        {
            float pres,pimp;
            float temp = Get_Temperature(w1->portnum,w->serno);
            double slope,offset;

            if(w->params && w->params->num == 2)
            {
                slope = w->params->values[0];
                offset = w->params->values[1];
            }
            else
            {
                slope = HB_SLOPE;
                offset = HB_OFFSET;
            }
	    pimp = slope * vad + offset;
            pres = 33.863886 * pimp;
            if(w1->verbose)
            {
                fprintf(stderr,"vad %.3f, vdd %.3f, slope %.4f, offset %.4f\n",
                        vad, vdd, slope, offset);
                fprintf(stderr,"temp %.2f, pres %.1f hPa (%.3f in)\n",
                        temp, pres, pimp);
            }
            
            if(w->s[0].name)
            {
                w->s[0].value = (strcasestr(w->s[0].name, "Pres"))
                    ? pres : temp;
                nv += w1_validate(w1, &w->s[0]);
            }
            else
            {
                w->s[0].valid = 0;
            }
            
            if(w->s[1].name)
            {
                w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                    ? temp : pres;
                nv += w1_validate(w1, &w->s[1]);
            }
            else
            {
                w->s[1].valid = 0;
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}

static int w1_read_sht11 (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    float temp, rh;
    
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    if(w1_select_device(w1,w))
    {
        if(ReadSHT11(w1->portnum, w->serno, &temp, &rh))
        {
            if(w->s[0].name)
            {
                w->s[0].value = (strcasestr(w->s[0].name, "Humidity"))
                    ? rh : temp;
                nv += w1_validate(w1, &w->s[0]);
            }
            else
            {
                w->s[0].valid = 0;
            }
            
            if(w->s[1].name)
            {
                w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                    ? temp : rh;
                nv += w1_validate(w1, &w->s[1]);
            }
            else
            {
                w->s[1].valid = 0;
            }
        }
        else
        {
            w->init = 0;
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    
    return nv;
}

static int w1_read_voltages(w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_select_device(w1,w))
    {
        int k;
        for(k =0; k< 2; k++)
        {
            float val=-1.0;
            if(w->s[k].abbrv)
            {
                int vreq=0;
                if (strcasestr(w->s[k].abbrv, "vdd"))
                {
                    val = ReadAtoD(w1->portnum,TRUE, w->serno);
                    vreq=1;
                }
                else if (strcasestr(w->s[k].abbrv, "vad"))
                {
                    val = ReadAtoD(w1->portnum, FALSE, w->serno);
                    vreq=1;                        
                }
                else if (strcasestr(w->s[k].abbrv, "vsens"))
                {
                    val = ReadVsens(w1->portnum, w->serno);
                    vreq=1;
                }
                if(vreq == 1)
                {
                    if(val != -1.0)
                    {
                        w->s[k].value = val;
                        nv += w1_validate(w1, &w->s[k]);
                    }
                }
                else
                {
                    if(strcasestr(w->s[k].name, "temp"))
                    {
                        val = Get_Temperature(w1->portnum,w->serno);
                        w->s[k].value = val;
                        nv += w1_validate(w1, &w->s[k]);
                    }
                }
            }
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    return nv;
}
                           
static int w1_read_humidity(w1_devlist_t *w1, w1_device_t *w)
{
    float humid=0,temp=0;
    float vdd =0 ,vad =0 ,vddx =0;
    int nv = 0;
    char vind=' ';
    
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(w1_select_device(w1,w))
    {
        vdd = vddx = ReadAtoD(w1->portnum,TRUE, w->serno);
        vad = ReadAtoD(w1->portnum, FALSE, w->serno);
        temp = Get_Temperature(w1->portnum, w->serno);

        if(vdd > 5.8)
        {

            vdd = (float)5.8;
            vind='+';
        }
        else if(vdd < 4.0)
        {
            vdd = (float) 4.0;
            vind = '-';
        }

        humid = (((vad/vdd) - (0.8/vdd))/0.0062)/(1.0546 - 0.00216 * temp);

        if(w->s[0].name)
        {
            w->s[0].value = (strcasestr(w->s[0].name, "Humidity"))
                ? humid : temp;
            nv += w1_validate(w1, &w->s[0]);
        }
        else
        {
            w->s[0].valid = 0;
        }

        if(w->s[1].name)
        {
            w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                ? temp : humid;
            nv += w1_validate(w1, &w->s[1]);
        }
        else
        {
            w->s[1].valid = 0;
        }
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
    }
    
    if(w->s[0].valid == 0 || w->s[1].valid == 0 )
    {
        w1_replog (w1, "%f %f %f %f %c", vddx, vad, temp, humid, vind);
    }
    return nv;
}

static int w1_read_pressure(w1_devlist_t *w1, w1_device_t *w)
{
    float temp,pres;
    int nv = 0;
    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    
    if(w1_select_device(w1, w))
    {
        if(w->init == 1)
        {
            if(Init_Pressure(w1->portnum, w->serno))
            {
                w->init = 2;
            }
        }
        if (w->init == 2)
        {
            if(ReadPressureValues (w1->portnum, &temp, &pres) )
            {
                if (w1->altitude)
                {
                    pres = pressure_at_msl(pres, temp, w1->altitude);
                }
                if(w->s[0].name)
                {
                    w->s[0].value = (strcasestr(w->s[0].name, "Pres"))
                        ? pres : temp;
                    nv += w1_validate(w1, &w->s[0]);
                }
                else
                {
                    w->s[0].valid = 0;
                }
                
                if(w->s[1].name)
                {
                    w->s[1].value = (strcasestr(w->s[1].name, "Temp"))
                        ? temp : pres;
                    nv += w1_validate(w1, &w->s[1]);
                }
                else
                {
                    w->s[1].valid = 0;
                }
            }
            else
            {
                w->init = 0;
            }
        }
        
    }
    else
    {
        w->s[0].valid = w->s[1].valid = 0;
        w->init  = 1;
    }
    return nv;
}

static int w1_read_windvane(w1_devlist_t *w1, w1_device_t *w)
{
  w1_windvane_private_t *private = NULL;
  int i, nv = 0;
  char message[80];
  float analogue[4];
  
  if(w->init == 0){
    w1_make_serial(w->serial, w->serno);
    if(w->private == NULL)
      w->private = malloc(sizeof(w1_windvane_private_t));
    w->init = 1;
  }
  
  private = (w1_windvane_private_t *)w->private;
  
    w->s[0].valid = w->s[1].valid = 0;

  if(w1_select_device(w1, w) && (private != NULL)){
    if(w->init == 1){
            if(SetupAtoDControl(w1->portnum, w->serno, private->control, message) && WriteAtoD(w1->portnum, FALSE, w->serno, private->control, 0x08, 0x11))
	w->init = 2;
    }
      
    if(w->init == 2){
            if(DoAtoDConversion(w1->portnum, FALSE, w->serno) && ReadAtoDResults(w1->portnum, FALSE, w->serno, analogue, private->control))
              {
                // In order to read the position of the wind vane, we first scale the readings
                // relative to the highest reading (which must be approximately VCC) and then
                // search a table of recognised vane positions to match our observation.
                float scaled[4], factor = fmaxf(analogue[0], fmaxf(analogue[1], fmaxf(analogue[2], analogue[3])));

                for(i=0; i<4; i++)
                    scaled[i] = analogue[i] / factor;
                
                // syslog(LOG_INFO, "Wind vane: analogue: %.4f %.4f %.4f %.4f   scaled: %.4f %.4f %.4f %.4f   max error %.4f",
                //        analogue[0], analogue[1], analogue[2], analogue[3], scaled[0], scaled[1], scaled[2], scaled[3], WINDVANE_MAX_ERROR);

                w->s[0].value = -1;
                for(i=0; i<16; i++){
                    if( ((scaled[0] <= wind_conversion_table[i][0]+WINDVANE_MAX_ERROR) && (scaled[0] >= wind_conversion_table[i][0]-WINDVANE_MAX_ERROR)) &&
                        ((scaled[1] <= wind_conversion_table[i][1]+WINDVANE_MAX_ERROR) && (scaled[1] >= wind_conversion_table[i][1]-WINDVANE_MAX_ERROR)) &&
                        ((scaled[2] <= wind_conversion_table[i][2]+WINDVANE_MAX_ERROR) && (scaled[2] >= wind_conversion_table[i][2]-WINDVANE_MAX_ERROR)) &&
                        ((scaled[3] <= wind_conversion_table[i][3]+WINDVANE_MAX_ERROR) && (scaled[3] >= wind_conversion_table[i][3]-WINDVANE_MAX_ERROR)) ) {
                        w->s[0].value = ((i - w1->vane_offset) & 0xf);
            nv += w1_validate(w1, &w->s[0]);
	    break;
	  }
	}

                if(w->s[0].value == -1) // no match found?
                    syslog(LOG_WARNING, "Wind vane: Unexpected values: v0=%.4f, v1=%.4f, v2=%.4f, v3=%.4f", analogue[0], analogue[1], analogue[2], analogue[3]);
      }
    }
  }
      
  return nv;
}

void w1_initialize_couplers(w1_devlist_t *w1)
{
    w1_device_t *w;
    int n, b;
    int nc = 0;
    w1_couplist_t clist[MAXCPL];
    w1_coupler_private_t *priv;

    for(w = w1->devs, n = 0; n < w1->numdev; n++, w++)
    {
        if (w->stype == W1_COUPLER && nc < MAXCPL)
        {
            w1_make_serial(w->serial, w->serno);
	    w->private = calloc(1, sizeof(w1_coupler_private_t));
	    priv = (w1_coupler_private_t*)w->private;
	    priv->active_lines = COUPLER_UNDEFINED;

            char *tmp, *p1, *p2;

            for(b = 0; b < 2; b++)
            {
                if(w->s[b].abbrv && w->s[b].name)
                {
		    tmp = strdup(w->s[b].name);
		    p1 = tmp;
                    for(; (p2 = strtok(p1,", |"));p1=NULL)
                    {
                        if(*p2)
                        {
  			    clist[nc].coupler_device = w;
                            strcpy(clist[nc].devid, p2);
			    if(b == 0)
			      clist[nc].branch = COUPLER_MAIN_ON;
			    else
			      clist[nc].branch = COUPLER_AUX_ON;
                            nc++;
                        }
                    }
		    free(tmp);
                }
            }
        }
    }
    
    int nx;
    
    for(nx = 0; nx < nc; nx++)
    {
        for(w = w1->devs, n = 0; n < w1->numdev; n++, w++)
        {
	    if (w->stype != W1_COUPLER && w->coupler == NULL &&
                strcmp(w->serial, clist[nx].devid) == 0)
            {
                w->coupler = (w1_coupler_t*)calloc(1, sizeof(w1_coupler_t));
                w->coupler->branch = clist[nx].branch;
		w->coupler->coupler_device = clist[nx].coupler_device;
//                break;
            }
        }
    }
}

void w1_all_couplers_off(w1_devlist_t *w1)
{
  w1_device_t *w;
  int n;

  if(w1->doread)
    for(w = w1->devs, n = 0; n < w1->numdev; n++, w++)
      if (w->stype == W1_COUPLER)
	w1_set_coupler(w1, w, COUPLER_ALL_OFF);
}


int w1_read_all_sensors(w1_devlist_t *w1)
{
    int nv = 0, r;

    if(w1->doread)
    {
        w1_device_t *d;
        int n;

        for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
        {
            switch(d->stype)
            {
                case W1_TEMP:
                    r = w1_read_temp(w1, d);
                    break;
                    
                case W1_HUMID:
                    r = w1_read_humidity(w1, d);
                    break;
                    
                case W1_PRES:
                    r = w1_read_pressure(w1, d);
                    break;
                    
                case W1_COUNTER:
                    r = w1_read_counter(w1, d);
                    break;
                    
                case W1_BRAY:
                    r = w1_read_bray(w1, d);
                    break;

                case W1_HBBARO:
                    r = w1_read_hb_pressure(w1, d);
                    break;

                case W1_SHT11:
                    r = w1_read_sht11(w1, d);
                    break;
                    
                case W1_WINDVANE:
                    r = w1_read_windvane(w1, d);
                    break;

                case W1_DS2438V:
                    r = w1_read_voltages(w1, d);
                    break;
                    
                case W1_INVALID:
                default:
                    r = -1;
                    break;
            }

            if(r >= 0){
                nv += r;
                if(r == 0)
                    syslog(LOG_WARNING, "Failed to read sensor %s (type %s)", d->serial, d->devtype);
            }
        }

	/* Put the bus back in a known state */
	w1_all_couplers_off(w1);
    }
    return nv;
}
