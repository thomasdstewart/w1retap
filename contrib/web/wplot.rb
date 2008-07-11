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

FIFO='/tmp/pertd2.fifo' 
DIRS=%w/N NE E SE S SW W NW/
BASEDIR = '/var/www/roo/wx'
G = -9.80665
R = 287.04
HFACT = 1.0


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
opt_t = nil
opts = OptionParser.new

opts.on("-t",'Test mode') {opt_t = true }
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
smt = dbh.execute "select reportdate, wind_dirn, wind_speed, wind_speed1, tide, pressure, source,mtime from observations where reportdate > ? order by reportdate",ithen.to_i

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
wsok = true
if(dock[0].to_i > dock[7].to_i + 900)
  wsok = false
  lastdock = nil if (ithen.to_i > dock[7].to_i)
end

if lastdock.nil?
  ld = dbh.select_one('select max(mtime) from observations')
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

lcdon=lcdoff=nil

lcdon = stn['lcdon'].to_i if stn.has_key?('lcdon')
lcdoff = stn['lcdoff'].to_i if stn.has_key?('lcdoff')

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
wxv = Array.new(720)

smt = dbh.execute "select EXTRACT(epoch from date), name, value from readings where date >= ? order by name,date", ithen
smt.each do |s|
  st = s[0].to_i
  id = (st - ithen.to_i)/120
  unless wxv[id]
    wxv[id] = {} 
    wxv[id][:date] = Time.at(st).strftime("%Y-%m-%d.%H:%M")
  end

  case s[1]
  when 'GHT'
    gtemp = s[2].to_f
    maxt = gtemp if gtemp > maxt
    mint = gtemp if gtemp < mint
    wxv[id][:ght] = gtemp

  when 'ITMP1'
    itemp1 = s[2].to_f
    wxv[id][:itmp1] = itemp1
    
  when 'ITMP2'
    itemp2 = s[2].to_f
    wxv[id][:itmp2] = itemp2
    
  when 'OTMP0'
    temp = s[2].to_f
    tmax = temp if temp > tmax
    tmin = temp if temp < tmin
    wxv[id][:temp] = temp

  when 'OTMP1'
    htemp = s[2].to_f
    wxv[id][:htemp] = htemp

  when 'OTMP2'
    btemp = s[2].to_f
    wxv[id][:btemp] = btemp
    
  when 'STMP1'
    stemp1 = s[2].to_f
    wxv[id][:stemp1] = stemp1
    
  when 'OPRS'
    press = s[2].to_f
    wxv[id][:press] = press

  when 'OHUM'
    humid = s[2].to_f
    hmax = humid if humid > hmax
    wxv[id][:humid] = humid

  when 'RGC0'
    if (r0 == 0)
      r0 = s[2].to_i
      lt0 =  st
    end
    r1 = s[2].to_i
    lt1 = st
    h[st]=s[2].to_i
  end
  sdate=st if sdate.nil?
  edate=st
end
smt.finish

sdate=Time.at(sdate).strftime("%Y-%m-%d.%H:%M")
edate=Time.at(edate).strftime("%Y-%m-%d.%H:%M")

wxv.each do |hs|
  next unless hs
  va = []
  va[0] = hs[:date]

  hs[:press] && fh[2].puts([hs[:date],"%.1f" % hs[:press]].join("\t")) 
  hs[:humid] && fh[3].puts([hs[:date],"%.1f" % hs[:humid]].join("\t")) 

  t2 = va[1] = (hs[:temp]) ? "%.1f" % hs[:temp] : 'x'
  va[2] = (hs[:stemp1]) ? "%.1f" % hs[:stemp1] : 'x'
  fh[1].puts va.join("\t")

  t1 = va[1] = (hs[:ght]) ? "%.1f" % hs[:ght] : 'x'
  va[2] = (hs[:itmp1]) ? "%.1f" % hs[:itmp1] : 'x'
  va[3] = (hs[:itmp2]) ? "%.1f" % hs[:itmp2] : 'x'
  fh[0].puts va.join("\t")

  va[1] = t1
  va[2] = t2
  va[3] = (hs[:htemp]) ? "%.1f" % hs[:htemp] : 'x'
  va[4] = (hs[:btemp]) ? "%.1f" % hs[:btemp] : 'x'
  fh[4].puts va.join("\t")
end

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
logdate = edate.sub(/\./, ' ')

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
  d = Time.at(k).strftime("%Y-%m-%d.%H:%M")
  fh.printf "%s %.4f\n", d,rain
end
fh.close

rain1 = 3600 * stn['rfact']*(r1 - r1h)/(lt1 - lh1)
print "Rain 1hr #{rain1}\n" if opt_t
h.clear

dock[0] = Time.at(dock[0].to_i).strftime("%Y-%m-%d %H:%M") 

lcdstr=''
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
      d2, logdate, gtemp, maxt, mint, temp, press, humid, rain1,
      dewpt, dock[0], dock[2], dock[1], dock[4]
    puts "Latest #{d2}"
    if now.hour == 0 and now.min  == 0
      dbh.do "insert into daily values (?,?,?,?)", d2, r1,0,logdate
      if now.day == 1
	dbh.do "insert into monthly values (?,?,?,?)", d2,r1,0,logdate
      end
    end
    dbh.commit if use_t
  rescue
    dbh.rollback if use_t
  end
  if now.min == 0
    unless lcdon.nil? and  lcdoff.nil?
      if now.hour == lcdon
	lcdstr="backlight on\n" 
      elsif now.hour == lcdoff
	lcdstr="backlight off\n" 
      end
    end
  end
end

rmax += 0.01
hmax *= 1.05
hmax = 100 if hmax < 1
hmax = 100 if hmax > 100

cells = RainCalc.raincalc dbh, Time.at(d2), r1, stn['rfact']
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
  begin
    File.rename "_#{gf}.#{itype}","#{gf}.#{itype}"
  rescue
  end
end


print "Done Graphs #{(Time.now- t_0)}s\n" if opt_t

if dodock == 0
  %w(wspeed tide wdirn).each do |gf|
    begin 
      File.unlink("#{gf}.#{itype}")
    rescue
    end
  end
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

sm_req=true
begin
  sm_req = false if File.mtime("/tmp/.sm.dat").day == now.day
rescue
end

if sm_req
  system("sun_moon -f /tmp/.sm.dat")
  if opt_t
    STDERR.puts "running sun_moon"
    STDERR.print IO.read("/tmp/.sm.dat")
  end
end

nc = 0
sollist=[]
File.open('/tmp/.sm.dat') do |f|
  f.each do |l|
    l.chomp!
    k,v=l.split("\t")
    sollist.push({ 'scol' => ((nc & 1) == 0) ? 'grey1' : 'grey2',
		  'solnam' => k, 'solval' => v })
    nc += 1
  end
end

raintimes=''
raintimes = "start: #{Time.at(rast).strftime("%H:%M")} " if rast != 0
raintimes += "last: #{Time.at(radr).strftime("%H:%M")} " if radr != 0
raintimes += "end: #{Time.at(raet).strftime("%H:%M")}" if raet != 0
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

wchill=''
if wsok and dock[2] and temp
  kws = dock[2]*1.852
  if kws > 3.0 && temp < 5.0
    wchill = "(Wind Chill %.1f&deg;C)" % (13.12 + 0.6215 * temp + 
					 ( 0.3965*temp - 11.37)*(kws**0.16))
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
	     'wx_notice' => wx_notice,
             'sollist' => sollist,
	     'wchill' => wchill
           })
File.open('w.html', 'w') do |f|
  f.print tmpl.output
end
File.rename('w.html','index.html')

rstr = ''
rstr += "@#{Time.at(radr).strftime("%H:%M")}" if radr != 0 and rain1 > 0
dwstr = (humid) ? "%.1f" % dewpt : 'N/A'

tmpl.load 'wx.static.tmpl'
tmpl.param({
	     'udate' => d2,
	     'idate' => Time.at(d2).strftime("%FT%T%z"),
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
	     'udate' => d2,
	     'idate' => Time.at(d2).strftime("%FT%T%z"),
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
File.open('.wx_static.dat', 'w') do |f|
  f.print tmpl.output
end
File.rename('.wx_static.dat','wx_static.dat')

if File.pipe?(FIFO) 
  rstr=rstr[1..-1]
  rtxt=nil
  rtxt0 = "#{"%0.1f" % rain1mm}/#{rstr}/#{"%.1f" % rain24mm}"
  if dock[2]
    ndir = (((dock[1] + 22.5) % 360) / 45).to_i
    rtxt1 = "#{DIRS[ndir]}#{"%.0f" % dock[2]}"
    lr = rtxt1.size
    rtxt = "#{rtxt0.ljust(20-lr)}#{rtxt1}"
  else
    rtxt = "#{rtxt0.center(20)}"
  end
  l4="line4\n0\n==EOD==\n#{rtxt}\n==EOD==\n"
  begin
    fd=IO.sysopen(FIFO,File::WRONLY|File::NONBLOCK)
    IO.open(fd,'w') {|f| f.syswrite("#{lcdstr}#{l4}") }
  rescue
    unless File.exists?('/tmp/pert.fail')
      IO.popen("mutt -s 'pertd failure' -x jrh@roo",'w') do |p|
	p.puts("wplot write to pertd failure")
      end
    end
    File.open('/tmp/pert.fail','w') {|f| f.puts('failed')}
  end
end

if opt_u and (opt_t or (now.min % 10 == 0))
  stemp = 
  w = { 'humid' => humid,
    'temp' => temp,
    'dewpt' => dewpt,
    'pres' => press,
    'rain' => rain1,
    'rain24' => rain24,		
    'udate' => now.to_i,
    'stemp' => stemp1
  } 
  Wug.upload(stn, opt_t, w) if stn['wu_user']
end 

