#!/usr/bin/env ruby

require 'dbi'

module RainCalc
  def RainCalc.raincalc dbh, now, rainf, factr
    avg = [ 89, 61, 66, 48, 56, 53, 41, 56, 66, 79, 84, 89 ]
    s = dbh.execute 'select udate,rain0 from daily order by udate desc limit 7'
    ds = []
    falls = []
    ds[0] = now
    falls[0] = rainf.to_f
    
    s.fetch do |row|
      ds << row[0]
      falls << row[1].to_f
    end
    s.finish
    
    n = falls.length
    cells = []
    
    1.upto(n-1) do |i|
      minch = (falls[i-1] - falls[i]) * factr
      mm = minch * 25.4
      t = Time.at(ds[i].to_i)
      cells << sprintf("%s:%.2f:%.1f:", t.strftime("%a %d %b"), 
		       minch.to_f, mm.to_f)
    end

    s = dbh.execute 'select udate,rain0 from monthly order by udate desc limit 2'
    base = falls[0];
    s.fetch do |row|
      d = row[0]
      m0 = row[1].to_f
      minch = (base-m0)*factr
      mm = minch *25.4
      t = Time.at(d.to_i)
      av = avg[t.month-1]
      cells << sprintf("%s:%.2f:%.1f:%d", t.strftime("%B %Y"), minch, mm,av)
      base = m0
    end
    s.finish
    cells 
  end
end
if __FILE__ == $0
  dsn = ARGV[0] || "dbi:Pg:sensors"
  dbh = DBI.connect(dsn)
  now = Time.now.to_i
  rainf = dbh.select_one("select max(value) from readings where name='RGC0'")[0]
  factr = 0.0105
  cells = RainCalc.raincalc dbh, now, rainf, factr
  cells.each do |c|
    puts c
  end
end
