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
#include <unistd.h>
#include <sys/types.h>

#include "ownet.h"
#include "temp10.h"
#include "atod26.h"
#include "atod20.h"
#include "cnt1d.h"
#include "pressure.h"
#include "sht11.h"
#include "swt1f.h"
#include "ds2760.h"
#include "ds192x.h"
#include "hbuv.h"
#include "hbht.h"
#include "w1retap.h"

#define W1_RETRY 10

// Table from DalSemi application note 755A (page four): http://pdfserv.maxim-ic.com/en/an/app755A.pdf
#define WINDVANE_VCC       (1.0)                                // connected to VCC through pull-up resistor
#define WINDVANE_R2        (WINDVANE_VCC / 2.0)                 // connected to 50% voltage divider
#define WINDVANE_R1        (WINDVANE_VCC * (2.0/3.0))           // connected to 66% voltage divider
#define WINDVANE_GND       (0.0)                                // connected to GND
#define WINDVANE_MAX_ERROR ((WINDVANE_R1 - WINDVANE_R2) / 2.0)  // max acceptable error

typedef struct
{
    float vdd;
    float vad;
    float vsens;
    float temp;
    int   req;
} ds2438v_t;

enum W1_DS2438_modes {DS2438_VDD=1, DS2438_VAD=2, DS2438_VSENS=4,
                      DS2438_TEMP=8};

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

extern int lfd;

static void w1_set_invalid(w1_device_t *w)
{
    int k;
    for(k = 0; k < w->ns; k++)
    {
        w->s[k].valid = 0;
    }
}

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
    static char *restxt[] = {"OK","RATE","MIN","undef","MAX","SEQ"};

    int chk = 0;
    float act =  s->value;
    float rate = 0;

    if((s->flags & W1_RMIN) && (s->value < s->rmin))
    {
        s->value = (s->ltime) ? s->lval : s->rmin;
        chk = W1_RMIN;
    }
    if((s->flags & W1_RMAX) && (s->value > s->rmax))
    {
        s->value = (s->ltime) ? s->lval : s->rmax;
        chk = W1_RMAX;
    }

    if(chk == 0 && (s->flags & W1_ROC))
    {
//        if (!(w1->allow_escape == 1 && (s->reason & (W1_RMIN|W1_RMAX)) != 0))
        {
            if (s->ltime > 0 && s->ltime != w1->logtime)
            {
                rate = fabs(s->value - s->lval) * 60.0 /
                    (w1->logtime - s->ltime);
                if (rate > s->roc)
                {
                    s->value = s->lval;
                    chk = W1_ROC;
                }
            }
        }
    }

    s->reason = chk;

    if(chk ==0 && strncmp(s->abbrv,"RGC", 3) == 0)
    {
/*
  w1_replog (w1, "%s result=%.2f actual=%.2f rate=%.2f prev=%.2f "
                   "prevtime_t=%d logtime_t=%d reason=%s",
                   s->abbrv, s->value, act, rate,
                   s->lval, s->ltime, w1->logtime, restxt[chk]);
*/
        if(s->value < s->lval)
        {
            s->value = s->lval;
            chk = W1_SEQ;
        }
    }

    if(chk == 0)
    {
        s->ltime = w1->logtime;
        s->lval = s->value;
        s->valid = 1;
    }
    else
    {
        w1_replog (w1, "%s result=%.2f actual=%.2f rate=%.2f prev=%.2f "
                   "prevtime_t=%d logtime_t=%d reason=%s",
                   s->abbrv, s->value, act, rate,
                   s->lval, s->ltime, w1->logtime, restxt[chk]);
        s->valid = 0;
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

        if (ReadTemperature(w1->portnum, w->serno, &temp, w1->temp_scan))
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
    w1_set_invalid(w);
    if(w1_select_device(w1, w))
    {
        if(w->s[0].abbrv)
        {
            if((w->s[0].valid = ReadCounter(w1->portnum, w->serno, 14, &cnt)))
            {
                if(w->params && w->params->num >= 1)
                    cnt += (int)w->params->values[0];
                w->s[0].value = cnt;
                nv += w1_validate(w1, &w->s[0]);
            }
        }

        if(w->s[1].abbrv)
        {
            if((w->s[1].valid = ReadCounter(w1->portnum, w->serno, 15, &cnt)))
            {
                if(w->params && w->params->num > 1)
                    cnt += (int)w->params->values[1];
                w->s[1].value = cnt;
                nv += w1_validate(w1, &w->s[1]);
            }
        }
    }
    return nv;
}


static int w1_read_ds2450(w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    char msg[48];
    float advolts[4];
    uchar ctrl[16];

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    w1_set_invalid(w);
    if(w1_select_device(w1, w))
    {
        if (SetupAtoDControl(w1->portnum, w->serno, ctrl, msg))
        {
            fprintf(stderr, "A/D settings found\n%s\n", msg);
        }
        else
        {
            fprintf(stderr, "ERROR, DS2450 set up unsuccessful!\n");
        }

        DoAtoDConversion(w1->portnum, 0, w->serno);
        if (ReadAtoDResults(w1->portnum, 0, w->serno, advolts, ctrl))
        {
            w1_sensor_t *s;
            char adname[4] = "ADA";
            int i;

            for (i = 0; i < 4; i++)
            {
                *(adname+2) = 'A'+ (char)i;
                if((s = w1_match_sensor(w, adname)) != NULL)
                {
                    s->value = advolts[i];
                    nv += w1_validate(w1, s);
                    fprintf(stderr, "DS2450 %s %.1f\n", adname, advolts[i]);
                }
            }
         }
    }
    return nv;
}

static int w1_read_voltages(w1_devlist_t *w1, w1_device_t *w, ds2438v_t *v)
{
    int nv = 0;
    int sel = 1;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    if(v == NULL)
    {
        sel = w1_select_device(w1,w);
    }

    w1_set_invalid(w);
    if(sel)
    {
        float val=-1.0;
        w1_sensor_t *s;

        s = w1_match_sensor(w, "vdd");
        if ((v && (v->req & DS2438_VDD)) || s)
        {
            val = ReadAtoD(w1->portnum,TRUE, w->serno);
            if(s)
            {
                if(val != -1.0)
                {
                    s->value = val;
                    nv += w1_validate(w1, s);
                }
                else
                {
                    s->valid = 0;
                }
            }
            if(v)
                v->vdd = val;
        }
        else
        {
            if(w1_match_sensor(w,"Solar") != NULL)
            {
                float v1;
                v1 = ReadAtoD(w1->portnum,TRUE, w->serno);
                w1_replog (w1, "Solar vdd %.1f", v1);
            }
        }

        s = w1_match_sensor(w, "vad");
        if ((v && (v->req & DS2438_VAD)) || s)
        {
            val = ReadAtoD(w1->portnum, FALSE, w->serno);
            if(s)
            {
                if(val != -1.0)
                {
                    s->value = val;
                    nv += w1_validate(w1, s);
                }
                else
                {
                    s->valid = 0;
                }
            }
            if(v)
                v->vad = val;
        }

        s = w1_match_sensor(w, "vsens");
        if ((v && (v->req & DS2438_VSENS)) || s)
        {
            val = ReadVsens(w1->portnum, w->serno);
            if(s)
            {
                if(val != -1.0)
                {
                    s->value = val;
                    nv += w1_validate(w1, s);
                }
                else
                {
                    s->valid = 0;
                }
            }
            if(v)
                v->vsens = val;
        }

        s = w1_match_sensor(w, "temp");
        if ((v && (v->req & DS2438_TEMP)) || s)
        {
            val = Get_Temperature(w1->portnum,w->serno);
            if(val != -1)
            {
                int nv1 = 0;
                if(s)
                {
                    s->value = val;
                    nv1 = w1_validate(w1, s);
                    nv += nv1;
                }
                if(v && nv1 == 1)
                    v->temp = val;
            }
        }
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
    int nv = 0;
    int k;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    if(w1_select_device(w1, w))
    {
        ds2438v_t v = {0};
        v.req = DS2438_VAD | DS2438_TEMP;
        nv = w1_read_voltages(w1, w, &v);

        if (v.vad > 0.0)
        {
            float pres;
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
		ptemp = v.temp;
            }
            pres = slope * v.vad + offset;
            if(w1->verbose)
            {
		fprintf(stderr,"vad %.3f slope %f, offset %f\n", v.vad, slope, offset);
            }

            if (w1->altitude)
            {
                float prtemp;
		if(w1->verbose)
		{
		    fprintf(stderr,"raw %f %f %d\n",pres, v.temp, w1->altitude);
		}

                prtemp = (w1->pres_reduction_temp) ? *w1->pres_reduction_temp : ptemp;
                pres = pressure_at_msl(pres, prtemp, w1->altitude);
		if(w1->verbose)
		{
		    fprintf(stderr,"msl %f \n",pres);
		}
            }
            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "Pres")))
            {
                s->value = pres;
                nv += w1_validate(w1, s);
            }
        }
    }
    else
    {
        for(k = 0; k < w->ns; k++)
        {
            w->s[k].valid = 0;
        }
    }
    return nv;
}

// Values for MSL
#define INHGHPA 33.863886
#define HB_SLOPE (0.6562*INHGHPA)
#define HB_OFFSET (26.0827*INHGHPA)

static int w1_read_hb_pressure (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    w1_set_invalid(w);
    if(w1_select_device(w1, w))
    {
        ds2438v_t v = {0};
        v.req = DS2438_VAD | DS2438_VDD | DS2438_TEMP;
        nv = w1_read_voltages(w1, w, &v);

        if (v.vad > 0.0)
        {
            float pres;
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
	    pres = slope * v.vad + offset;
            if(offset < 100.0)
            {
                pres *= 33.863886;
            }

            if(w1->verbose)
            {
                fprintf(stderr,"vad %.3f, vdd %.3f, slope %.4f, offset %.4f\n",
                        v.vad, v.vdd, slope, offset);
                fprintf(stderr,"temp %.2f, pres %.1f hPa\n", v.temp, pres);
            }

            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "Pres")))
            {
                s->value = pres;
                nv += w1_validate(w1, s);
            }
        }
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
    w1_set_invalid(w);
    if(w1_select_device(w1,w))
    {
        if(ReadSHT11(w1->portnum, w->serno, &temp, &rh))
        {
            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = temp;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Humidity")))
            {
                s->value = rh;
                nv += w1_validate(w1, s);
            }
        }
    }
    return nv;
}

static int w1_read_humidity(w1_devlist_t *w1, w1_device_t *w)
{
    float humid=0;
    float vddx =0;
    int nv = 0;
    char vind=' ';
    ds2438v_t v = {0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    w1_set_invalid(w);

    if(w1_select_device(w1,w))
    {
        v.req = DS2438_VAD | DS2438_VDD | DS2438_TEMP;
        nv = w1_read_voltages(w1, w, &v);
        vddx = v.vdd;

        if(v.vdd > 5.8)
        {
            v.vdd = (float)5.8;
            vind='+';
        }
        else if(v.vdd < 4.0)
        {
            v.vdd = (float) 4.0;
            vind = '-';
        }

        humid = (((v.vad/v.vdd) - (0.8/v.vdd))/0.0062)/
                      (1.0546 - 0.00216 * v.temp);

        w1_sensor_t *s;
        if((s = w1_match_sensor(w, "Temp")))
        {
            s->value = v.temp;
            nv += w1_validate(w1, s);
        }

        if((s = w1_match_sensor(w, "Humidity")))
        {
            s->value = humid;
            nv += w1_validate(w1, s);
        }
    }

    if(w->s[0].valid == 0 || w->s[1].valid == 0 )
    {
        w1_replog (w1, "readhumid %f %f %f %f %c", vddx, v.vad, v.temp, humid, vind);
    }
    return nv;
}

static int w1_read_hih (w1_devlist_t *w1, w1_device_t *w)
{
    float humid=0,temp=0;
    float vdd =0 ,vad =0;
    int nv = 0, nv1 = 0;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);
    if(w1_select_device(w1,w))
    {
        ds2438v_t v = {0};
        v.req = DS2438_VAD | DS2438_VDD | DS2438_TEMP;
        nv1 = w1_read_voltages(w1, w, &v);
        if(nv1 != 0)
        {
                // Shamelessly researched from owfs; ow_2438.c
            humid = (v.vad/v.vdd-(0.8/v.vdd)) /
                      (0.0062*(1.0305+0.000044*temp+0.0000011*v.temp*v.temp));

            if (w1->verbose)
            {
                fprintf(stderr, "vdd %f vad %f temp %f rh %f\n",
                        v.vdd, v.vad, v.temp, humid);
            }

            w1_sensor_t *s;
/*
 *  Not necessary, done read_voltages
        if((s = w1_match_sensor(w, "Temp")))
        {
            s->value = v.temp;
            nv += w1_validate(w1, s);
        }
*/
            if((s = w1_match_sensor(w, "Humidity")))
            {
                if (humid > 100)
                    humid = 99.9;
                s->value = humid;
                nv += w1_validate(w1, s);
            }
        }
        if(w->s[0].valid == 0 || w->s[1].valid == 0 )
        {
            w1_replog (w1, "hih nv1 %d %d %f %f %f %f", nv1, nv, vdd, vad, temp, humid);
            w->s[0].valid = w->s[1].valid = 0;
        }
    }
    return nv;
}

static int w1_read_current (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }

    w1_set_invalid(w);
    if(w1_select_device(w1, w))
    {
        ds2438v_t v = {0};
        v.req = DS2438_VAD;
        nv = w1_read_voltages(w1, w, &v);

        if (v.vad > -1.0)
        {
            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "current")))
            {
                if (v.vad >= 0.2)
                {
                        /*
                         * for voltages .2 and up, response is claimed
                         * to be linear within +/- 3% at a temperature
                         * of 23C.  .2 volts corresponds to a current
                         * of 1 Amp.  7.36x -.47 for x > 0.2volts
                         */
                    s->value = (v.vad * 7.36)-0.47;
                }
                else
                {
                        /*
                         * 9.09x - .8181 for x < 0.2
                         * This is an adjusted slope that will give zero amps
                         * at the minimum voltage of 0.09 volts and yield 1 amp
                         * at 0.2 volts where it will meet up with the
                         * first equation.
                         * The readings between 0 and 1 amp may be inaccurate.
                         */
                    s->value = (v.vad * 9.09)-0.8181;
                }
                nv += w1_validate(w1, s);
            }
        }
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
                    float prtemp;
                    prtemp = (w1->pres_reduction_temp) ?
                        *w1->pres_reduction_temp : temp ;
                    pres = pressure_at_msl(pres, prtemp, w1->altitude);
                }

                w1_sensor_t *s;
                if((s = w1_match_sensor(w, "Temp")))
                {
                    s->value = temp;
                    nv += w1_validate(w1, s);
                }

                if((s = w1_match_sensor(w, "Pres")))
                {
                    s->value = pres;
                    nv += w1_validate(w1, s);
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
        w->init  = 1;
    }
    return nv;
}

static int w1_read_ds2760 (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    ds2760_t v = {0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);

    if(w1_select_device(w1,w))
    {
        if(ReadDS2760(w1->portnum, w->serno, &v))
        {
            w1_sensor_t *s;
            if(w1->verbose)
            {
                fprintf(stderr,"ReadDS2760: v %f t %f i %f a %f\n",
                        v.volts, v.temp, v.curr, v.accum);
            }

            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = v.temp;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Current")))
            {
                s->value = v.curr;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Volt")))
            {
                s->value = v.volts;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Accumulator")))
            {
                s->value = v.accum;
                nv += w1_validate(w1, s);
            }
        }
    }
    return nv;
}

static int w1_read_ds1923 (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    ds1923_t v = {0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);

    if(w1_select_device(w1,w))
    {
        if(w->params)
        {
            v.kill = 1;
        }

        if(ReadDS1923(w1->portnum, w->serno, &v))
        {
            w1_sensor_t *s;
            if(w1->verbose)
            {
                fprintf(stderr,"ReadDS1923: t %f°C rh %f%%\n", v.temp, v.rh);
                fflush(stderr);
            }

            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = v.temp;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Humidity")))
            {
                s->value = v.rh;
                nv += w1_validate(w1, s);
            }

        }
        else if(w1->verbose)
        {
            fputs("ReadDS1923 read failed\n", stderr);
            fflush(stderr);
        }
    }
    return nv;
}

static int w1_read_ds1921 (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    ds1921_t v = {0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);

    if(w1_select_device(w1,w))
    {
        if(w->params)
        {
            v.kill = 1;
        }

        if(ReadDS1921(w1->portnum, w->serno, &v))
        {
            w1_sensor_t *s;
            if(w1->verbose)
            {
                fprintf(stderr,"ReadDS1921:  %f°C\n", v.temp);
            }

            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = v.temp;
                nv += w1_validate(w1, s);
            }
        }
    }
    return nv;
}

static int w1_read_hbuv (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    hbuv_t hb={0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);
    if(w1_select_device(w1,w))
    {
        int r;
        r = HBUV_read_data(w1->portnum, w->serno, &hb);
        if(r)
        {
            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = hb.raw_temp * 0.5;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "uv")) ||
               (s = w1_match_sensor(w, "ultra")) ||
               (s = w1_match_sensor(w, "violet")) )
            {
                s->value = hb.raw_uvi * 0.1;
                nv += w1_validate(w1, s);
            }
        }
    }
    return nv;
}

static int w1_read_hbht (w1_devlist_t *w1, w1_device_t *w)
{
    int nv = 0;
    hbht_res_t hb={0};

    if(w->init == 0)
    {
        w1_make_serial(w->serial, w->serno);
        w->init = 1;
    }
    w1_set_invalid(w);
    if(w1_select_device(w1,w))
    {
        int r;
        r = HBHT_read_data(w1->portnum, w->serno, &hb);
        if(w1->verbose)
            fprintf(stderr,"HBHT %d %.1f %.2f\n", r, hb.temp, hb.humid);
        if(r)
        {
            w1_sensor_t *s;
            if((s = w1_match_sensor(w, "Temp")))
            {
                s->value = hb.temp;
                nv += w1_validate(w1, s);
            }

            if((s = w1_match_sensor(w, "Humidity")))
            {
                s->value = hb.humid;
                nv += w1_validate(w1, s);
            }
        }
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
            char *inuse = calloc(w->ns, sizeof(char));
            int empty = 0;
            w1_make_serial(w->serial, w->serno);
	    w->private = calloc(1, sizeof(w1_coupler_private_t));
	    priv = (w1_coupler_private_t*)w->private;
	    priv->active_lines = COUPLER_UNDEFINED;

            for(b = 0; b < w->ns; b++)
            {
//                fprintf(stderr, "%d/%d: ", b,w->ns);
                if(w->s[b].abbrv && w->s[b].name)
                {
//                    fprintf(stderr,"%4s %s", w->s[b].abbrv, w->s[b].name);
                    if ((w->s[b].abbrv[0] == 'M' || w->s[b].abbrv[0] == 'A')
                        && isxdigit(w->s[b].name[0]))
                    {
                        char *tmp, *p1, *p2;
                        tmp = strdup(w->s[b].name);
                        for(p1 = tmp; (p2 = strtok(p1,", |"));p1=NULL)
                        {
                            if(*p2)
                            {
                                clist[nc].coupler_device = w;
                                strcpy(clist[nc].devid, p2);
                                if(w->s[b].abbrv[0] == 'M')
                                    clist[nc].branch = COUPLER_MAIN_ON;
                                else
                                    clist[nc].branch = COUPLER_AUX_ON;
                                nc++;
                            }
                        }
                        free(tmp);
                        inuse[b] = 1;
                    }
                    else
                    {
                        empty++;
                    }
                }
                else
                {
                    empty++;
                }
//                fputc('\n', stderr);
            }
            if(empty)
            {
                int i,j;
                w1_sensor_t *temps = calloc(w->ns - empty, sizeof( w1_sensor_t));
                for (i = 0, j= 0; i < w->ns; i++)
                {
                    if(inuse[i] == 1)
                    {
                        memcpy(temps+j, w->s+i, sizeof( w1_sensor_t));
                        j++;
                    }
                }
                void * old = w->s;
                w->s = temps;
                w->ns -= empty;
                free(old);
                fprintf(stderr, "(notice) freed up empty branch for %s\n",
                        w->serial);
            }
            free(inuse);
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

static int w1_lcm(int a[], int n)
{
    int i,j,c,amax;
    unsigned long prod;
    int s = -1;

    amax=a[0];
    for(i=0;i<n;i++)
    {
        if(a[i]>=amax)
        {
            amax=a[i];
        }
    }

    for(i=0,prod=1;i<n;i++)
    {
        prod=prod*a[i];
    }

    for(i=amax;i<=prod;i+=amax)
    {
        c=0;
        for(j=0;j<n;j++)
        {
            if(i%a[j]==0)
            {
                c+=1;
            }
        }

        if(c==n)
        {
            s = i;
            break;
        }
    }
    return s;
}


static int _w1_gcd (int m, int n)
{
    if (n == 0) {
        return m;
    } else {
        return _w1_gcd(n, m % n);
    }
}

int w1_gcd (int a[], int n)
{
    int s = 0;
    int i;

    for(i=0; i < n; i++)
    {
        s = _w1_gcd(s,a[i]);
    }
  return s;
}

void w1_verify_intervals (w1_devlist_t *w1)
{
    int n;
    int mint = w1->delay;
    w1_device_t *d;
    int *intvls;

    intvls = malloc(w1->numdev*sizeof(int));
    for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
    {
        if(d->intvl)
        {
            if(d->intvl < W1_MIN_INTVL)
                d->intvl = W1_MIN_INTVL;
            if(d->intvl > w1->delay)
                d->intvl = w1->delay;
        }
        else
        {
            d->intvl = w1->delay;
        }
        if (d->intvl < mint)
            mint = d->intvl;
    }
    w1->delay = mint;
    for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
    {
        d->intvl = (d->intvl / w1->delay) * w1->delay;
        if (d->intvl == 0) d->intvl = w1->delay;
        intvls[n] = d->intvl;
    }
    w1->cycle = w1_lcm(intvls, w1->numdev);
    w1->delay = w1_gcd(intvls, w1->numdev);
    free(intvls);
}


int w1_read_all_sensors(w1_devlist_t *w1, time_t secs)
{
    int nv = 0, r;

    if(w1->doread)
    {
        w1_device_t *d;
        int n;

        if(w1->portnum == -1)
        {
            if((w1->portnum = owAcquireEx(w1->iface)) < 0)
            {
                OWERROR_DUMP(stdout);
                exit(1);
            }
        }

        for(d = w1->devs, n = 0; n < w1->numdev; n++, d++)
        {
            int k;
            for(k = 0; k < d->ns; k++)
            {
                d->s[k].valid = 0;
            }
            if(secs == 0 || d->intvl == 0 || ((secs % d->intvl) == 0))
            {
                int rtry;

                for(rtry = 0; rtry < W1_RETRY; rtry++)
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
                            r = w1_read_voltages(w1, d, NULL);
                            break;

                        case W1_HIH:
                            r = w1_read_hih(w1, d);
                            break;

                        case W1_DS2760:
                            r = w1_read_ds2760(w1, d);
                            break;

                        case W1_DS2450:
                            r = w1_read_ds2450(w1, d);
                            break;

                        case W1_MS_TC:
                            r = w1_read_current(w1, d);
                            break;

                        case W1_DS1921:
                            r = w1_read_ds1921(w1, d);
                            break;

                        case W1_DS1923:
                            r = w1_read_ds1923(w1, d);
                            break;

                        case W1_HBUV:
                            r = w1_read_hbuv(w1, d);
                            break;

                        case W1_HBHT:
                            r = w1_read_hbht(w1, d);
                            break;

                        case W1_INVALID:
                        default:
                            r = -1;
                            break;
                    }
                    if(r != 0)
                        break;
                    else
                    {
                        w1_replog (w1, "retry %d %s", rtry, d->serial);
			usleep(50*1000);
                    }

                }
                if(r >= 0)
                {
                    nv += r;
                    if(r == 0)
                        syslog(LOG_WARNING, "Failed to read sensor %s (type %s)", d->serial, d->devtype);
                }
            }
        }
            /* Put the bus back in a known state */
	w1_all_couplers_off(w1);
    }
    return nv;
}
