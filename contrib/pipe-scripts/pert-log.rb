#!/usr/bin/ruby
log=nil
FIFO='/tmp/pertd2.fifo'
if File.pipe?(FIFO)
  if File.exists?('/tmp/.p.log')
    log=''
  end
  
  v=Array.new(6)
  ARGF.each do |l|
    log << l if log
    case l
    when /OTMP0\s+(\S+)\s+/
      v[0] = "%5.1f" % $1.to_f
    when /GHT\s+(\S+)\s+/
      v[1] = "%5.1f" % $1.to_f
    when /OTMP1\s+(\S+)\s+/
      v[2] = "%5.1f" % $1.to_f
    when /OTMP2\s+(\S+)\s+/
      v[3] = "%5.1f" % $1.to_f
    when /STMP1\s+(\S+)\s+/
      v[4] = "%5.1f" % $1.to_f
    when /OPRS\s+(\S+)\s+/
      v[5] = "%4.0f" % $1.to_f
    end
  end
  ln=''
  ln << "line1\n0\n==EOD==\nExtr #{v[0]} GHT #{v[1]}\n==EOD==\n" if(v[0] && v[1])
  ln << "line2\n0\n==EOD==\nIntr #{v[2]} Gge #{v[3]}\n==EOD==\n" if(v[2] && v[3])
  ln << "line3\n0\n==EOD==\nSoil #{v[4]} Pres #{v[5]}\n==EOD==\n"if(v[4] && v[5])
  if ln.size > 0
    begin
      fd=IO.sysopen(FIFO,File::WRONLY|File::NONBLOCK) 
      IO.open(fd,'w') {|f| f.syswrite(ln) }
    rescue
    end
  end
  if log and log.size > 0
    File.open('/tmp/.p.log','w'){|f| f.puts log}
  end
end
  
#12345678901234567890
#Extr xx.x GHT xx.x	OTMP0 GHT
#Intr xx.x Gge xx.x	OTMP1 OTMP2
#Soil xx.x Pres xxxx	STMP1 OPRS
