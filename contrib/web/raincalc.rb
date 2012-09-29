#!/usr/bin/env ruby

require 'sequel'
module RainCalc

  def RainCalc.getrfact(rfacts, date)
    res=nil
    rfacts.each do |r|
      res=r[:rfact]
      break if r[:date] < date
    end
    res
  end

  def RainCalc.raincalc db, now, rainf, rfacts
    avg = [ 89, 61, 66, 48, 56, 53, 41, 56, 66, 79, 84, 89 ]
    s = db[:daily].order(:date.desc).limit(7)
    ds = []
    falls = []
    ds[0] = now
    falls[0] = rainf.to_f
    
    s.each do |row|
      ds << row[:date]
      falls << row[:rain0]
    end
    
    n = falls.length
    cells = []
    
    1.upto(n-1) do |i|
      t = ds[i]
      minch = (falls[i-1] - falls[i]) * RainCalc.getrfact(rfacts,ds[i])
      mm = minch * 25.4
      cells << sprintf("%s:%.2f:%.1f:", t.strftime("%a %d %b"), 
		       minch.to_f, mm.to_f)
    end

    s = db[:monthly].order(:date.desc).limit(2)
    base = falls[0];
    s.each do |row|
      d = row[:date]
      m0 = row[:rain0]
      minch = (base-m0) * RainCalc.getrfact(rfacts,d)
      mm = minch *25.4
      av = avg[d.month-1]
      cells << sprintf("%s:%.2f:%.1f:%d", d.strftime("%B %Y"), minch, mm,av)
      base = m0
    end
    cells 
  end
end
if __FILE__ == $0
require 'json/pure'
  dsn = (ARGV[0] || 'postgres:///wx')
  db = Sequel.connect dsn
  now = Time.now
  res= db[:readings].order(:date.desc).first
  rainf=JSON.parse(res[:wxdata])['RGC0']
  factrs = db[:rfacts].order(:date.desc).all
  cells = RainCalc.raincalc db, now, rainf, factrs
  cells.each do |c|
    puts c
  end
end
