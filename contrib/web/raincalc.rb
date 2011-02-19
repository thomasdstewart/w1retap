#!/usr/bin/env ruby

require 'rubygems'
require 'sequel'

module RainCalc

  def RainCalc.getrfact(rfacts, date)
    res=nil
    rfacts.each do |r|
      res=r[:rfact]
      break if r[:rstamp] < date
    end
    res
  end

  def RainCalc.raincalc dbh, now, rainf, rfacts
    avg = [ 89, 61, 66, 48, 56, 53, 41, 56, 66, 79, 84, 89 ]
    s = dbh['select udate,rain0 from daily order by udate desc limit 7']
    ds = []
    falls = []
    ds[0] = now
    falls[0] = rainf.to_f
    
    s.each do |row|
      ds << row[:udate]
      falls << row[:rain0]
    end
    
    n = falls.length
    cells = []
    
    1.upto(n-1) do |i|
      t = Time.at(ds[i])
      minch = (falls[i-1] - falls[i]) * RainCalc.getrfact(rfacts,ds[i])
      mm = minch * 25.4

      cells << sprintf("%s:%.2f:%.1f:", t.strftime("%a %d %b"), 
		       minch.to_f, mm.to_f)
    end

    s = dbh['select udate,rain0 from monthly order by udate desc limit 2']
    base = falls[0];
    s.each do |row|
      d = row[:udate]
      m0 = row[:rain0]
      minch = (base-m0) * RainCalc.getrfact(rfacts,d)
      mm = minch *25.4
      t = Time.at(d)
      av = avg[t.month-1]
      cells << sprintf("%s:%.2f:%.1f:%d", t.strftime("%B %Y"), minch, mm,av)
      base = m0
    end
    cells 
  end
end
if __FILE__ == $0
  dsn = ARGV[0] || 'postgres:///sensors'
  dbh = Sequel.connect dsn
  now = Time.now.to_i
  rainf = dbh["select max(value) from readings where name='RGC0'"].first[:max]
  factrs = dbh[:rfacts].order(:rstamp.desc).all
  cells = RainCalc.raincalc dbh, now, rainf, factrs
  cells.each do |c|
    puts c
  end
end
