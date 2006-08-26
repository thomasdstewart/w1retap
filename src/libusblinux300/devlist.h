typedef struct
{
    char *dev;
    char *name;
    uchar family;
} w1devs_t;

w1devs_t devlist[] = {
    {"2401","silicon serial number",1},
    {"2411","silicon serial number Vcc input",1},
    {"1425","multikey 1153bit secure",2},
    {"2404","econoram time chip",4},
    {"2405","Addresable Switch",5},
    {NULL,"4k memory ibutton",6},
    {NULL,"1k memory ibutton",8},
    {"2502","1k add-only memory",9},
    {NULL,"16k memory ibutton",0xA},
    {"2505","16k add-only memory",0x0B},
    {NULL,"64k memory ibutton",0xc},
    {"2506","64k add-only  memory",0x0F},
    {"18S20","high precision digital thermometer",0x10},
    {"2406","dual addressable switch plus 1k memory",0x12},
    {"2407","dual addressable switch plus 1k memory",0x12},
    {"2430A","256 eeprom",0x14},
    {NULL,"4k Monetary",0x1A},
    {"2436","battery id/monitor chip",0x1B},
    {"28E04-100","4k EEPROM with PIO",0x1C},
    {"2423","4k ram with counter",0x1D},
    {"2409","microlan coupler",0x1F},
    {"2450","quad a/d converter",0x20},
    {NULL,"Thermachron",0x21},
    {"1822","Econo Digital Thermometer",0x22},
    {"2433","4k eeprom",0x23},
    {"2415","time chip",0x24},
    {"2438","smart battery monitor",0x26},
    {"2417","time chip with interrupt",0x27},
    {"18B20","programmable resolution digital thermometer",0x28},
    {"2408","8-channel addressable switch",0x29},
    {"2890","digital potentiometer",0x2C},
    {"2431","1k eeprom",0x2D},
    {"2770","battery monitor and charge controller",0x2E},
    {"2760","high-precision li+ battery monitor",0x30},
    {"2761","high precision li+ battery monitor",0x30},
    {"2762","high precision li+ battery monitor with alerts",0x30},
    {"2720","efficient addressable single-cell rechargable lithium protection ic",0x31},
    {"2432","1k protected eeprom with SHA-1",0x33},
    {"2740","high precision coulomb counter",0x36},
    {NULL,"Password protected 32k eeprom",0x37},
    {"2413","dual channel addressable switch",0x3A},
    {"2422","Temperature Logger 8k mem",0x41},
    {"2751","multichemistry battery fuel gauge",0x51},
    {NULL,"Serial ID Button",0x81},
    {"2404S","dual port plus time",0x84},
    {"2502-E48 2502-UNW","48 bit node address chip",0x89},
    {"2505-UNW","16k add-only uniqueware",0x8B},
    {"2506-UNW","64k add-only uniqueware",0x8F},
    {"LCD","LCD (Swart)",0xFF},
    {NULL,NULL,0}
};
