
#proc settings
#if @DEVICE in svg,svgz
 xml_encoding: utf-8
#set ADJ1 = -0.33
#set LW = 3.0
#set RAREA = '1 1 8 3.6'
#set TSIZE = 12
#else
#set ADJ1 = -0.1
#set LW = 1.0
#set RAREA = '1 1 4 2.2'
#set TSIZE = 10
#endif

// Greenhouse

#proc getdata
 file: w1.dat
 fieldnames times gtemp itemp1 itemp2 

#proc page
 outfilename _gtemp.@DEVICE
 font: Sans
 textsize: @TSIZE
 backgroundcolor: rgb(1,1,0.92156863)

#proc areadef
  title: Greenhouse Temperature
  titledetails: align=C
  yautorange: datafield=gtemp,itemp1,itemp2
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: inc
  grid: color=gray(0.8)
  label: °C
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: gtemp
 xfield: times
 linedetails: width=@LW color=green
 legendlabel: Greenhouse

#proc lineplot
 yfield: itemp1
 xfield: times
 linedetails: width=@LW color=yelloworange
 legendlabel: Propagator1

#proc lineplot
 yfield: itemp2
 xfield: times
 linedetails: width=@LW color=lightorange
 legendlabel: Propagator2

#proc legend
 location: min+1 max
 seglen: 0.2

// Outside

#proc getdata
 file: w2.dat
 fieldnames times otemp stemp ctemp1

#proc page
 font: Sans
 outfilename _temp.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Outside Temperature
  titledetails: align=C
  yautorange: datafield=otemp,stemp,ctemp1
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white
  
#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: °C
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: ctemp1
 xfield: times
 linedetails: width=@LW color=orange
 legendlabel: Front Garden

#proc lineplot
 yfield: otemp
 xfield: times
 linedetails: width=@LW color=magenta
 stairstep: no
 legendlabel: Air

#proc lineplot
 yfield: stemp
 xfield: times
 linedetails: width=@LW color=tan2
 legendlabel: Soil

#proc legend
 location: min+1 max
 seglen: 0.2
 
//Pressure 

#proc getdata
 file: w3.dat
 fieldnames times press

#proc page
 font: Sans
 outfilename _press.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Pressure
  titledetails: align=C
  yautorange: datafield=press
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: hPa
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: press
 xfield: times
 linedetails: width=@LW color=powderblue

// Humidity 

#proc getdata
 file: w4.dat
 fieldnames times humid

#proc page
 font: Sans
 outfilename _humid.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Relative Humidity
  titledetails: align=C
  yautorange: datafield=humid hifix=@hmax lowfix=0
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: %
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: humid
 xfield: times
 linedetails: width=@LW color=lightpurple

// Solar 

#proc getdata
 file: w6.dat
 fieldnames times solar

#proc page
 font: Sans
 outfilename _solar.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Solar Radiation
  titledetails: align=C
  yautorange: datafield=solar hifix=@solmax lowfix=0
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: W/m^2
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: solar
 xfield: times
 linedetails: width=@LW color=redorange

// All temps

#proc getdata
 file: w5.dat
 fieldnames times gtemp otemp htemp btemp

#proc page
 font: Sans
 outfilename _temps.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Comparative Temperatures
  titledetails: align=C
  yautorange: datafield=gtemp,otemp,htemp,btemp
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times hifix=@edate nearest=hours
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incr
  grid: color=gray(0.8)
  label: °C
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: btemp
 xfield: times
 linedetails: width=@LW color=oceanblue
 legendlabel: Garage
 
#proc lineplot
 yfield: htemp
 xfield: times
 linedetails: width=@LW color=yelloworange
 legendlabel: Sitting Room

#proc lineplot
 yfield: gtemp
 xfield: times
 linedetails: width=@LW color=drabgreen
 legendlabel: Greenhouse

#proc lineplot
 yfield: otemp
 xfield: times
 linedetails: width=@LW color=claret
 legendlabel: Outside
  
#proc legend
 location: min+1 max
 seglen: 0.2

#if @dodock = 1
#proc getdata
 file: w0.dat
 fieldnames times dirn ws gs tide

#proc page
 font: Sans
 outfilename _wspeed.@DEVICE
 backgroundcolor: rgb(0.92156863,1,1)
 textsize: @TSIZE

#proc areadef
  title: Wind Speed
  titledetails: align=C
  yautorange: datafield=ws,gs
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times nearest=hour hifix=@edate
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  stubformat: hh:mm
  stubvert: yes
  minorticinc: 0.25 hours

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: Knots
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: ws
 xfield: times
 linedetails: width=@LW color=blue
 legendlabel: Average

#proc lineplot
 yfield: gs
 xfield: times
 linedetails: width=@LW color=red
 legendlabel: Gust

#proc legend
 location: min+1 max
 seglen: 0.2

#proc page
 font: Sans
 outfilename _tide.@DEVICE
 backgroundcolor: rgb(0.92156863,1,1)
 textsize: @TSIZE

#proc areadef
  title: Tide Level
  titledetails: align=C
  yautorange: datafield=tide
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times nearest=hour hifix=@edate
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  minorticinc: 0.25 hours
  stubformat: hh:mm
  stubvert: yes

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: metres
  labeldetails: adjust=@ADJ1,0
  
#proc lineplot
 yfield: tide
 xfield: times
 linedetails: color=gray(0.6) width=@LW

#proc page
 font: Sans
 outfilename _wdirn.@DEVICE
 backgroundcolor: rgb(0.92156863,1,1)
 textsize: @TSIZE

#proc areadef
  title: Wind Direction
  titledetails: align=C
  yrange: 0 359
  xscaletype: datetime yyyy-mm-dd.hh:mm
  xautorange: datafield=times nearest=hour hifix=@edate
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  minorticinc: 0.25 hours
  stubformat: hh:mm
  stubvert: yes

#proc yaxis
  stubs: inc 45
  grid: color=gray(0.8)
  label: Degs
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: dirn
 xfield: times
 linedetails: width=@LW color=lavender
#endif

#proc getdata
 file: w7.dat

#proc page
 font: Sans
 outfilename _rain.@DEVICE
 backgroundcolor: rgb(1,1,0.92156863)
 textsize: @TSIZE

#proc areadef
  title: Rainfall
  titledetails: align=C
  yautorange: datafield=2 hifix=@rmax lowfix=0
  xscaletype: datetime yyyy-mm-dd.hh:mm 
  xautorange: datafield=1 nearest=hour hifix=@edate
  rectangle: @RAREA
  areacolor: white

#proc xaxis
  stubs: inc 1 hour
  minorticinc: 0.25 hours
  stubformat: hh:mm
  stubvert: yes

#proc yaxis
  stubs: incremental 
  grid: color=gray(0.8)
  label: Rate
  labeldetails: adjust=@ADJ1,0

#proc lineplot
 yfield: 2
 xfield: 1
 linedetails: width=@LW color=skyblue
 fill: skyblue
 
