#!/usr/bin/ruby
# -*- coding: utf-8 -*-

log=nil
FIFO='/tmp/pertd2.fifo'
if File.writable? '/tmp/.p.log'
  log=''
end
v=[]

rrdtmpl=[]
rrdvals=[]
ttime_t=0

ARGF.each do |l|
  l = l.force_encoding("utf-8") rescue l
  log << l if log
  l.chomp!
  stamp,name,value,units,timet = l.split(/\t/)
  exit unless ((timet.to_i % 120) == 0)
  if File.pipe?(FIFO)
    case name 
    when 'OTMP0'
      v[0] = "%5.1f" % value.to_f
    when 'GHT'
      v[1] = "%5.1f" % value.to_f
    when 'OTMP1'
      v[2] = "%5.1f" % value.to_f
    when 'OTMP2'
      v[3] = "%5.1f" % value.to_f
    when 'STMP1'
      v[4] = "%5.1f" % value.to_f
    when 'OPRS'
      v[5] = "%4.0f" % value.to_f
    end
  end
end

if v.size > 0
  ln=''
  v[0] ||= '     '
  v[2] ||= '     '
  v[4] ||= '     '
  ln << "line1\n0\n==EOD==\nExtr #{v[0]} GHT #{v[1]}\n==EOD==\n" 
  ln << "line2\n0\n==EOD==\nIntr #{v[2]} Gge #{v[3]}\n==EOD==\n" 
  ln << "line3\n0\n==EOD==\nSoil #{v[4]} Pres #{v[5]}\n==EOD==\n"
  if ln.size > 0
    begin
      fd=IO.sysopen(FIFO,File::WRONLY|File::NONBLOCK) 
      IO.open(fd,'w') {|f| f.syswrite(ln) }
    rescue
    end
  end
end

if log and log.size > 0
  File.open('/tmp/.p.log','w'){|f| f.puts log}
end

#12345678901234567890
#Extr xx.x GHT xx.x	OTMP0 GHT
#Intr xx.x Gge xx.x	OTMP1 OTMP2
#Soil xx.x Pres xxxx	STMP1 OPRS
