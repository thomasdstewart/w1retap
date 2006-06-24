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
//  initrov.c - This utility initializes roving SHA iButtons
//              (DS1963S).
//
//  Version:   2.00
//  History:
//

#include "ownet.h"
#include "ibsha18.h"

// external function prototypes
extern SMALLINT owAcquire(int,char *);
extern void owRelease(int);
extern int PrintHeader(char *);
extern void output_status(int,char *);
extern int EnterString(char *,char *,int,int);
extern int ToHex(char);
extern int GetSeviceProviderSettings(int,char *,uchar *,int,uchar *);
extern int GetRovingSettings(uchar *, uint *);
extern int WriteFileSHA(uchar *,uchar *,int,uchar *,TransState *);
extern void ExitProg(char *,int);
extern int CreateAuthSecret(uchar *, TransState *);
extern int FindSHA(uchar *, TransState *);
extern int SelectRoving(TransState *);
extern int CreateUniqueDeviceSecret(uchar *, TransState *);
extern int ReadAuth(eCertificate *, TransState *);
extern int WriteMoney(eCertificate *, TransState *);
extern int FindCoprocessor(TransState *);
extern int VerifyMoney(eCertificate *, TransState *);
extern int getkeystroke(void);
extern int key_abort(void);

// local function
int CreateRovingSHA(uchar *,uchar *,uint,char *,TransState *);

// verbose output mode
int VERBOSE=0;
// file for output
FILE *fp;

//----------------------------------------------------------------------
//  This is the Main routine for initrov
//
int main(int argc, char **argv)
{
   int filenum;
   char msg[300];
   uchar auth_secret[85],dmmy[85];
   int cnt,exit_code,i;
   uint money;
   uchar rov_information[128];
   TransState ts;

   // check arguments to see if request instruction with '?' or too many
   if ((argc > 4) || (argc < 2) || ((argc > 1) && (argv[1][0] == '?' || argv[1][1] == '?')))
       ExitProg("\nusage: initrov 1wire_net_name <output_filename> </Verbose>\n"
              "  - initialize a roving SHA iButton on specified\n"
              "    1-Wire Net port,\n"
              "    co-processor SHA iButton must be present\n"
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
   if (!PrintHeader("InitRov Version 2.00"))
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

   // run the script find the co-processor
   sprintf((char *)(&ts.copr_file[0]),"COPR");
   if (FindCoprocessor(&ts))
   {
      // loop to print ROM of device found
      cnt = sprintf(msg,"** Co-processor SHA iButton found: ");
      for (i = 7; i >= 0; i--)
         cnt += sprintf(msg + cnt,"%02X",ts.copr_rom[i]);
      cnt += sprintf(msg + cnt,"\n** Provider file: ");
      for (i = 0; i < 4; i++)
         cnt += sprintf(msg + cnt,"%c",ts.provider[i]);
      cnt += sprintf(msg + cnt,".%d\n",ts.provider[4]);
      cnt += sprintf(msg + cnt,"** Money master secret page: %d\n",ts.c_mmaster_scrt);
      cnt += sprintf(msg + cnt,"** Auth master secret page: %d\n",ts.c_amaster_scrt);
      cnt += sprintf(msg + cnt,"** Unique device (calculated) secret page: %d\n",ts.c_udevice_scrt);
      output_status(LV_ALWAYS,msg);
      exit_code = EXIT_SUCCESS;
   }
   else
      ExitProg("ERROR, could not find co-processor on 1-Wire Net, end program\n",EXIT_ERROR);

   // prompt for service provider settings
   output_status(LV_ALWAYS,"\n\nEnter provider information to create the roving SHA iButton\n");
   if (!GetSeviceProviderSettings(FALSE, (char*)ts.provider, auth_secret, FALSE, dmmy))
      ExitProg("Aborted entering the provider settings, end program\n",EXIT_ERROR);

   // destination roving page (fixed pages for demo)
   ts.r_udevice_scrt = 5;  // roving unique device secret

   // default info and money
   sprintf((char*)rov_information,"no info");
   money = 1000;

   // loop to create roving SHA iButtons
   do
   {
      // create roving SHA
      exit_code = CreateRovingSHA(&auth_secret[0], &rov_information[0], money, &msg[0], &ts);
      output_status(LV_ALWAYS,msg);

      // print clear result
      if (exit_code == EXIT_SUCCESS)
         output_status(LV_ALWAYS,"\n-------------\n   SUCCESS\n-------------\n");
      else if (exit_code != EXIT_ERROR)
         output_status(LV_ALWAYS,"\n-------------\n    ERROR\n-------------\n");
   }
   while ((exit_code != EXIT_ERROR) && !key_abort());

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
      printf("ERROR reported (%d), end program\n",exit_code);
   else
      printf("End program normally\n");

   // end the program
   exit(exit_code);

   return exit_code;
}


//--------------------------------------------------------------------------
// Find and create a roving sha iButtons
//
int CreateRovingSHA(uchar *auth_secret, uchar *rov_information, uint money,
                    char *msg, TransState *ts)
{
   int rt=EXIT_SUCCESS,cnt=0,i;
   uint ul;
   uchar page_map[5];
   eCertificate ec;

   // place in loop for easy cleanup on error
   for (;;)
   {
      // prompt for the roving information
      if (!GetRovingSettings(rov_information, &money))
      {
         cnt += sprintf(msg + cnt,"Aborted entering the roving settings, end program\n");
         rt = EXIT_ERROR;
         break;
      }

      // give warning
      output_status(LV_ALWAYS,"\n** Warning (non-Copr) SHA iButton on 1-Wire will be overwritten!!\n"
                    "   (Press ENTER to continue)\n\n");
      getkeystroke();

      // select roving port
      if (SelectRoving(ts))
         cnt += sprintf(msg + cnt,"** 1-Wire port for roving selected\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, Could not select co-processor 1-Wire\n");
         rt = EXIT_ERROR;
         break;
      }

      // find the first available (non-Copr) SHA iButton
      ts->user_data[0] = 0;
      if (FindSHA(&ts->rov_rom[0], ts))
      {
         // loop to print ROM of device found
         cnt += sprintf(msg + cnt,"** SHA iButton found: ");
         for (i = 7; i >= 0; i--)
            cnt += sprintf(msg + cnt,"%02X",ts->rov_rom[i]);
         cnt += sprintf(msg + cnt,"\n");

         // set page map for money file
         page_map[0] = ts->r_udevice_scrt;
         page_map[1] = ts->r_udevice_scrt + 8;
      }
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, Could not find a SHA iButton\n");
         rt = EXIT_ERROR_NO_SHA;
         break;
      }

      // create the authentication secret
      if (CreateUniqueDeviceSecret(&auth_secret[0], ts))
         cnt += sprintf(msg + cnt,"** Uniqued device secret created\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, Could not create authorization secret\n");
         rt = EXIT_ERROR_AUTH_SCRT;
         break;
      }

      // write the money/info file (with invalid money to start)
      if (WriteFileSHA(ts->provider, rov_information, 56, page_map, ts))
         cnt += sprintf(msg + cnt,"** Roving info file created\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, Could not write roving info file\n");
         rt = EXIT_ERROR_WRITE;
         break;
      }

      // read auth money page (to get counters/rom in ec)
      if (ReadAuth(&ec,ts))
      {
         cnt += sprintf(msg + cnt,"** Money page read to get counter\n");
         // put correct money value in cert
         ul = money * 100;  // (1.02)
         for (i = 0; i < 3; i++)
         {
            ec.balance[i] = (uchar)(ul & 0xFF);
            ul >>= 8;
         }
         // increment counter
         ec.write_cycle_cnt++;
         // set transaction ID
         ec.trans_id = 0x1234;
         // code mult
         ec.code_mult = 0x8B48;  // US Dollars with 10^-2 multiplier (1.02)
      }
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, could not read auth money page\n"
                      "   Auth secret provide may be incorrect!!\n");
         rt = EXIT_ERROR_WRITE;
         break;
      }

      // write real money
      if (WriteMoney(&ec,ts))
         cnt += sprintf(msg + cnt,"** Money written\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, Could not write money\n");
         rt = EXIT_ERROR_WRITE;
         break;
      }

      // read auth money page (to get copr to do unique secret)
      if (ReadAuth(&ec,ts))
         cnt += sprintf(msg + cnt,"** Money page authenticate read\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, could not read auth money page\n"
                      "   Auth secret provide may be incorrect!!\n");
         rt = EXIT_ERROR_WRITE;
         break;
      }

      // verify money (sanity check)
      if (VerifyMoney(&ec, ts))
         cnt += sprintf(msg + cnt,"** Money contents verified\n");
      else
      {
         cnt += sprintf(msg + cnt,"ERROR, could not verify money\n");
         rt = EXIT_ERROR_WRITE;
         break;
      }

      break;
   }

   return rt;
}
