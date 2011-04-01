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
//---------------------------------------------------------------------------
//
//  ibshaut.c - Utility functions for SHA iButton applications
//
//  Version:   2.00
//  History:
//

#include "ownet.h"
#include "ibsha18.h"

// external functions
extern int ReadUDP_SHA(int,ushort,uchar *,int *);
extern int WritePage(int, uchar *, int, TransState *);
extern int EnterString(char *, char *, int, int);
extern int EnterNum(char *, int, int *, int, int);
extern int ToHex(char ch);

// local function prototypes
int PrintHeader(char *);
void output_status(int, char *);
int GetSeviceProviderSettings(int, char *, uchar *, int, uchar *);
int GetRovingSettings(uchar *, uint *);

// globals
extern int VERBOSE;
extern FILE *fp;

//--------------------------------------------------------------------------
// Prints a message, and the current driver versions.  Return TRUE
// driver version were found and FALSE if not.
//
int PrintHeader(char *title)
{
   char msg[512];
   int rt=TRUE;

   // print a header with driver version info
   sprintf(msg,"\n%s\n",title);
   output_status(LV_ALWAYS,msg);
   output_status(LV_ALWAYS,"Dallas Semiconductor Corporation\n");
   output_status(LV_ALWAYS,"Copyright (C) 1992-2000\n\n");
   output_status(LV_ALWAYS,"Press CTRL-C to end program\n\n");

   return rt;
}


//--------------------------------------------------------------------------
//  output status message
//
void output_status(int level, char *st)
{
   // skip null strings
   if (st[0] == 0)
      return;

   // check if in verbose mode
   if ((level >= LV_ALWAYS) || VERBOSE)
   {
      printf("%s",st);
      if(fp != NULL)
         fprintf(fp,"%s",st);
   }
}

//----------------------------------------------------------------------
// Get the settings for the Service Provider
//
int GetSeviceProviderSettings(int get_sp, char *sp_name, uchar *auth_secret, int get_money, uchar *money_secret)
{
   char temp[255];
   int i,len,cnt;

   // instructions
   output_status(LV_ALWAYS,"Default values specified in ()\n"
                "Hit (CTRL-C or ESCAPE and then ENTER) to abort\n");

   // service provider name
   if (get_sp)
   {
      sprintf(sp_name,"DLSM");
      len = EnterString("\nEnter the service provider name",sp_name,1,4);
      if (len < 0)
         return FALSE;
      else
      {
         // file with spaces to make easier on compare
         for (i = len; i < 4; i++)
            sp_name[i] = ' ';
         sp_name[4] = 0;
         sprintf(temp,"Service provider name selected: %s\n",sp_name);
         output_status(LV_ALWAYS,temp);
      }
   }

   // auth secret
   sprintf(temp,"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
   len = EnterString("\nEnter service provider AUTHENTICATE secret (hex)\n",temp,2,64);
   if (len < 0)
      return FALSE;
   else
   {
      // convert to byte array with FF padding
      for (i = 0; i < len; i += 2)
      {
         auth_secret[i / 2] = ToHex(temp[i]) << 4;
         auth_secret[i / 2] |= ToHex(temp[i + 1]);
      }
      for (i = len; i < 64; i += 2)
         auth_secret[i / 2] = 0xFF;

      cnt = sprintf(temp,"Service provider AUTHENTICATE secret entered (hex):\n ");
      for (i = 0; i < 32; i++)
         cnt += sprintf(temp + cnt,"%02X",auth_secret[i]);
      cnt += sprintf(temp + cnt,"\n");
      output_status(LV_ALWAYS,temp);
   }

   // money secret
   if (get_money)
   {
      sprintf(temp,"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      len = EnterString("\nEnter service provider MONEY secret (hex)\n",temp,2,64);
      if (len < 0)
         return FALSE;
      else
      {
         // convert to byte array with FF padding
         for (i = 0; i < len; i += 2)
         {
            money_secret[i / 2] = ToHex(temp[i]) << 4;
            money_secret[i / 2] |= ToHex(temp[i + 1]);
         }
         for (i = len; i < 64; i += 2)
            money_secret[i / 2] = 0xFF;

         cnt = sprintf(temp,"Service provider MONEY secret entered (hex):\n ");
         for (i = 0; i < 32; i++)
            cnt += sprintf(temp + cnt,"%02X",money_secret[i]);
         cnt += sprintf(temp + cnt,"\n");
         output_status(LV_ALWAYS,temp);
      }
   }

   return TRUE;
}


//----------------------------------------------------------------------
// Get the settings for the Service Provider
//
int GetRovingSettings(uchar *rov_information, uint *money)
{
   char temp[255];
   int i,len;

   // instructions
   output_status(LV_ALWAYS,"Default values specified in ()\n"
                "Hit (CTRL-C or ESCAPE and then ENTER) to abort\n");

   // user information
   rov_information[29] = 0;
   len = EnterString("\nEnter roving user fixed information\n",(char*)rov_information,0,28);
   if (len < 0)
      return FALSE;
   else
   {
      rov_information[len] = 0;
      sprintf(temp,"Roving user fixed information: %s\n",rov_information);
      output_status(LV_ALWAYS,temp);

      // pad the user information with spaces up to 28 characters
      for (i = len; i < 28; i++)
         rov_information[i] = ' ';
      // pad rest with zeros (money page)
      for (i = 28; i < 56; i++)
         rov_information[i] = 0;
   }

   // money (1.02) change to dollars
   if (!EnterNum("\nEnter the starting money balance (in US dollars)\n", 5, (int*)money, 0, 99999))
      return FALSE;
   else
   {
      sprintf(temp,"Starting balance: $%ld US\n",*money);
      output_status(LV_ALWAYS,temp);
   }

   return TRUE;
}



