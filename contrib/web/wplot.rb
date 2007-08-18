#!/usr/bin/ruby

# Copyright (c) 2006 Jonathan Hudson <jh+w1retap@daria.co.uk>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

######################################################################
# Warning! Danger! 
# I wrote this code before I really understood how to write good 
# ruby. It is ugly. Use at your own risk.
######################################################################


$: << ENV['HOME'] + '/lib/ruby'

require 'optparse'
require 'socket'
require 'dbi'
require 'html/template'

require 'raincalc.rb'
require 'svgwind.rb'
require 'svgthermo.rb'
require 'wug.rb'
require 'cwop.rb'

BASEDIR = '/var/www/roo/wx'
G = -9.80665
R = 287.04
HFACT = 1.0

GTEMP_V =  (1 << 0)
TEMP_V =   (1 << 1)
HTEMP_V =  (1 << 2)
BTEMP_V =  (1 << 3)
RPRESS_V = (1 << 4)
HUMID_V =  (1 << 5)
RAIN_V =   (1 << 6)
ITEMP1_V = (1 << 7)
ITEMP2_V = (1 << 8)
STEMP1_V = (1 << 9)

#def pr_msl p0, temp, z
#  zdiff = (0 - z)
#  kt = temp +  273.15
#  x = (G * zdiff / (R * kt))
#  p0 * Math.exp(x)
#end

def dewpoint ct, humid
  b = (ct > 0 ) ? 237.3 : 265.5
  es = 610.78 * Math.exp(17.2694 * ct / (ct + b))
  e = humid / 100 * es
  f = Math.log( e / 610.78 ) / 17.2694
  b *  f / ( 1 - f)
end

def CtoF c
  ('%.1f' % ((9.0/5.0)*c + 32.0))
end
  
def mbtoin mb
  ("%.3f" % (mb/33.8638864))
end

now = Time.now

opt_S = 'dbi:Pg:sensors'
opt_W = 'dbi:Pg:docks'
opt_i = 1200
opt_u = true
opt_U = opt_s = opt_f = nil
opts = OptionParser.new

opts.on("-t",'Test mode') {|val| opt_t = val }
opts.on("-i")                        {|val| opt_i = val.to_i }
opts.on("-f")                        {|val| opt_f = val }
opts.on("-s")                        {|val| opt_s = val }
opts.on("-g")                        {|val| opt_g = val }
opts.on("-l")                        {|val| opt_l = val }
opts.on("-u") {|o| opt_u=false }
opts.on('-U', '--user NAME','User name for database') {|val| opt_U = val }
opts.on("-S", "--sensor_db DBNAME", String) {|val| opt_S = val }
opts.on("-W", "--docks_db  DBNAME", String) {|val| opt_W = val }
rest = opts.parse(ARGV)

base = rest[0] || BASEDIR

if opt_s
  imime='"image/svg+xml"'
  itype='svg'
  opt_p = 1
else
  imime='"image/png"'
  itype='png'
end

if test(?d,base) 
  Dir.chdir(base) 
else
  opt_t = true
end

print "Running on #{Socket.gethostname}\n with\t#{opt_S}\n\t#{opt_W}\n" if opt_t

ithen = now - 24*3600
sleep 10 if (now.min % 10) == 0 and !opt_t

f=File.open('w0.dat','w')
f1=File.open('w_3.dat','w')
luser = opt_U || ENV['USER']
dbh = DBI.connect(opt_W, luser, '')
dsource=nil
smt = dbh.execute "select reportdate, wind_dirn, wind_speed, wind_speed1, tide, pressure, source from observations where reportdate > ? order by reportdate",ithen.to_i

lastdock=nil
dock = []
smt.each do |dock|
  lastdock = Time.at(dock[0].to_i).strftime("%Y-%m-%d.%H:%M")
  dsource = dock[6]
  f.printf("%s\t%.1f\t%.1f\t%.1f\t%s\n", lastdock, 
	   dock[1].to_f, 0.8 * dock[2].to_f, 0.8 * dock[3].to_f, dock[4])
  f1.printf("%s %.1f\n", lastdock,dock[5])
end
smt.finish
f.close
f1.close

dodock = 1
if lastdock.nil?
  ld = dbh.select_one('select max(reportdate) from observations')
  lastdock = Time.at(ld[0].to_i).strftime("%Y-%m-%d %H:%M")
  dodock = 0
else
  lastdock.sub!(/\./, ' ')
end

dbh.disconnect

dock[2] = 0.8 * dock[2].to_f
dock[3] = 0.8 * dock[3].to_f

fh = []
fh[0] = File.open('w1.dat', 'w')
fh[1] = File.open('w2.dat', 'w')
fh[2] = File.open('w3.dat', 'w')
fh[3] = File.open('w4.dat', 'w')
fh[4] = File.open('w5.dat', 'w')

dbh = DBI.connect(opt_S, 'jrh', '')

stn = {}
s = dbh.execute('select * from station')
s.fetch_hash do |r|
  stn = r
end  
s.finish

maxt = tmax = -999
mint = tmin = 999
hmax = 0
ltim = ithen
d1 = -1
d2 = -2
gtemp = temp = press = humid = htemp = btemp = nil
sdate=edate=nil
ld = r0 = r1 = 0
lt0 = lt1 = nil
ast = 0
press = nil
itemp1=nil
itemp2=nil
stemp1=nil
d = ''
h = {}
val = 0
va = []


smt = dbh.execute "select date, name, value from readings where date >= ? order by date", ithen
smt.each do |s|
  begin
    ast = s[0].to_time
  rescue
    require 'parsedate'
    r=ParseDate.parsedate(s[0])
    ast = Time.local(*r)
  end
  if ltim != ast 
    d = ltim.strftime("%Y-%m-%d.%H:%M")

    if (val & (GTEMP_V|ITEMP1_V|ITEMP2_V)) != 0
      if (val & GTEMP_V) == GTEMP_V
	va[0] = "%.1f" % gtemp
      else
	va[0] = 'x'
      end
      if (val & ITEMP1_V) == ITEMP1_V
	itemp1 = "%.1f" % itemp1
      else
	itemp1 = 'x'
      end
      if (val & ITEMP2_V) == ITEMP2_V
	itemp2 = "%.1f" % itemp2
      else
	itemp2 = 'x'
      end
      fh[0].puts [d,va[0],itemp1,itemp2].join("\t")
    end

    if (val & (TEMP_V|STEMP1_V)) != 0
      if (val & TEMP_V) == TEMP_V
	va[1] = "%.1f" % temp
      else
	va[1] = 'x'      
      end
      if (val & STEMP1_V) == STEMP1_V
	stemp1 = "%.1f" % stemp1
      else
	stemp1 = 'x'
      end
      fh[1].puts [d,va[1],stemp1].join("\t")
    end

    if (val & RPRESS_V) == RPRESS_V
      fh[2].puts [d,"%.1f" % press].join("\t")
    else
      press = ''
    end 

    if (val & HUMID_V) == HUMID_V
      fh[3].puts [d,"%.1f" % humid].join("\t")
    end

    if (val & (TEMP_V|GTEMP_V|HTEMP_V|BTEMP_V)) != 0
      if ((val & HTEMP_V) == HTEMP_V)
	va[2] = "%.1f" % htemp 
      else
	va[2] = 'x'
	htemp = ''
      end
      if ((val & BTEMP_V) == BTEMP_V)
	va[3] = "%.1f" % btemp 
      else
	va[3] = 'x'
	btemp = nil
      end
      gstr = d+"\t"+ va.join("\t")
      fh[4].puts gstr
    end
    sdate=d if sdate.nil?
    edate=d
    ltim = ast
    val = 0
  end

  case s[1]
  when 'GHT'
    gtemp = s[2].to_f
    maxt = gtemp if gtemp > maxt
    mint = gtemp if gtemp < mint
    val |= GTEMP_V

  when 'ITMP1'
    itemp1 = s[2].to_f
    val |= ITEMP1_V


  when 'ITMP2'
    itemp2 = s[2].to_f
    val |= ITEMP2_V


  when 'STMP1'
    stemp1 = s[2].to_f
    val |= STEMP1_V

  when 'OTMP0'
    temp = s[2].to_f
    tmax = temp if temp > tmax
    tmin = temp if temp < tmin
    val |= TEMP_V

  when 'OTMP1'
    htemp = s[2].to_f
    val |= HTEMP_V

  when 'OTMP2'
    btemp = s[2].to_f
    val |= BTEMP_V

  when 'OPRS'
    press = s[2].to_f
    val |= RPRESS_V

  when 'OHUM'
    humid = HFACT * s[2].to_f
    hmax = humid if humid > hmax
    val |= HUMID_V

  when 'RGC0'
    if (r0 == 0)
      r0 = s[2].to_i
      lt0 = ast
    end
    r1 = s[2].to_i
    lt1 = ast
    h[ast]=s[2].to_i
    val |= RAIN_V
  end
end

d = ast.strftime("%Y-%m-%d.%H:%M")

if (val & (GTEMP_V|ITEMP1_V|ITEMP2_V)) != 0
  if (val & GTEMP_V) == GTEMP_V
    va[0] = "%.1f" % gtemp
  else
    va[0] = 'x'
  end
  if (val & ITEMP1_V) == ITEMP1_V
    itemp1 = "%.1f" % itemp1
  else
    itemp1 = 'x'
  end
  if (val & ITEMP2_V) == ITEMP2_V
    itemp2 = "%.1f" % itemp2
  else
    itemp2 = 'x'
  end
  fh[0].puts [d,va[0],itemp1,itemp2].join("\t")
end

if (val & (TEMP_V|STEMP1_V)) != 0
  if (val & TEMP_V) == TEMP_V
    va[1] = "%.1f" % temp
  else
    va[1] = 'x'      
  end
  if (val & STEMP1_V) == STEMP1_V
    stemp1 = "%.1f" % stemp1
  else
    stemp1 = 'x'
  end
  fh[1].puts [d,va[1],stemp1].join("\t")
end

if (val & RPRESS_V) == RPRESS_V
  fh[2].puts [d,"%.1f" % press].join("\t")
else
  press = ''
end 

if (val & HUMID_V) == HUMID_V
  fh[3].puts [d,"%.1f" % humid].join("\t")
end

if (val & (TEMP_V|GTEMP_V|HTEMP_V|BTEMP_V)) != 0
  if ((val & HTEMP_V) == HTEMP_V)
    va[2] = "%.1f" % htemp 
  else
    va[2] = 'x'
    htemp = ''
  end
  if ((val & BTEMP_V) == BTEMP_V)
    va[3] = "%.1f" % btemp 
  else
    va[3] = 'x'
    btemp = nil
  end
  gstr = d+"\t"+ va.join("\t")
  fh[4].puts gstr
end
sdate=d if sdate.nil?
edate=d
ltim = ast

smt.finish
fh.each {|f| f.close}
d2 = lt1

if press.to_i < 900 || press.to_i > 1200
  press = dock[5];
  File.rename("w_3.dat", "w3.dat")
end

if humid
  dewpt= dewpoint temp, humid
else
  dewpt = nil
  File.open('w4.dat', 'w') do |f|
    f.puts "#{sdate} 0"
    f.puts "#{edate} 0"
  end
end
logdate = d.sub(/\./, ' ')

rain = 0
rain1 = 0
rain24 = 0

if ((r1 - r0) and (lt1 - lt0))
  rain24 = 24*3600 * stn['rfact']*(r1 - r0)/(lt1 - lt0)
end
print "Rain 24hr #{rain24}\n" if opt_t

rmax = 0.0
lh1 = lt1 - 3600
r1h = 0
rast = 0
raet = 0
lrr = 0
radr = 0

fh = File.open 'w6.dat', 'w'
h.keys.sort.each do |k|
  j = (k - opt_i)
  rain = 0
  if h.has_key?(j)
    rain = (h[k] - h[j]) * stn['rfact']
    rmax = rain if rain > rmax
    if rain != 0 
      radr = k
      if lrr == 0
	rast = k
	raet = 0
      end
    else
      raet = k if raet == 0  and rast != 0
    end
  end
  lrr = rain
  if k >= lh1 and r1h == 0
    r1h = h[k]
    lh1 = k
  end
  d = k.strftime("%Y-%m-%d.%H:%M")
  fh.printf "%s %.4f\n", d,rain
end
fh.close

rain1 = 3600 * stn['rfact']*(r1 - r1h)/(lt1 - lh1)
print "Rain 1hr #{rain1}\n" if opt_t
h.clear

dock[0] = Time.at(dock[0].to_i).strftime("%Y-%m-%d %H:%M") 
use_t = true # Workaround for b0rken SQLite3  DBI
if (now.min % 10 == 0 or opt_f) and !opt_t
  puts "Latest"
  begin
    dbh['AutoCommit'] = false
  rescue
    use_t = false
  end
  begin
    dbh.do "delete from latest where udate < ?",ithen.to_i
    dbh.do "insert into latest values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
      d2.to_i, logdate, gtemp, maxt, mint, temp, press, humid, rain1,
      dewpt, dock[0], dock[2], dock[1], dock[4]
    puts "Latest #{d2}"
    if now.hour == 0 and now.min  == 0
      dbh.do "insert into daily values (?,?,?,?)", d2.to_i, r1,0,logdate
      if now.day == 1
	dbh.do "insert into monthly values (?,?,?,?)", d2.to_i,r1,0,logdate
      end
    end
    dbh.commit if use_t
  rescue
    dbh.rollback if use_t
  end
end

rmax += 0.01
hmax *= 1.05
hmax = 100 if hmax < 1
hmax = 100 if hmax > 100

cells = RainCalc.raincalc dbh, d2, r1, stn['rfact']
dbh.disconnect
gval = gtemp || -100
SvgThermo.makeimage gval, 150, 40 
SvgWind.makeimage((dodock == 0 ? -1 : dock[1]), 120, 120)

ENV['GDFONTPATH'] = '/usr/share/fonts/truetype:/usr/share/fonts/truetype/freefont/';

#["wdirn.png", "wspeed.png", "tide.png" ].each do |f|
#  begin
#    File.delete(f)
#  rescue
#  end
#end

print "Start Graphs, edate=#{edate} rmax=#{rmax}\n" if opt_t
t_0 = Time.now;

plotopt = (opt_s.nil?) ? '-font FreeSans -png -crop 0.2,0.5,4.2,2.5' : '-svg'
cmd = "ploticus #{plotopt} edate=#{edate} rmax=#{rmax} hmax=#{hmax} currain=#{rain} dodock=#{dodock} wxcrop.plt"
system cmd
print "#{cmd}\n" if opt_t

if (opt_p and opt_s.nil?)
  puts "Start Convert" if opt_t
  %w{gtemp humid press temp temps tide wdirn wspeed}.each do |x|
    svg = "#{x}.svg"
    if File.exists?(svg)
      system "rsvg -w 400 -h 200 #{svg} #{x}.png"
    end
  end
end

%w(thermo winddirn gtemp temp press humid temps wspeed tide wdirn rain).each do |gf|
  File.rename "_#{gf}.png","#{gf}.png"
end

print "Done Graphs #{(Time.now- t_0)}s\n" if opt_t

if dodock == 0
  File.symlink("nodock.png", "wspeed.png")
end

tmpl = HTML::Template.new('wx.htt.html')
rainlist = []
nc = 0
cells.each do |cl|
  c = cl.split(':')
  rainlist.push({ 'pcol' => ((nc & 1) == 0) ? 'grey1' : 'grey2',
		  'pnam' => c[0],
                  'vali' => c[1],
                  'valm' => c[2],
		  'vala' => c[3]
                })
  nc += 1
end

raintimes=''
raintimes = "start: #{rast.strftime("%H:%M")} " if rast != 0
raintimes += "last: #{radr.strftime("%H:%M")} " if radr != 0
raintimes += "end: #{raet.strftime("%H:%M")}" if raet != 0
puts "Raintimes #{raintimes}" if opt_t

wx_notice=''
begin
  fh = File.open 'wx_notice.html', 'r'
  wx_notice = fh.read
  fh.close
rescue
end

itemp1 =nil if itemp1 == 'x'
itemp2 =nil if itemp2 == 'x'
gtemp =nil if gtemp == 'x'
stemp =nil if stemp == 'x'

hustr = (humid) ? "%.1f" % humid : 'N/A'
gtempC = gtempS = gtempF = itemp1C = itemp2C = itemp1S = itemp2S = 'N/A'

if gtemp
  gtempS = "%.1f" % gtemp
  gtempC = "#{gtempS}&deg;C"
  gtempF = "%.1f&deg;F" % CtoF(gtemp)
end
if itemp1 
  begin
    itemp1S = "%.1f" % itemp1
    itemp1C = "#{itemp1S}&deg;C"
  rescue
  end
end
if itemp2 
  begin
    itemp2S = "%.1f" % itemp2
    itemp2C = "#{itemp2S}&deg;C"
  rescue
  end
end

if stemp1
  begin
    stemp1S = "%.1f" % stemp1
  rescue
  end
end

rain1mm = rain1*25.4
rain24mm = rain24*25.4
lastdock << '*' if dsource == 'AVTS'
tmpl.param({
             'gtype' => itype,
             'temp' => "%.1f&deg;C / %.1f&deg;F" % [temp, CtoF(temp)],
             'date' => logdate,
             'gtemp' => "#{gtempC} / #{gtempF}",
             'gmin' => (mint) ? "%.1f&deg;C / %.1f&deg;F" % [mint, CtoF(mint)] : 'N/A',
             'gmax' => (maxt) ? "%.1f&deg;C / %.1f&deg;F" % [maxt, CtoF(maxt)] : 'N/A',
	     'itemp1' => itemp1C,
	     'itemp2' => itemp2C,
	     'stemp' => "#{stemp1S}&deg;C",
	     'tmax' => "%.1f&deg;C" % [tmax], 
	     'tmin' => "%.1f&deg;C" % [tmin], 
             'press' => "%.1f hPa / %.1f inHg" % [press, mbtoin(press)],
             'humid' => hustr,
             'dewpt' => (humid) ? "%.1f&deg;C / %.1f&deg;F" % [dewpt, CtoF(dewpt)] : 'N/A',
             'rain' => "%.1fmm / %.2fin" % [rain1mm, rain1],
             'rain24' => "%.1fmm / %.2fin" % [rain24mm, rain24],
             'pdate' => lastdock,
             'wdirn' => dock[1],
             'wspeed' => dock[2],
             'wgust' => dock[3],
             'tide' => dock[4],
             'rainlist' => rainlist,
	     'raintimes' => raintimes,
	     'wx_notice' => wx_notice
           })
File.open('w.html', 'w') do |f|
  f.print tmpl.output
end
File.rename('w.html','index.html')

rstr = ''
rstr += "@#{radr.strftime("%H:%M")}" if radr != 0 and rain1 > 0
dwstr = (humid) ? "%.1f" % dewpt : 'N/A'

tmpl.load 'wx.static.tmpl'
tmpl.param({
	     'udate' => d2.to_i,
	     'idate' => d2.strftime("%FT%T%z"),
	     'temp' => "%.1f" % temp,
	     'press' => "%.1f" % press,
	     'humid' => hustr,
	     'rain' => "%.1fmm" % [rain1mm],
	     'rstr' => rstr,
	     'dewpt' =>  dwstr,
	     'gtemp' => gtempS,
	     'itemp1' =>  itemp1S,
	     'itemp2' =>  itemp2S,
	     'wspd' => dock[2],
	     'wdir' => dock[1],
	     'tide' => dock[4],
	   })

File.open('wx_static.xml', 'w') do |f|
  f.print tmpl.output
end

tmpl.load('wx.text.tmpl')
tmpl.param({
	     'udate' => d2.to_i,
	     'idate' => d2.strftime("%FT%T%z"),
	     'temp' => "%.1f" % temp,
	     'press' => "%.1f" % press,
	     'humid' => hustr,
	     'rain' => "%.1fmm" % rain1mm,
	     'rstr' => rstr,
	     'r24' => "[%.1fmm]" % rain24mm,
	     'dewpt' => dwstr,
	     'gtemp' => gtempS,
	     'itemp1' => itemp1S,
	     'itemp2' => itemp2S,
	     'stemp' => stemp1S,
	     'htemp' => "%.1f" % htemp,
	     'btemp' => (btemp) ? "%.1f" % btemp : 'N/A',
	     'wspd' => dock[2],
	     'wdir' => dock[1],
	     'tide' => dock[4]
	   })
File.open('/tmp/.wx_static.dat', 'w') do |f|
  f.print tmpl.output
end

if opt_u and (now.min % 10 == 0)
  w = { 'humid' => humid,
    'temp' => temp,
    'dewpt' => dewpt,
    'pres' => press,
    'rain' => rain1,
    'rain24' => rain24,		
    'udate' => now.to_i } 
  Wug.upload(stn, opt_t, w) if stn['wu_user']
  CWOP.upload(stn, opt_t, w) if stn['cwop_user']
end 

