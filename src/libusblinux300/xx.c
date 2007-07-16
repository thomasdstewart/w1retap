

static void w1_set_coupler(int portnum, uchar *ident, int line)
{
  uchar a[4];
  SetSwitch1F(portnum, serno, line, 2, a, TRUE);
}

static int w1_select_device(int portnum, uchar *serno, uchar *coupler, int line)
{
    u_char thisdev[8];
    int found = 0, i;

    
    if(coupler)
    {
        w1_set_coupler(portnum, coupler, 0);
        w1_set_coupler(portnum, coupler, line);
    }

    owFamilySearchSetup(portnum,serno[0]);
    while (owNext(w1->portnum,TRUE, FALSE)){
      owSerialNum(w1->portnum, thisdev, TRUE);
      if(memcmp(thisdev, serno, sizeof(thisdev)) == 0){
	found = 1;
	break;
      }
    }
    return found;
}
