#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include "ownet.h"
#include <gmodule.h>

SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
static void *handle;

static void loadiolib(char *port)
{
    char *name;
    
    if (strncmp(port, "DS2490", sizeof("DS2490")-1) == 0)
    {
        name = "libw1usb.so";
        FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = TRUE;
    }
    else
    {
        name = "libw1serial.so";
        FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE;
    }

    handle =  g_module_open(name, G_MODULE_BIND_LOCAL);
    if(handle == NULL)
    {
        const gchar *gerr;
        gerr = g_module_error();
        fprintf(stderr, "Can't open the device library %s\n"
                "System returned:\n%s\n"
		"This is typically a build dependency or installation error\n",
		name, gerr);
        exit(1);
    }
    g_module_symbol (handle, "msDelay_", (void *)&msDelay);
    g_module_symbol (handle, "msGettick_", (void *)&msGettick);
    g_module_symbol (handle, "owHasOverDrive_",(void *)&owHasOverDrive);
    g_module_symbol (handle, "owHasPowerDelivery_",(void *)&owHasPowerDelivery);
    g_module_symbol (handle, "owHasProgramPulse_",(void *)&owHasProgramPulse);
    g_module_symbol (handle, "owLevel_",(void *)&owLevel);
    g_module_symbol (handle, "owProgramPulse_",(void *)&owProgramPulse);
    g_module_symbol (handle, "owReadBitPower_",(void *)&owReadBitPower);
    g_module_symbol (handle, "owReadByte_",(void *)&owReadByte);
    g_module_symbol (handle, "owReadBytePower_",(void *)&owReadBytePower);
    g_module_symbol (handle, "owSpeed_",(void *)&owSpeed);
    g_module_symbol (handle, "owTouchBit_",(void *)&owTouchBit);
    g_module_symbol (handle, "owTouchByte_",(void *)&owTouchByte);
    g_module_symbol (handle, "owTouchReset_",(void *)&owTouchReset);
    g_module_symbol (handle, "owWriteByte_",(void *)&owWriteByte);
    g_module_symbol (handle, "owWriteBytePower_",(void *)&owWriteBytePower);
    g_module_symbol (handle, "owAccess_",(void *)&owAccess);
    g_module_symbol (handle, "owFamilySearchSetup_",(void *)&owFamilySearchSetup);
    g_module_symbol (handle, "owFirst_",(void *)&owFirst);
    g_module_symbol (handle, "owNext_",(void *)&owNext);
    g_module_symbol (handle, "owOverdriveAccess_",(void *)&owOverdriveAccess);
    g_module_symbol (handle, "owSerialNum_",(void *)&owSerialNum);
    g_module_symbol (handle, "owSkipFamily_",(void *)&owSkipFamily);
    g_module_symbol (handle, "owVerify_",(void *)&owVerify);
    g_module_symbol (handle, "owAcquire_",(void *)&owAcquire__);
    g_module_symbol (handle, "owAcquireEx_",(void *)&owAcquireEx__);
    g_module_symbol (handle, "owRelease_",(void *)&owRelease);
    g_module_symbol (handle, "owBlock_",(void *)&owBlock);
    g_module_symbol (handle, "owProgramByte_",(void *)&owProgramByte);
}

int owAcquireEx(char *port_zstr)
{
    if(handle == NULL)
    {
        loadiolib(port_zstr);
    }
    return owAcquireEx__(port_zstr);
}

SMALLINT owAcquire(int portnum, char *port_zstr)
{
    if(handle == NULL)
    {
        loadiolib(port_zstr);
    }
    return owAcquire__(portnum, port_zstr);
}
