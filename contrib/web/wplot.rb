#!/usr/bin/ruby

$: << ENV['HOME'] + '/lib/ruby'

require 'optparse'
require 'socket'
require 'dbi'
require 'html/template'

require 'raincalc.rb'
require 'svgwind.rb'
require 'svgthermo.rb'
require 'wug.rb'

BASEDIR = '/var/www/roo/wx'
G = -9.80665
R = 287.04

def pr_msl p0, temp, z
  zdiff = (0 - z)
  kt = temp +  273.15
  x = (G * zdiff / (R * kt))
  p0 * Math.exp(x)
end

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

$opt_S = 'dbi:Pg:sensors'
$opt_W = 'dbi:Pg:docks'

opts = OptionParser.new
opts.on("-t")                        {|val| $opt_t = val }
opts.on("-f")                        {|val| $opt_f = val }
opts.on("-s")                        {|val| $opt_s = val }
opts.on("-g")                        {|val| $opt_s = val }
opts.on("-l")                        {|val| $opt_s = val }
opts.on("-S", "--sensor_db DBNAME", String) {|val| $opt_S = val }
opts.on("-W", "--docks_db  DBNAME", String) {|val| $opt_W = val }
rest = opts.parse(ARGV)

base = rest[0] || BASEDIR

if $opt_s
  imime='"image/svg+xml"'
  itype='svg'
  $opt_p = 1
else
  imime='"image/png"'
  itype='png'
end

if test(?d,base) and $opt_t.nil?
  Dir.chdir(base) 
else
  $opt_t = true
end

print "Running on #{Socket.gethostname}\n with\t#{$opt_S}\n\t#{$opt_W}\n" if $opt_t

ithen = now.to_i - 24*3600
sleep 10 if (now.min % 10) == 0 and !$opt_t

f=File.open('w0.dat','w')

dbh = DBI.connect($opt_W, 'jrh', '')
smt = dbh.execute "select reportdate, wind_dirn, wind_speed, wind_speed1, tide from observations where reportdate > #{ithen} order by reportdate"

dock = []
smt.each do |s|
  f.printf("%s %.1f %.1f %.1f %s\n", 
	   Time.at(s[0].to_i).strftime("%Y-%m-%d.%H:%M"), 
	   s[1].to_f, 0.8 * s[2].to_f, 0.8 * s[3].to_f, s[4])
  dock = s
end

smt.finish
f.close
dbh.disconnect

fh = []
fh[0] = File.open('w1.dat', 'w')
fh[1] = File.open('w2.dat', 'w')
fh[2] = File.open('w3.dat', 'w')
fh[3] = File.open('w4.dat', 'w')
fh[4] = File.open('w5.dat', 'w')

dbh = DBI.connect($opt_S, 'jrh', '')

stn = {}
s = dbh.execute('select * from station')
s.fetch_hash do |r|
  stn = r
end  
s.finish

maxt = -999
mint = 999
hmax = -1
ltim = -1
d1 = -1
d2 = -2
gtemp = 0
temp = press = humid = nil
sdate=edate=nil
ld = r0 = r1 = 0
lt0 = lt1 = nil
ast = 0
rpress = 0
d = ''

smt = dbh.execute "select date, name, value from readings where date > #{ithen} order by date"
smt.each do |s|
  ast = s[0].to_i
  ltim = ast if ltim == -1
  case s[1]
  when 'GHT'
    gtemp = s[2].to_f
    maxt = gtemp if gtemp > maxt
    mint = gtemp if gtemp < mint

  when 'OTMP0'
    temp = s[2].to_f

  when 'OPRS'
    rpress = s[2].to_f

  when 'OHUM'
    humid = s[2].to_f
    hmax = humid if humid > hmax

  when 'RGC0'
    if (r0 == 0)
      r0 = s[2].to_f
      lt0 = ast
    end
    r1 = s[2].to_f
    lt1 = ast
  end

  if ltim != ast
    d = Time.at(ast).strftime("%Y-%m-%d.%H:%M")
    fh[0].printf("%s %.1f\n", d, gtemp)
    fh[1].printf("%s %.1f\n", d, temp)
    press=pr_msl(rpress, temp, stn['altitude'].to_f)
    fh[2].printf("%s %.1f\n", d, press)
    fh[3].printf("%s %.1f\n", d, humid)
    fh[4].printf("%s %.1f %.1f\n", d, gtemp, temp)
    sdate=d if sdate.nil?
    edate=d
    ltim = ast
  end
end

if ltim != ast
  d = Time.at(ast).strftime("%Y-%m-%d.%H:%M")
  fh[0].printf("%s %.1f\n", d, gtemp)
  fh[1].printf("%s %.1f\n", d, temp)
  press=pr_msl(rpress, temp, stn['altitude'])
  fh[2].printf("%s %.1f\n", d, press)
  fh[3].printf("%s %.1f\n", d, humid)
  fh[4].printf("%d %.1f %.1f\n", d, gtemp, temp)
  sdate=d if sdate.nil?
  edate=d
  ltim = ast
end
smt.finish
fh.each {|f| f.close}
d2 = lt1

dewpt= dewpoint temp, humid
logdate = d.sub(/\./, ' ')

rain = 0
rain24 = 0

if stn['rfact']
  l1h = lt1 - 3600
  r1h = r1
  as = []

  st = dbh.execute "select value from readings where date = #{l1h} and name='RGC0'"
  st.each do |s|
    as = s
  end
  st.finish

  if !as.empty?
    r1h =as[0].to_i
    print "Rain1(0) #{r1h}\n" if $opt_t
  else
    st = dbh.execute "select date, value from readings where name = 'RGC0' and date > #{l1h} order by date  asc limit 1"
    st.each do |s|
      as = s
    end
    st.finish
    if !as.empty?
      l1h = as[0]
      r1h = as[1].to_i
    end
  end
  stn['rfact'] = stn['rfact'].to_f
  rain = 3600 * stn['rfact']*(r1 - r1h)/(lt1 - l1h)
  print "Rain #{rain}\n" if $opt_t
  if ((r1 - r0) and (lt1 - lt0))
    rain24 = 24*3600 * stn['rfact']*(r1 - r0)/(lt1 - lt0)
  end
  print "Rain24 #{rain24}\n" if $opt_t
else
  rain = 0;
end

dock[0] = Time.at(dock[0].to_i).strftime("%Y-%m-%d %H:%M") 
use_t = true # Workaround for b0rken SQLite3  DBI
if now.min % 10 == 0 or $opt_f
  begin
    dbh['AutoCommit'] = false
  rescue
    use_t = false
  end
  begin
    dbh.do "delete from latest where udate < #{ithen}"
    dbh.do "insert into latest values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
      d2, logdate, gtemp, maxt, mint, temp, press, humid, rain,
      dewpt, dock[0], dock[2], dock[1], dock[4]

    if now.hour == 0 and now.min  == 0
      dbh.do "insert into daily values (?,?,?)", d2, r1,0
      if now.day == 1
	dbh.do "insert into monthly values (?,?,?)", d2,r1,0
      end
    end
    dbh.commit if use_t
  rescue
    dbh.rollback if use_t
  end
end

rmax = 0.0;

fh = File.open 'w6.dat', 'w'
s = dbh.execute "select gstamp,rain from latest order by udate"
s.fetch do |r|
  date,value = r[0],r[1].to_f
  date = date.to_s
  date.sub!(/(....-..-..) (..:..):00/,'\1.\2')
  fh.print "#{date} #{value}\n"
  rmax = value if value > rmax
end
s.finish
fh.print "#{edate} #{rain}\n"
fh.close

rmax += 0.01
hmax *= 1.05
hmax = 100 if hmax > 100

cells = RainCalc.raincalc dbh, d2, r1, stn['rfact']
dbh.disconnect

SvgThermo.makeimage gtemp, 150, 40 
SvgWind.makeimage dock[1], 120, 120

ENV['GDFONTPATH'] = '/usr/share/fonts/truetype:/usr/share/fonts/truetype/freefont/';

print "Start Graphs, edate=#{edate} rmax=#{rmax}\n" if $opt_t
t_0 = Time.now;

plotopt = ($opt_s.nil?) ? '-font FreeSans -png -crop 0.2,0.5,4.2,2.5' : '-svg'
cmd = "ploticus #{plotopt} edate=#{edate} rmax=#{rmax} hmax=#{hmax} currain=#{rain} wxcrop.plt"
system cmd
print "#{cmd}\n" if $opt_t

if ($opt_p and $opt_s.nil?)
  puts "Start Convert" if $opt_t
  %w{gtemp humid press temp temps tide wdirn wspeed}.each do |x|
    system "rsvg -w 400 -h 200 #{x}.svg #{x}.png"
  end
end

print "Done Graphs #{(Time.now- t_0)}s\n" if $opt_t

tmpl = HTML::Template.new('wx.ht.html')
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

tmpl.param({
             'gtype' => itype,
             'temp' => "%.1f&deg;C / %.1f&deg;F" % [temp, CtoF(temp)],
             'date' => logdate,
             'gtemp' => "%.1f&deg;C / %.1f&deg;F" % [gtemp, CtoF(gtemp)],
             'gmin' => "%.1f&deg;C / %.1f&deg;F" % [mint, CtoF(mint)],
             'gmax' => "%.1f&deg;C / %.1f&deg;F" % [maxt, CtoF(maxt)],
             'press' => "%.1f hPa / %.1f inHg" % [press, mbtoin(press)],
             'humid' => "%.1f" % humid,
             'dewpt' => "%.1f&deg;C / %.1f&deg;F" % [dewpt, CtoF(dewpt)],
             'rain' => "%.2fin / %.1fmm" % [rain, rain*25.4],
             'rain24' => "%.2fin / %.1fmm" % [rain24, rain24*25.4],
             'pdate' => dock[0],
             'wdirn' => dock[1],
             'wspeed' => dock[2],
             'wgust' => dock[3],
             'tide' => dock[4],
             'rainlist' => rainlist
           })
File.open('w.html', 'w') do |f|
  f.print tmpl.output
end
File.rename('w.html','index.html')

tmpl.load 'wx.static.tmpl'
tmpl.param({
	     'udate' => d2,
	     'idate' => Time.at(d2).strftime("%FT%T%z"),
	     'temp' => "%.2f" % temp,
	     'press' => "%.2f" % press,
	     'humid' => "%.2f" % humid,
	     'rain' => "%.4f" % rain,
	     'dewpt' => "%.2f" % dewpt,
	     'gtemp' => "%.2f" % gtemp,
	     'wspd' => dock[2],
	     'wdir' => dock[1],
	     'tide' => dock[4]
	   })

File.open('wx_static.xml', 'w') do |f|
  f.print tmpl.output
end

tmpl.load('wx.text.tmpl')
tmpl.param({
	     'udate' => d2,
	     'idate' => Time.at(d2).strftime("%FT%T%z"),
	     'temp' => "%.2f" % temp,
	     'press' => "%.2f" % press,
	     'humid' => "%.2f" % humid,
	     'rain' => "%.4f" % rain,
	     'dewpt' => "%.2f" % dewpt,
	     'gtemp' => "%.2f" % gtemp,
	     'wspd' => dock[2],
	     'wdir' => dock[1],
	     'tide' => dock[4]
	   })
File.open('/tmp/.wx_static.dat', 'w') do |f|
  f.print tmpl.output
end

w = {}
w['humid'] = humid
w['temp'] = temp
w['dewpt'] = dewpt
w['pres'] = press
w['rain'] = rain
w['udate'] = now.to_i

Wug.upload(stn, w, $opt_t) if now.min % 10 == 0

