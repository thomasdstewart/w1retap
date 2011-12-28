#!/usr/bin/ruby

require 'mongo'

module RainCalc

  def RainCalc.getrfact(rfacts, date)
    res=nil
    rfacts.each do |r|
      res=r['rfact']
      break if r['rstamp'] < date
    end
    res
  end

  def RainCalc.raincalc db, now, rainf, rfacts
    coll=db.collection('daily')
    avg = [ 89, 61, 66, 48, 56, 53, 41, 56, 66, 79, 84, 89 ]
    ds = []
    falls = []
    ds[0] = now
    falls[0] = rainf.to_f
    s = coll.find({}, {:sort => [['udate', Mongo::DESCENDING]]}).limit(7)
    s.each do |row|
      ds << row['udate']
      falls << row['rain0']
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

    coll = db.collection('monthly')
    s = coll.find({}, {:sort => [['udate', Mongo::DESCENDING]]}).limit(2)
    base = falls[0];
    s.each do |row|
      d = row['udate']
      m0 = row['rain0']
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
  opt_d = ARGV[0] || 'roo'
  conn = Mongo::Connection.new(opt_d)
  db = conn.db("wx")
  coll = db.collection('readings')
  rainf = coll.find_one({}, {:sort => [['date', Mongo::DESCENDING]]})["RGC0"]
  now = Time.now.to_i
  factrs=[]
  coll=db.collection('rfacts')
  coll.find({},{:sort => [['rstamp', Mongo::ASCENDING]]}).each do |r|
    factrs << r
  end
  cells = RainCalc.raincalc db, now, rainf, factrs
  cells.each do |c|
    puts c
  end
end
