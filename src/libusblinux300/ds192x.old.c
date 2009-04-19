//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES 
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of Dallas Semiconductor 
// shall not be used except as stated in the Dallas Semiconductor 
// Branding Policy. 
//--------------------------------------------------------------------------
//
//  ds192x.c - Reads the stuff on the 1-Wire of the DS1921 and DS1923. 
//  modified from humalog.c and thermodl.c
//

// Include Files
#include <stdio.h>
#include "ownet.h"
#include "humutil.h"
#include "ds192x.h"


int ReadDS1921(int portnum, uchar *snum, ds1921_t *v)
{
    int ret = 0;
    
    owSerialNum(portnum,snum,FALSE);
    if (owAccess(portnum))
    {
    }
    return ret;
}

int ReadDS1923(int portnum, uchar *snum, ds1923_t *v)
{
    int ret = 0;
    uchar state[96];
    double val,valsq,error;
    configLog config;

        // set up config for DS1923
    config.configByte = 0x20;
  
    if(readDevice(portnum,snum,state,&config)==0)
    {
        if(0 != doTemperatureConvert(portnum,snum,state))
        {
            ret = -1;
        }
        else
        {
            config.useTemperatureCalibration = 1;
            val = decodeTemperature(&state[12],2,FALSE,config);
            valsq = val*val;
            error = config.tempCoeffA*valsq +
                config.tempCoeffB*val + config.tempCoeffC;
            v->temp = val - error;
            
            config.useHumidityCalibration = 1;
            
            val = decodeHumidity(&state[14],2,FALSE,config);
            valsq = val*val;
            error = config.humCoeffA*valsq +
                config.humCoeffB*val + config.humCoeffC;
            v->rh = val - error;
            
            if(v->rh < 0.0)
                v->rh = 0.0;
        }
    }
    else
    {
        ret = -1 ;
    }
    return ret;
}

