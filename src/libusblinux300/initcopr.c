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
//  initcopr.c - This utility initializes the SHA iButton
//              (DS1963S) as a coprocessor.
//
//  Version:   2.00
//  History:
//

#include "ownet.h"
#include "ibsha18.h"

// external function prototypes
extern int PrintHeader(char *);
extern void output_status(int, char *);
extern int GetSeviceProviderSettings(int,char *,uchar *,int,uchar *);
extern int WriteFileSHA(uchar *,uchar *,int,uchar *,TransState *);
extern void ExitProg(char *, int);
extern int  owAcquire(int,char *);
extern void owRelease(int);
extern int CreateAuthSecret(uchar *, TransState *);
extern int CreateMoneySecret(uchar *, TransState *);
extern int SelectCoProcessor(TransState *);
extern int FindSHA(uchar *, TransState *);
extern int getkeystroke(void);

// verbose output mode
int VERBOSE=0;
// file for output
FILE *fp;

//----------------------------------------------------------------------
//  This is the Main routine for initcopr
//
int main(int argc, char **argv)
{
   int filenum;
   char msg[200];
   uchar auth_secret[65],money_secret[65];
   int cnt,exit_code,i;
   uchar buf[6],flname[6],page_map[1];
   TransState ts;

   // check arguments to see if request instruction with '?' or too many
   if ((argc > 4) || (argc < 2) || ((argc > 1) && (argv[1][0] == '?' || argv[1][1] == '?')))
       ExitProg("\nusage: initcopr 1wire_net_name <output_filename> </Verbose>\n"
              "  - initialize a coprocessor SHA iButton on specified\n"
              "    1-Wire Net port\n"
              "  - 1wire_net_port - required port name\n"
              "    example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" \n"
              "    (Linux DS2480),\"1\" (Win32 TMEX)\n"
              "  - <output_filename> optional output filename\n"
              "  - </V> optional show verbose output (default not shown)\n"
              "  - version 2.00\n",EXIT_ERROR);

   // attempt to acquire the 1-Wire Net
   ts.portnum[IB_ROVING] = 0;
   if (!owAcquire(ts.portnum[IB_ROVING],argv[1]))
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // hard code CO_PROCESSOR port to be the same
   ts.portnum[IB_COPR] = ts.portnum[IB_ROVING];

   // print the header info
   if (!PrintHeader("InitCoPr Version 2.00"))
      ExitProg("No driver found\n",EXIT_ERROR);

   // check arguments
   VERBOSE = FALSE;
   filenum = 0;
   if (argc >= 3)
   {
      if (argv[2][0] != '/')
         filenum = 2;
      else if ((argv[2][1] == 'V') || (argv[2][1] == 'v'))
         VERBOSE = TRUE;

      if (argc == 4)
      {
         if (argv[3][0] != '/')
            filenum = 3;
         else if ((argv[3][1] == 'V') || (argv[3][1] == 'v'))
            VERBOSE = TRUE;
      }
   }

   // open the output file
   fp = NULL;
   if (filenum > 0)
   {
      fp = fopen(argv[filenum],"wb");
      if(fp == NULL)
      {
         printf("ERROR, Could not open output file!\n");
         exit(EXIT_ERROR);
      }
      else
         printf("File '%s' opened to write data.\n\n",
                 argv[filenum]);
   }

   // set the default pages used for secrets (will be written to copr)
   ts.c_mmaster_scrt = 0;  // co-processor money master secret
   ts.c_amaster_scrt = 1;  // co-processor auth master secret
   ts.c_udevice_scrt = 2;  // co-processor unique device secret(calculated)
   ts.r_udevice_scrt = 5;  // roving unique device secret
   ts.t_udevice_scrt = 4;  // till unique device secret
   sprintf((char *)(&ts.copr_file[0]),"COPR");

   // prompt for service provider settings
   output_status(LV_ALWAYS,"Enter information to create the co-processor SHA"
                " iButton for a service\n");
   if (!GetSeviceProviderSettings(TRUE,(char*)ts.provider,auth_secret,TRUE, money_secret))
      ExitProg("Aborted entering the provider settings, end program\n",EXIT_ERROR);

   // create the data for the TMEX file
   sprintf((char*)flname,"%s",ts.copr_file);  // first 4 letters, service name
   sprintf((char *)buf,"%s",ts.provider);
   buf[4] = 102;                       // extension of SHA money file
   buf[5] = ts.c_mmaster_scrt;         // money master secret number
   buf[6] = ts.c_amaster_scrt;         // auth master secret number
   buf[7] = ts.c_udevice_scrt;         // unique device secret(calculated)
   page_map[0] = 1;                    // page map for only 1 file page

   // get warning
   output_status(LV_ALWAYS,"\n** Warning SHA iButton on 1-Wire will be overwritten!!\n  (Press ENTER to continue)\n");
   getkeystroke();

   // select co-processor port
   if (SelectCoProcessor(&ts))
   {
      output_status(LV_ALWAYS,"** 1-Wire port for co-processor selected\n");

      // find the first available SHA iButton
      if (FindSHA(&ts.copr_rom[0], &ts))
      {
         // loop to print ROM of device found
         cnt = sprintf(msg,"** SHA iButton found: ");
         for (i = 7; i >= 0; i--)
            cnt += sprintf(msg + cnt,"%02X",ts.copr_rom[i]);
         sprintf(msg + cnt,"\n");
         output_status(LV_ALWAYS,msg);

         // create the authentication secret
         if (CreateAuthSecret(&auth_secret[0], &ts))
         {
            output_status(LV_ALWAYS,"** Authenticate secret created\n");

            // create the money secret
            if (CreateMoneySecret(&money_secret[0], &ts))
            {
               output_status(LV_ALWAYS,"** Money secret created\n");
               // coprocessor is already current device
               if (WriteFileSHA(flname, buf, 8, page_map, &ts))
               {
                  output_status(LV_ALWAYS,"** Co-processor info file created\n");
                  exit_code = EXIT_SUCCESS;
               }
               else
               {
                  output_status(LV_ALWAYS,"ERROR, Could not write Co-processor info file\n");
                  exit_code = EXIT_ERROR_WRITE;
               }
            }
            else
            {
               output_status(LV_ALWAYS,"ERROR, Could not create money secret\n");
               exit_code = EXIT_ERROR_MONEY_SCRT;
            }
         }
         else
         {
            output_status(LV_ALWAYS,"ERROR, Could not create authorization secret\n");
            exit_code = EXIT_ERROR_AUTH_SCRT;
         }
      }
      else
      {
         output_status(LV_ALWAYS,"ERROR, Could not find a SHA iButton\n");
         exit_code = EXIT_ERROR_NO_SHA;
      }
   }
   else
   {
      output_status(LV_ALWAYS,"ERROR, Could not select co-processor 1-Wire\n");
      exit_code = EXIT_ERROR;
   }

   // release the 1-Wire Net
   owRelease(ts.portnum[IB_ROVING]);
   sprintf(msg,"Closing port %s.\n", argv[1]);
   output_status(LV_ALWAYS,msg);

   // close opened file
   if ((fp != NULL) && (fp != stdout))
   {
      printf("File '%s' closed.\n",
              argv[filenum]);
      fclose(fp);
   }

   // check on result of operation
   if (exit_code)
   {
      OWERROR_DUMP(stdout);
      printf("ERROR reported (%d), end program\n",exit_code);
   }
   else
      printf("End program normally\n");

   // end the program
   exit(exit_code);

   return exit_code;
}


