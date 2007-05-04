/*
 * pressure.c -- TAI8570 interface for w1retap
 *  (c) 2005 Jonathan Hudson <jh+w1retap@daria.co.uk>
 *
 * Substansially derived from the tai8570.c file in the archive
 * tai8570sjm102_300.zip archive available from
 * http://oww.sourceforge.net/download/tai8570sjm102_300.zip via
 * http://www.aagelectronica.com/aag/ * tai8570.c
 *
 * tai8570.c contains the following notice:
 * -------------------------------------------------
 * tai8570.c
 * Version 1.02
 *
 * Simon Melhuish
 * http://melhuish.info/simon/
 * http://oww.sourceforge.net/
 *
 * Derived from source by AAG
 * ------------------------------------------------- 
 *
 * Other parts derived from the Dallas PD kit, so let's assume their
 * PD licence.
 *
 * Jonathan Hudson removed unused code, reformatted to his favourite
 * layout and added API entries suitable for w1retap.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ownet.h"
#include "swt12.h"
#include "owfile.h"

/**
 *
 * @author  Aitor
 */

#define SEC_READW4     "111011010000"
#define SEC_READW2     "111010110000"
#define SEC_READW1     "111010101000"
#define SEC_READW3     "111011001000"

#define SEC_READD1     "11110100000"
#define SEC_READD2     "11110010000"
#define SEC_RESET      "101010101010101000000"

typedef unsigned char byte;
typedef int bool;
#define false 0
#define true  1

static u_char SwitchSN[2][8];
static int reader=-1, writer;

static char SW1[20], SW2[20], SW3[20], SW4[20];

static byte CFG_READ = (byte) 0xEC;	// '11101100'   Configuración de lectura para DS2407
static byte CFG_WRITE = (byte) 0x8C;	// '10001100'  Configuración de Escritura para DS2407
static byte CFG_READPULSE = (byte) 0xC8;	// '11001000'  Configuración de lectura de Pulso de conversion para DS2407

static int C1 = 0, C2 = 0, C3 = 0, C4 = 0, C5 = 0, C6 = 0;
static int D1 = 0, D2 = 0;

static void SendPressureBit (int portnum, bool i);
static bool CheckConversion (int portnum);


//----------------------------------------------------------------------
//      SUBROUTINE - WriteSwitch12 - Adapted from SetSwitch12 Dallas code
//
//  This routine sets the channel state of the specified DS2406
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number was provided to
//                 OpenCOM to indicate the port number.
// 'SerialNum'   - Serial Number of DS2406 to set the switch state
// 'st'          - Config byte to write
//
//  Returns: TRUE(1)  State of DS2406 set and verified
//           FALSE(0) could not set the DS2406, perhaps device is not
//                    in contact
//
int WriteSwitch12 (int portnum, uchar * SerialNum, ushort st)
{
    int rt = FALSE;
    uchar send_block[30];
    int send_cnt = 0;

    setcrc16 (portnum, 0);

	// set the device serial number to the counter device
    owSerialNum (portnum, SerialNum, FALSE);

	// access the device
    if (owAccess (portnum))
    {
            // create a block to send that reads the counter
            // write status command
        send_block[send_cnt++] = 0x55;
        docrc16 (portnum, 0x55);
        
            // address of switch state
        send_block[send_cnt++] = 0x07;
        docrc16 (portnum, 0x07);
        send_block[send_cnt++] = 0x00;
        docrc16 (portnum, 0x00);

        send_block[send_cnt++] = (uchar) st;
        docrc16 (portnum, st);

            // read CRC16
        send_block[send_cnt++] = 0xFF;
        send_block[send_cnt++] = 0xFF;
        
            // now send the block
        if (owBlock (portnum, FALSE, send_block, send_cnt))
        {
                // perform the CRC16 on the last 2 bytes of packet
            docrc16 (portnum, send_block[send_cnt - 2]);
            
                // verify crc16 is correct
            if (docrc16 (portnum, send_block[send_cnt - 1]) ==
                0xB001)
                rt = TRUE;
        }
    }

	// return the result flag rt
    return rt;
}

// Open (i.e. o/p goes high) the channel A switch
static int OpenA (int portnum, uchar * SerialNum)
{
    byte I = 0XFF, res = 0;
    
    int Ind = 5;
    do
    {
        owSerialNum (portnum, SerialNum, FALSE);
        I = ReadSwitch12 (portnum, false);
        WriteSwitch12 (portnum, SerialNum, I | 0X20);
        I = ReadSwitch12 (portnum, false);
        Ind--;
        res = (I & 0XDF);
        if (res != 0)
            return true;
    }
    while (Ind > 0);
    return (Ind > 0);
}

// Open (i.e. o/p goes high) the channel B switch
static int OpenB (int portnum, uchar * SerialNum)
{
    byte I = 0XFF, res = 0;
    
    int Ind = 5;
    do
    {
        owSerialNum (portnum, SerialNum, FALSE);
        I = ReadSwitch12 (portnum, false);
        WriteSwitch12 (portnum, SerialNum, I | 0X40);
        I = ReadSwitch12 (portnum, false);
        Ind--;
        res = (I & 0XBF);
        if (res != 0)
            return true;
    }
    while (Ind > 0);
    return (Ind > 0);
}

// Open B (data in/out) on both reader and writer
static bool OpenPIOS_B (int portnum)
{
    if (!OpenB (portnum, SwitchSN[reader]))
        return FALSE;
    if (!OpenB (portnum, SwitchSN[writer]))
        return FALSE;
    return TRUE;
}

// Open A (clock) on both reader and writer
static bool OpenPIOS_A (int portnum)
{
    if (!OpenA (portnum, SwitchSN[reader]))
        return FALSE;
    if (!OpenA (portnum, SwitchSN[writer]))
        return FALSE;
    
    return TRUE;
}

// Send a Channel Access command sequence
static int ChannelAccess (int portnum, uchar * SerialNum,
                          byte AChanC1, byte AChanC2, byte * ACInfo)
{
    owSerialNum (portnum, SerialNum, FALSE);
    if (!owAccess (portnum))
    {
        fprintf (stderr, "owAccess failed in ChannelAccess\n");
        return false;
    }
    owTouchByte (portnum, 0XF5);
    owTouchByte (portnum, AChanC1);
    owTouchByte (portnum, AChanC2);
    *ACInfo = owTouchByte (portnum, 0XFF);
    return true;
}

//---------------------------------------------------------------------------
static int ConfigWrite (int portnum)
{
    byte ACInfo = 0;
    return ChannelAccess (portnum, SwitchSN[writer], CFG_WRITE, 0xFF,
                          &ACInfo);
}

//---------------------------------------------------------------------------
static int ConfigRead (int portnum)
{
    byte ACInfo = 0;
    return ChannelAccess (portnum, SwitchSN[reader], CFG_READ, 0xFF,
                          &ACInfo);
}

//---------------------------------------------------------------------------
static int ConfigReadPulse (int portnum)
{
    byte ACInfo = 0;
    return ChannelAccess (portnum, SwitchSN[reader], CFG_READPULSE, 0xFF,
                          &ACInfo);
}

static bool WritePressureSensor (int portnum, char *Cmd)
{
    bool rslt = false;
    int lng, i;
    lng = strlen (Cmd);
    if (ConfigWrite (portnum))
    {
        for (i = 0; i < lng; i++)
        {
            if (Cmd[i] == '0')
                SendPressureBit (portnum, false);
            else
                SendPressureBit (portnum, true);
        }
        SendPressureBit (portnum, false);
        rslt = true;
    }
    return rslt;
}

static void SendPressureBit (int portnum, bool i)
{
    if (i)
        owTouchByte (portnum, 0x0e);
    else
        owTouchByte (portnum, 0x04);
}

    /******************************************************/

static bool ReadPressureBit (int portnum)
{
    int res = 0;
    res = owReadByte (portnum);
    res &= 0x80;
    owWriteByte (portnum, 0xfa);
    return (res != 0);
}

/******************************************************/
static char * ReadPressureSensor (int portnum, byte CountBit)
{
    static char StringBuffer[64];
    int i;

    strcpy (StringBuffer, "");
    if (ConfigRead (portnum))
    {
        for (i = 0; i < CountBit; i++)
        {
            if (ReadPressureBit (portnum))
                strcat (StringBuffer, "1");
            else
                strcat (StringBuffer, "0");
        }
    }
    else
    {
        fprintf (stderr, "ConfigRead failed\n");
    }
    return StringBuffer;
}

/******************************************************/
static bool PressureReset (int portnum)
{
    bool rsl = false;
    if (!OpenPIOS_A (portnum))
        return false;	// Inspired by AAG C++ version
    if (!ConfigWrite (portnum))
        return false;	// Inspired by AAG C++ version
    rsl = WritePressureSensor (portnum, SEC_RESET);
    return rsl;
}

/******************************************************/
static char *ReadSensorValueStr (int portnum, char *cmd, char *Lect)
{
    Lect[0] = '\0';
    
    if (!OpenPIOS_A (portnum))
        return FALSE;
    if (WritePressureSensor (portnum, cmd))
    {
        bool t;
        if (!OpenPIOS_A (portnum))
            return FALSE;
        strcpy (Lect, ReadPressureSensor (portnum, (byte) 16));
        t = OpenPIOS_B (portnum);
    }
    else
    {
        fprintf (stderr, "WritePressureSensor failed\n");
    }
    return Lect;
}

/******************************************************/
static int ReadSensorValue (int portnum, char *Comm)
{
    char Lect[30];
    int Res = 0;
    int i = 0;

    Lect[0] = 0;    
    if (!PressureReset (portnum))
        return 0;
    if (!WritePressureSensor (portnum, Comm))
        return 0;
    if (!CheckConversion (portnum))
        return 0;
    if (!OpenPIOS_A (portnum))
        return 0;
    strcpy (Lect, ReadPressureSensor (portnum, 16));
    
    if (!OpenPIOS_B (portnum))
        return 0;
    
    if (strlen (Lect) != 16)
        return 0;
    for (i = 0; i < 16; i++)
    {
        Res = (Res << 1);
        if (Lect[i] == '1')
            ++Res;
    }
    return Res;
}

static bool CheckConversion (int portnum)
{
    int i;
    if (!ConfigReadPulse (portnum))
    {
        fprintf (stderr,
                 "ConfigReadPulse failed in CheckConversion\n");
        return false;
    }

    for (i = 0; i < 100; i++)
        if (!owTouchBit (portnum, 1))
            break;
    return (i < 100);
}

/******************************************************/

static bool ReadSensor_Calibration (int portnum)
{
    int uno = 1, im = 0, Count = 0;
    int i = 0;
    int Lect = false;
    char SW11[20] = "0", SW22[20] = "0", SW33[20] = "0", SW44[20] = "0";

    do
    {
        if (!PressureReset (portnum))
            return false;
        ReadSensorValueStr (portnum, SEC_READW1, SW1);
        ReadSensorValueStr (portnum, SEC_READW2, SW2);
        ReadSensorValueStr (portnum, SEC_READW3, SW3);
        ReadSensorValueStr (portnum, SEC_READW4, SW4);

        if (strlen (SW1) != 16)
            return false;
        if (strlen (SW2) != 16)
            return false;
        if (strlen (SW3) != 16)
            return false;
        if (strlen (SW4) != 16)
            return false;
        
        Lect = ((strcmp (SW1, SW11) == 0)
                && (strcmp (SW2, SW22) == 0))
            && ((strcmp (SW3, SW33) == 0)
                && (strcmp (SW4, SW44) == 0));
        
        strcpy (SW11, SW1);
        strcpy (SW22, SW2);
        strcpy (SW33, SW3);
        strcpy (SW44, SW4);
        
        Count++;
    }
    while ((Count < 5) && (Lect == false));

    C1 = 0;
    C2 = 0;
    C3 = 0;
    C4 = 0;
    C5 = 0;
    C6 = 0;
    
    for (i = 14; i >= 0; i--)
    {
        if (SW1[i] == '1')
        {
            C1 = C1 | (uno << (14 - i));
        }
    }

    for (i = 9; i >= 0; i--)
    {
        if (SW2[i] == '1')
            C5 = C5 | (uno << (9 - i));
    }
    if (SW1[15] == '1')
        C5 = C5 | (uno << (10));
    
    for (i = 15; i >= 10; i--)
    {
        if (SW2[i] == '1')
            C6 = C6 | (uno << (15 - i));
    }
    
    for (i = 9; i >= 0; i--)
    {
        if (SW3[i] == '1')
            C4 = C4 | (uno << (9 - i));
    }
    
    for (i = 9; i >= 0; i--)
    {
        if (SW4[i] == '1')
            C3 = C3 | (uno << (9 - i));
    }
    
    im = 0;
    for (i = 15; i >= 10; i--)
    {
        if (SW4[i] == '1')
            C2 = C2 | (uno << (im));
        im++;
    }
    
    for (i = 15; i >= 10; i--)
    {
        if (SW3[i] == '1')
            C2 = C2 | (uno << (im));
        im++;
    }
    return true;
}

/******************************************************/

int ReadPressureValues (int portnum, float *temp, float *pressure)
{
	double DT, OFF, SENSE, X;
	D1 = ReadSensorValue (portnum, SEC_READD1);
	D2 = ReadSensorValue (portnum, SEC_READD2);
	DT = D2 - ((8 * C5) + 20224.0);
	OFF = C2 * 4 + (((C4 - 512) * DT) / 4096);
	SENSE = 24576 + C1 + ((C3 * DT) / 1024);
	X = ((SENSE * (D1 - 7168)) / 16384) - OFF;
	*pressure = 250.0 + X / 32;
	*temp = 20.0 + ((DT * (C6 + 50)) / 10240);
	return true;
}

static int find_press_roles(int portnum, u_char *serno)
{
    int found = 0, fl_len;
    short hnd;
    FileEntry fe ;
    uchar buf[100];
    int j;
    
    for(j =0; j < 2 && found == 0; j++)
    {
        reader = writer = -1;
        memcpy(SwitchSN[0], serno, sizeof(SwitchSN[0]));    
        owSerialNum (portnum, SwitchSN[0], FALSE);
        memset(&fe, 0, sizeof(fe));
        memcpy(fe.Name, "8570", 4) ;

        found = owOpenFile(portnum, SwitchSN[0], &fe, &hnd) ;
        if (found)
        {
            int info;
            owReadFile(portnum, SwitchSN[0], hnd, buf, 99, &fl_len) ;
        
            owSerialNum (portnum, SwitchSN[0], false);
            owCloseFile(portnum, SwitchSN[0], hnd);        
            
            info = ReadSwitch12(portnum, false) ;
            if (info == -1)
            {
                fprintf(stderr, "Unable to read info byte\n") ;
                found = 0;
            }
            else
            {
                if ((info & 0x80) == 0)
                {
                    reader=0;
                    writer=1;
                }
                else
                {
                    reader=1;
                    writer=0;
                }
                memcpy(SwitchSN[1], buf, sizeof(SwitchSN[1]));
            }
        }
        
    }
    return found;
}

int Init_Pressure(int portnum, u_char *serno)
{
    int iscalib = false;
    if(find_press_roles(portnum,serno))
    {
        if (ReadSensor_Calibration (portnum))
        {
            iscalib = true;
        }
    }
    return iscalib;
}


