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
require 'rubygems'
require 'sequel'
require 'html/template'

require 'raincalc.rb'
require 'svgwind.rb'
require 'svgthermo.rb'
require 'wug.rb'
require 'cwop.rb'
require 'inotify'
require 'timeout'

FIFO='/tmp/pertd2.fifo' 
DIRS=%w/N NE E SE S SW W NW/
BASEDIR = '/var/www/roo/wx'
G = -9.80665
R = 287.04
HFACT = 1.0
UPDATEWAIT = 30
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
opt_S = 'postgres:///sensors'
opt_i = 1200
opt_u = true
opt_U = opt_f = opt_t = opt_p = nil
opt_s = 'svgz'
rest=[]

ARGV.options do |opts|
  opts.banner = "Usage: #{File.basename $0} [options] directory"
  opts.on('-t','--test', 'Test mode') {opt_t = true }
  opts.on('-i', '--interval=VALUE','Window for rain calc',
	  Integer) {|o| opt_i = o}
  opts.on('-f', '--force', 'Force time related actions') {opt_f = true}
  opts.on('-p', '--png') {opt_s = nil }
  opts.on('-s', 'No-op') {}
  opts.on('-g', '--graphics=FMT') {|o| opt_s = o}
  opts.on('-u','--no-wundergroud') {opt_u=false}
  opts.on('-U', '--user NAME','User name for database') {|o| opt_U = o }
  opts.on("-S", "--sensor_db Sequel_DBname", String) {|o| opt_S = o }
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

print "Running on #{Socket.gethostname}\n with\t#{opt_S}\n" if opt_t

ithen = now - 24*3600
iithen = ithen.to_i

if (now.min % 10) == 0 and !opt_t
  if File.exists? "/tmp/.w1retap.dat"
    i = Inotify.new
    i.add_watch("/tmp/.w1retap.dat",  Inotify::CLOSE_WRITE)
    begin
      status = Timeout::timeout(UPDATEWAIT) {
	i.each_event { |ev| break }
      }
    rescue Timeout::Error
    ensure
      i.close
    end
  end
end

luser = opt_U || ENV['USER']
dbh = Sequel.connect(opt_S)
dsource=nil
lastdock=nil
dock = {}
docks=[]
wsok = true
dodock = nil
rfacts = dbh[:rfacts].order(:rstamp.desc).all

smt = dbh[:docks].filter('reportdate > ?', iithen).order(:reportdate)
smt.each do |dk|
  docks << dk
end

File.open('w0.dat','w') do |f|
  if docks.size == 0 || docks[0][:reportdate] > iithen + 3600
    dt = Time.at(iithen).strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\tx\tx\tx\tx\n", dt)
  end
  docks.each do |d|
    lastdock = Time.at(d[:reportdate]).strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\t%.1f\t%.1f\t%.1f\t%s\n", lastdock, 
	     d[:wind_dirn].to_f, 0.8 * d[:wind_speed].to_f, 
	     0.8 * d[:wind_speed1].to_f, d[:tide])
  end
  if docks.size > 0
    dock=docks[-1]
    dsource = dock[:source]
    dodock = 1
    if(dock[:reportdate].to_i > dock[:mtime].to_i + 900)
      wsok = false
      lastdock = nil if (iithen > dock[:mtime].to_i)
    end
  end
  if ((docks.size== 0) or ((now - dock[:mtime]).to_i > 3600))
    dt = now.strftime("%Y-%m-%d.%H:%M")    
    f.printf("%s\tx\tx\tx\tx\n", dt)
  end
end

#puts "lastdock #{lastdock} dodock #{dodock}"
if lastdock.nil?
  ld = dbh[:docks].max(:mtime)
  lastdock = Time.at(ld).strftime("%Y-%m-%d %H:%M")
  dodock = nil
  dock[:reportdate] = ld
  dock[:wind_speed] = nil
  dock[:wind_dirn] = nil
  dock[:tide] = nil
else
  lastdock.sub!(/\./, ' ')
  dock[:wind_speed] = 0.8 * dock[:wind_speed].to_f
  dock[:wind_speed1] = 0.8 * dock[:wind_speed1].to_f
end

fh = []
fh[0] = File.open('w1.dat', 'w')
fh[1] = File.open('w2.dat', 'w')
fh[2] = File.open('w3.dat', 'w')
fh[3] = File.open('w4.dat', 'w')
fh[4] = File.open('w5.dat', 'w')
fh[5] = File.open('w6.dat', 'w')

stn = dbh[:station].first
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
edate=0
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

# optimisations for each RDBMS, alas

str = case opt_S
      when /^postgres/
	'select EXTRACT(epoch from date) as date, name, value from readings where date >= ? order by name,date'
      when /^sqlite/
	'select strftime("%s",substr(date,1,19)) as date,name,value from readings where date >=? order by name,date'
      when /^mysql/
	'select UNIX_TIMESTAMP(date) as date, name, value from readings where date >= ? order by name,date'
      else
	'select date, name, value from readings where date >= ? order by name,date'
      end

smt = dbh[str, ithen]
smt.each do |s|
  st = s[:date].to_i
  id = (st - iithen)/120
  unless wxv[id]
    wxv[id] = {} 
    wxv[id][:date] = Time.at(st).strftime("%Y-%m-%d.%H:%M")
  end

  case s[:name]
  when 'GHT'
    gtemp = s[:value]
    maxt = gtemp if gtemp > maxt
    mint = gtemp if gtemp < mint
    wxv[id][:ght] = gtemp

  when 'ITMP1'
    itemp1 = s[:value]
    wxv[id][:itmp1] = itemp1

  when 'ITMP2'
    itemp2 = s[:value]
    wxv[id][:itmp2] = itemp2
    
  when 'OTMP0'
    temp = s[:value]
    tmax = temp if temp > tmax
    tmin = temp if temp < tmin
    wxv[id][:temp] = temp

  when 'OTMP1'
    htemp = s[:value]
    wxv[id][:htemp] = htemp

  when 'OTMP2'
    btemp = s[:value]
    wxv[id][:btemp] = btemp
    
  when 'STMP1'
    stemp1 = s[:value]
    wxv[id][:stemp1] = stemp1

  when 'CFRAME1'
    ctemp1 = s[:value]
    wxv[id][:ctemp1] = ctemp1
    
  when 'OPRS'
    press = s[:value]
    wxv[id][:press] = press

  when 'OHUM'
    humid = s[:value]
    hmax = humid if humid > hmax
    wxv[id][:humid] = humid

  when 'SOLAR'
    wxv[id][:solar] = solar = (s[:value] * SFACTOR).to_i
    smax = solar if solar > smax

  when 'RGC0'
    if (r0 == 0)
      r0 = s[:value].to_i
      lt0 =  st
    end
    r1 = s[:value].to_i
    lt1 = st
    h[st]=s[:value].to_i
  end
  sdate=st if sdate.nil?
  edate=st if st > edate
end

sdate=Time.at(sdate).strftime("%Y-%m-%d.%H:%M")
edate=Time.at(edate).strftime("%Y-%m-%d.%H:%M")

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
rfact = stn[:rfact]

if ((r1 - r0) and (lt1 - lt0))
  rain24 = 24*3600 * rfact *(r1 - r0)/(lt1 - lt0)
end
print "Rain 24hr #{rain24}\n" if opt_t

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

dock[:reportdate] = Time.at(dock[:reportdate]).strftime("%Y-%m-%d %H:%M") #if dodock

lcdstr=''

if (now.min % 10 == 0 or opt_f) and !opt_t and ! File.exists?('/tmp/.w1retap.lock')
  dbh.transaction do
    dbh[:latest].filter{udate < iithen}.delete
    dbh[:latest].insert(d2, logdate, gtemp, maxt, mint, temp, press, humid, rain1,
			dewpt, dock[:reportdate], dock[:wind_speed], dock[:wind_dirn], dock[:tide])
    if now.hour == 0 and now.min  == 0
      dbh[:daily].insert(d2, r1,0,logdate)
      if now.day == 1
	dbh[:monthly].insert(d2,r1,0,logdate)
      end
    end
  end
  if now.min == 0
    unless lcdon.nil? and  lcdoff.nil?
      if now.hour == lcdon
	lcdstr="backlight on\n" 
      elsif now.hour == lcdoff
	lcdstr="backlight off\n" 
      end
    end
    if (now.hour % 2) == 0
      File.unlink '/tmp/.moon.info' rescue nil
    end
  end
end

rmax += 0.01
hmax *= 1.05
hmax = 100 if hmax < 1
hmax = 100 if hmax > 100

cells = RainCalc.raincalc dbh, Time.at(d2), r1, rfacts
cpuc = dbh["select value from readings where name = 'CPU' order by date desc Limit 1"].first[:value]
dbh.disconnect

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
	   500
	 when 5,6,7,8
	   800
	 end

solmax = smax *1.2 if smax > solmax

cmd = "#{PLOTTER} #{plotopt} edate=#{edate} rmax=#{rmax} hmax=#{hmax} currain=#{rain} dodock=#{dodock.to_i} solmax=#{solmax} #{plotplt}"
system cmd
print "#{cmd}\n" if opt_t

unless opt_s
  system "rsvg -h 150 -w 40 _thermo.svgz _thermo.png"
  system "rsvg -h 120 -w 120 _winddirn.svgz _winddirn.png"
  File.unlink '_thermo.svgz' ,'_winddirn.svgz'
end

=begin
if (opt_p and opt_s.nil?)
  puts "Start Convert" if opt_t
  %w{gtemp humid press temp temps tide wdirn wspeed solar}.each do |x|
    svg = "_#{x}.svgz"
    if File.exists?(svg)
      system "rsvg -w 400 -h 200 #{svg} #{x}.png"
      File.unlink(svg)
    end
  end
end
=end

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
if wsok and dock[:wind_speed] and temp
  kws = dock[:wind_speed]*1.852
  if kws > 3.0 && temp < 5.0
    wchill = %Q(Wind Chill %.1f&deg;C) % (13.12 + 0.6215 * temp + 
					 ( 0.3965*temp - 11.37)*(kws**0.16)) 
  end
end

rain1mm = rain1*25.4
rain24mm = rain24*25.4
lastdock << '*' if dsource == 'AVTS'
#istamp = (opt_s) ? itype :  "#{itype}?stamp=#{now.to_i}"
xstamp = "?stamp=#{now.to_i}"
istamp = "#{itype}#{xstamp}"

wspd_s = ("%.1f" % dock[:wind_speed]) rescue 'n/a'
wdir_s = ("%.0f" % dock[:wind_dirn])  rescue 'n/a'
tide_s = ("%.1f" %dock[:tide]) rescue 'n/a'
gust_s = ("%.1f" % dock[:wind_speed1]) rescue 'n/a'

minfo = IO.read('/tmp/.moon.info') rescue nil
mnstate='Moon'
if minfo
  mpct,mlimb=minfo.split("\t")
  mpct=((mpct.to_f * 100 + 0.5)).to_i
  mphase = 'Waxing'
  mphase = (mlimb.to_f < 180) ? 'Waning' : 'Waxing'
  mnstate << " #{mphase} #{mpct}%"
  STDERR.puts mnstate
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
	     'ctemp1' =>  ctemp1S,
	     'stemp' => stemp1S,
	     'htemp' => "%.1f" % htemp,
	     'btemp' => (btemp) ? "%.1f" % btemp : 'N/A',
	     'wspd' => wspd_s,
	     'wdir' => wdir_s,
	     'tide' => tide_s,
	     'solar' => solstr
	   })

File.open('wx_static.xml', 'w') do |f|
  f.print tmpl.output
end

puts "cpu #{cpuc}" if opt_t
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
	     'ctemp1' => ctemp1S,
	     'stemp' => stemp1S,
	     'htemp' => "%.1f" % htemp,
	     'btemp' => (btemp) ? "%.1f" % btemp : 'N/A',
	     'wspd' => wspd_s,
	     'wdir' => wdir_s,
	     'tide' => tide_s,
	     'solar' => solstr,
             'cpu' => cpuc
	   })

  File.open('.wx_static.dat', 'w') do |f|
    f.print tmpl.output
  end
  File.rename('.wx_static.dat','wx_static.dat')

if File.pipe?(FIFO) 
  rstr=rstr[1..-1]
  rtxt=nil
  rtxt1 = '---'
  rtxt0 = "#{"%0.1f" % rain1mm}/#{rstr}/#{"%.1f" % rain24mm}"
  if dock[:wind_speed]
    ndir = (((dock[:wind_dirn].to_f + 22.5) % 360) / 45).to_i
    rtxt1 = "#{DIRS[ndir]}#{"%.0f" % dock[:wind_speed]}"
  end
  lr = rtxt1.size
  rtxt = "#{rtxt0.ljust(20-lr)}#{rtxt1}"
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

#File.open('/tmp/wug.log','w') do |f|
#  f.puts "opt_u #{opt_u}"
#  f.puts "opt_t #{opt_t}"
#  f.puts "now.min #{now.min}"  
#end

if opt_u and ((now.min % 10 == 0) or opt_t)
  w = { :humid => humid,
    :temp => temp,
    :dewpt => dewpt,
    :pres => press,
    :rain => rain1,
    :rain24 => rain24,		
    :udate => now.to_i,
    :stemp => stemp1,
    :solar => solar
  } 
  Wug.upload(stn, opt_t, w) if stn[:wu_user]
end 

