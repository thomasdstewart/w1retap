#!/usr/bin/ruby
require 'dbi'
include Math

# Wet bulb temperature from 
# <http://www.faqs.org/faqs/meteorology/temp-dewpoint/>
# t in °C, rh as % (0-100), pr as hPa
def wettemp t,rh,pr
  e = (rh/100.0) * 0.611*exp(17.27*t/(t+237.3))
  td = (116.9+237.3*log(e))/(16.78-log(e))
  gamma =  0.000066*pr
  delta = 4098*e/(td+237.3)**2
  tw = ((gamma*t)+(delta*td))/(gamma+delta)
  [td,tw]
end

def get_my_cnf
  u=p=nil
  File.open(File.join(ENV['HOME'],'.my.cnf')) do |f|
    f.each do |l|
      case l
      when /^user=(\S+)/
	u=$1
      when /^password=(\S+)/
	p=$1
      end  
      break if u and p
    end
  end
  [u,p]
end

ts=t=pr=rh=nil
snow=nil

dsn = (ARGV[0] || 'dbi:Mysql:sensors-logger:localhost')

ARGF.each do |l|
  case l
  when /(\S+)\s+AirTemperature\s+(\S+)\s+/
    ts = $1
    t = $2.to_f
  when /BrayBarometer\s+(\S+)\s+/
    pr = $1.to_f
  when  /RH\s+(\S+)\s+/
    rh = $1.to_f
  when  /(\S+)\s+VAD\s+(\S+)\s+/
    snow = (10.075-($2.to_f))*24.6062992
    ts=$1 unless ts
  end
end

if ts and (snow || (t and pr and rh))
  user,pass=get_my_cnf
  if user
    if t
      dp,tw =  wettemp t,rh,pr
      db = DBI.connect(dsn,user,pass)
      db.execute 'insert into readings(date,name,value) values (?,?,?)',
	ts,'WetBulb',("%.3f" % tw)
    end

    if snow
      db.execute 'insert into readings(date,name,value) values (?,?,?)',
	ts,'SnowDepth',("%.1f" % snow)
    end 
    db.disconnect
  end
end
