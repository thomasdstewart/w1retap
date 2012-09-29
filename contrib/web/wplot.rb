#!/usr/bin/ruby
# -*- coding: utf-8 -*-

# Copyright (c) 2006-2011 Jonathan Hudson <jh+w1retap@daria.co.uk>
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

Encoding.default_internal = Encoding.default_external = "UTF-8"

$: << './'  << ENV['HOME'] + '/lib/ruby'

require 'optparse'
require 'socket'
require 'sequel'
require 'html/template'

require 'raincalc.rb'
require 'svgwind.rb'
require 'svgthermo.rb'
require 'wug.rb'
require 'wow.rb'
require "sleepy_penguin/sp"

FIFO='/tmp/pertd2.fifo' 
DIRS=%w/N NE E SE S SW W NW/
BASEDIR = '/var/www/roo/wx'
G = -9.80665
R = 287.04
HFACT = 1.0
UPDATEWAIT = (30*1000)
PLOTTER='pl'

# See http://www.cocoontech.com/index.php?showtopic=6452&hl=solar
#
# The formula to get current thru the sense resistor (current
# register volts)/390 ohms = (sensor current) Note: asumes a
# standard Hobby Boards solar sensor The formula to get solar energy
# is: (sensor current) * 1157598 = W/M^2 (solar energy)

SFACTOR=2.9682


def dewpoint ct, humid
  if humid > 0.0
    b = (ct > 0 ) ? 237.3 : 265.5
    es = 610.78 * Math.exp(17.2694 * ct / (ct + b))
    e = humid / 100 * es
    f = Math.log( e / 610.78 ) / 17.2694
    b *  f / ( 1 - f)
  else
    0
  end
end

def CtoF c
  ('%.1f' % ((9.0/5.0)*c + 32.0))
end
  
def mbtoin mb
  ("%.3f" % (mb/33.8638864))
end

now = Time.now
opt_i = 1200
opt_u = true

opt_S = 'postgres:///wx?host=/tmp/'
#opt_S = 'sqlite:///home/jrh/Projects/wx-sequel/wx.sqlite'

opt_U = opt_f = opt_t = opt_p = nil
opt_s = 'svgz'
rest=[]
forcedir=nil

ARGV.options do |opts|
  opts.banner = "Usage: #{File.basename $0} [options] directory"
  opts.on('-t','--test', 'Test mode') {opt_t = true }
  opts.on('-d','--force-dir DIR') { |o| forcedir=o}
  opts.on('-i', '--interval=VALUE','Window for rain calc', Integer) {|o| opt_i = o}
  opts.on('-f', '--force', 'Force time related actions') {opt_f = true}
  opts.on('-p', '--png') {opt_s = nil }
  opts.on('-g', '--graphics=FMT') {|o| opt_s = o}
  opts.on('-u','--no-wundergroud') {opt_u=false}
  opts.on('-U', '--user NAME','User name for database') {|o| opt_U = o }
  opts.on("-S", "--sensor_db_host=HOST" ) {|o| opt_S = o }
  opts.on('-?', "--help", "Show this message") {puts opts; exit}
  begin
    rest = opts.parse!
  rescue
    puts opts ; exit
  end
end
  
base = (rest[0] || BASEDIR)

case opt_s
when nil,'png'
  opt_s = nil
  imime='image/png'
  itype='png'
when 'svg','svgz'  
  imime='image/svg+xml'
  itype=opt_s
else
  abort 'Bad graph type'
end

if test(?d,base) 
  Dir.chdir(base) unless opt_t
else
  opt_t = true
end

Dir.chdir(forcedir) if forcedir

print "Running on #{Socket.gethostname}\n with\t#{opt_S} in #{Dir.pwd}\n" if opt_t

ithen = now - 24*3600

if (now.min % 2) == 0 and !opt_t
  if File.exists? "/tmp/.w1retap.dat"
    ino = SP::Inotify.new
    ino.add_watch("/tmp/.w1retap.dat",SP::Inotify::CLOSE_WRITE|SP::Inotify::DELETE_SELF )
    ep = SP::Epoll.new(nil)
    ep.set(ino,:IN)
    ep.wait(1, UPDATEWAIT) {|flags,io|}
  end
end

luser = opt_U || ENV['USER']

pj=false
db = Sequel.connect(opt_S)

############### Find type / existance of wxdata 
###### from db.schema(:readings) 

if opt_S.match(/^postgres/)
  db.extension :pg_json
  pj=true  
else
 require 'json/pure'
end

dsource='N/A'
lastdock=nil
dock = {}
docks=[]
wsok = true
dodock = nil
rfacts=[]

rfacts = db[:rfacts].order(:date.desc).all

smt = db[:docks].filter('date > ?', ithen).order(:date)
smt.each do |dk|
  docks << dk
end

File.open('w0.dat','w') do |f|
  if docks.size == 0 || docks[0][:date] > ithen + 3600
    dt = ithen.strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\tx\tx\tx\tx\n", dt)
  end
  docks.each do |d|
    lastdock = d[:date].strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\t%.1f\t%.1f\t%.1f\t%s\n", lastdock,
	     d[:wind_dirn], d[:wind_speed], 
	     d[:wind_speed1], d[:tide]) rescue nil
  end
  
  if docks.size > 0
    dock=docks[-1]
    dsource = dock[:source]
    dodock = 1
    if(dock[:date] > dock[:mtime] + 900)
      wsok = false
      lastdock = nil if (ithen > dock[:mtime])
    end
  end
  if ((docks.size== 0) or ((now - dock[:mtime]) > 3600))
    dt = now.strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\tx\tx\tx\tx\n", dt)
  end
end

if lastdock.nil?
  ld = db[:docks].max(:mtime)
  lastdock = ld.strftime("%Y-%m-%d %H:%M")
  dodock = nil
  dock[:date] = ld
  dock[:wind_speed] = dock[:wind_dirn] = dock[:tide] = nil
else
  lastdock.sub!(/\./, ' ')
end

fh = []
fh[0] = File.open('w1.dat', 'w')
fh[1] = File.open('w2.dat', 'w')
fh[2] = File.open('w3.dat', 'w')
fh[3] = File.open('w4.dat', 'w')
fh[4] = File.open('w5.dat', 'w')
fh[5] = File.open('w6.dat', 'w')

stn = db[:station].first
lcdon=lcdoff=nil
lcdon = stn[:lcdon] if stn.has_key?(:lcdon)
lcdoff = stn[:lcdoff] if stn.has_key?(:lcdoff)

maxt = tmax = -999
mint = tmin = 999
hmax = 0
ltim = ithen
d1 = -1
d2 = -2
gtemp = temp = press = humid = htemp = btemp = nil
solar=nil
sdate=nil
edate=Time.at(0)
ld = r0 = r1 = 0
lt0 = lt1 = nil
ast = 0
press = nil
itemp1=nil
itemp2=nil
stemp1=nil
ctemp1=nil
d = ''
h = {}
val = 0
wxv = Array.new(720)
smax = 0

smt=db[:readings].filter('date > ?',ithen).order(:date)
smt.each do |s|
  st = s[:date]
  id = (st - ithen)/120
  unless wxv[id]
    wxv[id] = {} 
    wxv[id][:date] = s[:date].strftime("%Y-%m-%d.%H:%M")
  end
  sd = (pj) ? s[:wxdata] : JSON.parse(s[:wxdata])
  sd.each do |k,v|
    case k
    when 'GHT'
      gtemp = v
      maxt = gtemp if gtemp > maxt
      mint = gtemp if gtemp < mint
      wxv[id][:ght] = gtemp

    when 'ITMP1'
      itemp1 = v
      wxv[id][:itmp1] = itemp1
      
    when 'ITMP2'
      itemp2 = v
      wxv[id][:itmp2] = itemp2
    
    when 'OTMP0'
      temp = v
      tmax = temp if temp > tmax
      tmin = temp if temp < tmin
      wxv[id][:temp] = temp

    when 'OTMP1'
      htemp = v
      wxv[id][:htemp] = htemp

    when 'OTMP2'
      btemp = v
      wxv[id][:btemp] = btemp
      
    when 'STMP1'
      stemp1 = v
      wxv[id][:stemp1] = stemp1
      
    when 'CFRAME1'
      ctemp1 = v
      wxv[id][:ctemp1] = ctemp1
      
    when 'OPRS'
      press = v
      wxv[id][:press] = press

    when 'OHUM'
      humid = v
      hmax = humid if humid > hmax
      wxv[id][:humid] = humid
      
    when 'SOLAR'
      wxv[id][:solar] = solar = (v * SFACTOR).to_i
      smax = solar if solar > smax
      
    when 'RGC0'
      if (r0 == 0)
	r0 = v.to_i
	lt0 =  st
      end
      r1 = v.to_i
      lt1 = st
      h[st]=v.to_i
    end
    sdate=st if sdate.nil?
    edate=st if st > edate
  end
end

latest_date = edate
sdate=sdate.strftime("%Y-%m-%d.%H:%M")
edate=edate.strftime("%Y-%m-%d.%H:%M")

wxv.each do |hs|
  next unless hs
  va = []
  va[0] = hs[:date]

  hs[:press] && fh[2].puts([hs[:date],"%.1f" % hs[:press]].join("\t")) 
  hs[:humid] && fh[3].puts([hs[:date],"%.1f" % hs[:humid]].join("\t")) 
  hs[:solar] && fh[5].puts([hs[:date],hs[:solar]].join("\t")) 

  t2 = va[1] = (hs[:temp]) ? "%.1f" % hs[:temp] : 'x'
  va[2] = (hs[:stemp1]) ? "%.1f" % hs[:stemp1] : 'x'
  va[3] = (hs[:ctemp1]) ? "%.1f" % hs[:ctemp1] : 'x'
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

#if press.to_i < 900 || press.to_i > 1200
#  press = dock[:pressure];
#  File.rename("w_3.dat", "w3.dat")
#end

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
rfact = stn[:rfact].to_f

unless ((r1 - r0).zero? and (lt1 - lt0).zero?)
#  rain24 = 24*3600 * rfact * (r1 - r0) / (lt1 - lt0)
  rain24 = rfact * (r1 - r0)
end

if opt_t
  print "Rain 24hr #{rain24*25.4}\n"
  print "r1 #{r1} r0 #{r0} rfact #{rfact}\n"
  print "lt1 #{lt1}\n"
  print "lt0 #{lt0}\n"
end

rmax = 0.0
lh1 = lt1 - 3600
r1h = 0
rast = 0
raet = 0
lrr = 0
radr = 0
d=nil
fh = File.open 'w7.dat', 'w'
h.keys.sort.each do |k|
  j = (k - opt_i)
  rain = 0
  if h.has_key?(j)
    rain = (h[k] - h[j]) * rfact
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
  if k >= lh1 and r1h == 0
    r1h = h[k]
    lh1 = k
  end
  v = (rain == 0.0 && lrr == 0.0) ? 'x' : "%.4f" % rain
#  v = "%.4f" % rain  
#  if (d && rain != 0.0 && lrr == 0.0)
#    fh.puts "#{d} 0.0"
#  end
  d = Time.at(k).strftime("%Y-%m-%d.%H:%M")
  fh.puts "#{d} #{v}"
  lrr = rain
end
fh.close

rain1 = 3600 * rfact*(r1 - r1h)/(lt1 - lh1)
print "Rain 1hr #{rain1}\n" if opt_t
h.clear

lcdstr=''

if ((now.min % 10 == 0 || opt_f) && opt_t.nil?)
  xdstamp = dock[:date]

  doc = {:date => latest_date, :ght => gtemp, :gmax => maxt, 
    :gmin => mint, :temp => temp, :pres => press, :humid => humid, 
    :rain => rain1, :dewpt => dewpt, :dstamp => xdstamp, 
    :wspeed => dock[:wind_speed], :wdirn => dock[:wind_dirn], 
    :tide => dock[:tide]}

  db[:latest].insert(doc)
  db[:latest].filter{date < latest_date - (7*3600*24)}.delete

  if now.hour == 0 and now.min  == 0
    doc = { :date => latest_date, :rain0 => r1, :rain1 => 0}
    db[:daily].insert(doc)
    if now.day == 1
      db[:monthly].insert(doc)
    end
  end
end
if now.min == 0
  #unless lcdon.nil? && lcdoff.nil?
  #  if now.hour == lcdon.to_i
  #    lcdstr="backlight on\n" 
  #  elsif now.hour == lcdoff.to_i
  #    lcdstr="backlight off\n" 
  #  end
  #end
  if (now.hour % 2) == 0
    File.unlink '/tmp/.moon.info' rescue nil
  end
end

dock[:date] = dock[:date].strftime("%Y-%m-%d %H:%M") #if dodock

rmax += 0.01
hmax *= 1.05
hmax = 100 if hmax < 1
hmax = 100 if hmax > 100


cells = RainCalc.raincalc db, latest_date, r1, rfacts

cpui = db[:cpuinfo].order(:date.desc).first
cpuc = cpui[:cputemp]
fans = cpui[:fan]

# This is no longer the greenhouse temp!
gval = temp || -100
SvgThermo.makeimage gval, 150, 40, (opt_s == 'svg')
SvgWind.makeimage((dodock ? dock[:wind_dirn].to_i : -1), 120, 120, (opt_s == 'svg'))

ENV['GDFONTPATH'] = '/usr/share/fonts/truetype:/usr/share/fonts/truetype/freefont'

#["wdirn.png", "wspeed.png", "tide.png" ].each do |f|
#  begin
#    File.delete(f)
#  rescue
#  end
#end

print "Start Graphs, edate=#{edate} rmax=#{rmax}\n" if opt_t
t_0 = Time.now;


if opt_s
  plotopt = "-#{opt_s}"
  plotplt = 'wxsvg.plt'
else
  plotopt = '-png -crop 0.2,0.5,4.2,2.5 -font FreeSans'
  plotplt = 'wxpng.plt'
end

%w(wspeed tide wdirn).each do |gf|
  begin 
    File.unlink("#{gf}.#{itype}")
  rescue
  end
end

solmax = case now.month
	 when 1,2,11,12
	   200
	 when 3,4,9,10
	   300
	 when 5,6,7,8
	   500
	 end

solmax = smax * 1.2 if smax > solmax

cmd = "#{PLOTTER} #{plotopt} edate=#{edate} rmax=#{rmax} hmax=#{hmax} currain=#{rain} dodock=#{dodock.to_i} solmax=#{solmax} #{plotplt}"
system cmd
print "#{cmd}\n" if opt_t

unless opt_s
  system "rsvg -h 150 -w 40 _thermo.svgz _thermo.png"
  system "rsvg -h 120 -w 120 _winddirn.svgz _winddirn.png"
  File.unlink '_thermo.svgz' ,'_winddirn.svgz'
end

%w(thermo winddirn gtemp temp press humid temps wspeed tide wdirn rain solar).each do |gf|
  begin
    File.rename "_#{gf}.#{itype}","#{gf}.#{itype}"
  rescue
  end
end

print "Done Graphs #{(Time.now- t_0)}s\n" if opt_t

unless dodock
  File.symlink("nodock.#{itype}", "wspeed.#{itype}")
  File.symlink("nodata.#{itype}", "tide.#{itype}")
  File.symlink("nodata.#{itype}", "wdirn.#{itype}")
end

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

sm_req=false
begin
  sm_req = true if (File.mtime("/tmp/.sm.dat").day != now.day) 
rescue
  sm_req = true
end

if sm_req
  ts = Time.utc(now.year,now.month,now.day,0,0,0).to_i
  %x(sun_moon -f /tmp/.sm.dat -t #{ts} 2>&1)
  if opt_t
    STDERR.puts "running sun_moon"
    STDERR.print IO.read("/tmp/.sm.dat")
  end
end

minfo=nil
if !File.exists?('/tmp/.moon.info') or !File.exists?("moon.png")
  minfo = %x(sun_moon -M)
end

if minfo
  mpct,mlimb=minfo.split("\t")
  mpct=mpct.to_f
  midx = (28*mpct).to_i
  if mlimb.to_f < 180
    midx = 55 - midx
  end
  mimage="moon_images/m%02d.png" % midx
  if opt_t
    STDERR.puts "Moon state #{minfo}"
    STDERR.puts "Moon #{mimage}" 
  end
require 'fileutils'
  FileUtils.ln_sf mimage, "moon.png"
  File.open('/tmp/.moon.info','w') {|f| f.puts minfo}
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
ctemp1 =nil if ctemp1 == 'x'
gtemp =nil if gtemp == 'x'
stemp =nil if stemp == 'x'

hustr = (humid) ? "%.1f" % humid : 'N/A'
solstr = (solar) ? solar.to_s : 'N/A'
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
if ctemp1 
  begin
    ctemp1S = "%.1f" % ctemp1
    ctemp1C = "#{ctemp1S}&deg;C"
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
if wsok and dock['wind_speed'] and temp
  kws = dock['wind_speed']*1.852
  if kws > 3.0 && temp < 5.0
    wchill = %Q(Wind Chill %.1f&deg;C) % (13.12 + 0.6215 * temp + 
					 ( 0.3965*temp - 11.37)*(kws**0.16)) 
  end
end

rain1mm = rain1*25.4
rain24mm = rain24*25.4

xstamp = "?stamp=#{now.to_i}"
istamp = "#{itype}#{xstamp}"

wspd_s = ("%.1f" % dock[:wind_speed]) rescue 'n/a'
wdir_s = ("%.0f" % dock[:wind_dirn])  rescue 'n/a'
tide_s = ("%.1f" % dock[:tide]) rescue 'n/a'
gust_s = ("%.1f" % dock[:wind_speed1]) rescue 'n/a'

minfo = IO.read('/tmp/.moon.info') rescue nil
mnstate='Moon'
if minfo
  mpct,mlimb=minfo.split("\t")
  mpct=((mpct.to_f * 100 + 0.5)).to_i
  mphase = 'Waxing'
  mphase = (mlimb.to_f < 180) ? 'Waning' : 'Waxing'
  mnstate << " #{mphase} #{mpct}%"
  STDERR.puts mnstate if opt_t
end

tmpl = HTML::Template.new("wx.htt.#{itype}.html")
tmpl.param({
             'gtype' => istamp,
             'xstamp' => xstamp,
             'imime' => imime,
             'temp' => "%.1f&deg;C / %.1f&deg;F" % [temp, CtoF(temp)],
             'date' => logdate,
             'gtemp' => "#{gtempC} / #{gtempF}",
             'gmin' => (mint) ? "%.1f&deg;C / %.1f&deg;F" % [mint, CtoF(mint)] : 'N/A',
             'gmax' => (maxt) ? "%.1f&deg;C / %.1f&deg;F" % [maxt, CtoF(maxt)] : 'N/A',
	     'itemp1' => itemp1C,
	     'itemp2' => itemp2C,
	     'ctemp1' => ctemp1C,
	     'stemp' => "#{stemp1S}&deg;C",
	     'tmax' => "%.1f&deg;C" % [tmax], 
	     'tmin' => "%.1f&deg;C" % [tmin], 
             'press' => "%.1f hPa / %.1f inHg" % [press, mbtoin(press)],
             'humid' => hustr,
	     'solar' => solstr,
             'dewpt' => (humid) ? "%.1f&deg;C / %.1f&deg;F" % [dewpt, CtoF(dewpt)] : 'N/A',
             'rain' => "%.1fmm / %.2fin" % [rain1mm, rain1],
             'rain24' => "%.1fmm / %.2fin" % [rain24mm, rain24],
             'pdate' => lastdock,
             'wdirn' => wdir_s,
             'wspeed' => wspd_s,
             'wgust' => gust_s,
             'tide' => tide_s,
	     'dsource' => dsource,
             'rainlist' => rainlist,
	     'raintimes' => raintimes,
	     'wx_notice' => wx_notice,
             'sollist' => sollist,
	     'wchill' => wchill,
	     'mnstate' => mnstate,
           })
File.open('w.html', 'w') do |f|
  f.print tmpl.output
end
File.rename('w.html','index.html')

rtm = (radr != 0 and rain1 > 0) ? Time.at(radr).strftime("%H:%M") : nil
rstr = ''
rstr << "@#{rtm} " if rtm
rstr24 = "%.1f" % rain24mm
dwstr = (humid) ? "%.1f" % dewpt : 'N/A'
tstr = "%.1f" % temp
r1str = "%.1f" % rain1mm
htempS = "%.1f" % htemp
btempS = (btemp) ? "%.1f" % btemp : 'N/A'
tsS = Time.at(d2).strftime("%FT%T%z")
pstr = "%.1f" % press
fans = -1 unless fans
cpuc = -1 unless cpuc
hostd="%.1f°C / %.0f" % [cpuc,fans]
monstr = "#{rainlist[-2]['valm']}/#{rainlist[-2]['vala']}"
r24 = "#{rstr24}mm/24h"

jarry=nil
File.open('.wx_static.json', 'w') do |f|
  jarry = [{"name" => "Temperature", "value" => tstr, "units" => "°C"},
    { "name" => "Pressure", "value" => pstr, "units" => "hPa"},
    { "name" => "Humidity", "value" => hustr, "units" => "%"},
    { "name" => "Rainfall", "value" => r1str, "units" => "mm/hr#{rstr}"}]

  if rain24mm > 0.0
     jarry << { "name" => "24hr rain", "value" => rstr24, "units" => "mm/24hr"}
   end
  windstr = ""
  if dock[:wind_speed]
    ndir = (((dock[:wind_dirn].to_f + 22.5) % 360) / 45).to_i
    windstr = "%.f kts, %.f° (#{DIRS[ndir]})" %  [dock[:wind_speed],dock[:wind_dirn]]
  end

  [{ "name" => "Monthly", "value" => monstr, "units" => "mm"},
    { "name" => "Dewpoint", "value" => dwstr, "units" => "°C"},
    { "name" => "Greenhouse", "value" => gtempS, "units" => "°C"},
    { "name" => "Interior", "value" => htempS, "units" => "°C"},
    { "name" => "Garage", "value" => btempS, "units" => "°C"},
    { "name" => "Soil", "value" => stemp1S, "units" => "°C"},
#    { "name" => "Propagator1", "value" => itemp1S, "units" => "°C"},
#    { "name" => "Propagator2", "value" => itemp2S, "units" => "°C"},
    { "name" => "Front Garden", "value" => ctemp1S, "units" => "°C"},
    { "name" => 'Solar', "value" => solstr, "units" => " W/m^2"},
    { "name" => "Wind", "value" => windstr, "units" => ""},
#    { "name" => "Direction", "value" => wdir_s, "units" => "degrees"},
    { "name" => "Tide", "value" => tide_s, "units" => "metres"},
    { "name" => "Date", "value" => tsS, "units" => ""},
    { "name" => "Host", "value" => hostd, "units" => "rpm"},
    { "name" => "Dock Source", "value" => dsource, "units" => ""}
  ].each do |j|
    jarry << j
  end
  f.puts jarry.to_json
end
File.rename('.wx_static.json','wx_static.json')

nc = 0
pcol=nil
jarry.each do |j|
  j['pcol'] = ((nc & 1) == 0) ? 'grey1' : 'grey2'
  nc += 1
end
jarry.pop
jarry.pop
jarry[-1]['value']=Time.at(d2).strftime("%d %b %R")


fnz = (rain24 > 0) ? 'rain.svgz' : 'solar.svgz'
fnx = File.readlink('cond.svgz')
unless fnx == fnz
  File.symlink(fnz, ".cond.svgz")
  File.rename(".cond.svgz", "cond.svgz")
end

tmpl = HTML::Template.new("wx.page.html")
tmpl.param({'wdata' => jarry, 'gtype' => istamp})
File.open('.wxpage.html', 'w') do |f|
  f.print tmpl.output
end
File.rename(".wxpage.html", "wxpage.html")
rstr << "[#{rstr24}mm/24h]"

puts "cpu #{cpuc}" if opt_t
tmpl.load('wx.text.tmpl')
tmpl.param({
	     'udate' => d2,
	     'idate' => tsS,
	     'temp' => tstr,
	     'press' => pstr,
	     'humid' => hustr,
	     'rain' => r1str,
	     'rstr' => rstr,
	     'monstr' => monstr,
	     'dewpt' => dwstr,
	     'gtemp' => gtempS,
	     'itemp1' => itemp1S,
	     'itemp2' => itemp2S,
	     'ctemp1' => ctemp1S,
	     'stemp' => stemp1S,
	     'htemp' => htempS,
	     'btemp' => btempS,
	     'wspd' => wspd_s,
	     'wdir' => wdir_s,
	     'tide' => tide_s,
	     'solar' => solstr,
             'hostd' => hostd,
	     'dsource' => dsource
	   })


begin
  File.open('.wx_static.dat', 'w') do |f|
    f.print tmpl.output
  end
  File.rename('.wx_static.dat','wx_static.dat')
rescue 
  File.open('/tmp/static.fail','w') {|f| f.puts('static')}  
end

#if File.pipe?(FIFO) 
#  rtxt=nil
#  rtxt1 = '---'
#  rtxt0 = r1str
#  rtxt0 << "/#{rtm}" if rtm
#  rtxt0 << '/' << rstr24
#  if dock['wind_speed']
#    ndir = (((dock['wind_dirn'].to_f + 22.5) % 360) / 45).to_i
#    rtxt1 = "#{DIRS[ndir]}#{"%.0f" % dock['wind_speed']}"
#  end
#  lr = rtxt1.size
#  rtxt = "#{rtxt0.ljust(20-lr)}#{rtxt1}"
#  l4="line4\n0\n==EOD==\n#{rtxt}\n==EOD==\n"
#  begin
#    fd=IO.sysopen(FIFO,File::WRONLY|File::NONBLOCK)
#    IO.open(fd,'w') {|f| f.syswrite("#{lcdstr}#{l4}") }
#  rescue
#    unless File.exists?('/tmp/pert.fail')
#      IO.popen("mutt -s 'pertd failure' -x jrh@roo",'w') do |p|
#	p.puts("wplot write to pertd failure")
#      end
#    end
#    File.open('/tmp/pert.fail','w') {|f| f.puts('failed')}
#  end
#end

if opt_u and ((now.min % 10 == 0) or opt_t)
  w = { :humid => humid,
    :temp => temp,
    :dewpt => dewpt,
    :pres => press,
    :rain => rain1,
    :rain24 => rain24,		
    :date => latest_date,
    :stemp => stemp1,
    :solar => solar
  } 
  acnt = db[:accounts].filter(:name => 'wug').first
  Wug.upload(stn, opt_t, w, acnt) if acnt[:userid]
  acnt = db[:accounts].filter(:name => 'wow').first
  Wow.upload(stn, opt_t, w, acnt) if acnt[:userid]
end 


