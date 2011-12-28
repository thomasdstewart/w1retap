#!/usr/bin/ruby

require 'mongo'
require 'optparse'
require 'zlib'

class RainPlot
  AVG = [ 89, 61, 66, 48, 56, 53, 41, 56, 66, 79, 84, 89 ]
  def getrfact(rfacts, date)
    res=nil
    rfacts.each do |r|
      res=r['rfact']
      break if r['rstamp'] < date
    end
    res
  end

  def initialize
    ARGV.options do |opts|
      opts.banner = "Usage: #{File.basename $0} [options]"
      opts.on('-d', "--display") {@display=true}
      opts.on('-p', "--pdf") {@pdf=true}
      opts.on('-v', "--verbose") {@verbose=true}
      opts.on('-t', "--title TITLE") {|o| @title=o}
      opts.on('-s', "--start TIME") {|o| @start=o}
      opts.on('-e', "--end TIME") {|o| @last=o}
      opts.on('-?', "--help", "Show this message") {puts opts; exit}
      begin
	rest = opts.parse!
      rescue
	puts opts ; exit
      end
    end
    @title ||= "Annual Rainfall"
    @now = Time.now
    @out = @title.gsub(/ /,'')
    unless @last
      @last = @now.strftime("%Y-%m-01")
      @curr=true
    end
    unless @start
      y = @now.year-1
      m = @now.month+1
      if m == 13
	m=1
	y+=1
      end
      @start = 	"%04d-%02d-01" % [y,m]
    end
    @start = Time.parse(@start)
    @last = Time.parse(@last)
  end

  def conspire
    conn = Mongo::Connection.new('heffalump')
    db = conn.db("wx")
    factrs = []
    coll=db.collection('rfacts')
    coll.find({},{:sort => [['rstamp', Mongo::ASCENDING]]}).each do |r|
      factrs << r
    end
    now=Time.new
    coll = db.collection('monthly')

    lm = nil
    lv = nil
    ly = nil
    mx = 0
    t1 = 0
    t2 = 0
    fact = nil

    arry=[]
    res = coll.find({'stamp' => {"$gte" => @start, "$lte" => @last}},
		    {:sort => [['stamp', Mongo::ASCENDING]] })
    res.each do |r|
      if lv 
	STDERR.puts r.inspect if @verbose
	ds="%04d-%02d" % [ly,lm]
	fact = getrfact(factrs, r['udate'])
	dv = ((r['rain0'] - lv) * fact *25.4 + 0.5).to_i
	av = AVG[lm-1]
	t1 += av
	t2 += dv
	mx = av if av > mx
	mx = dv if dv > mx
	arry << [ds,dv,av]
      end
      lv = r['rain0']
      lm = r['stamp'].localtime.month
      ly = r['stamp'].localtime.year
    end

    if @curr
      coll = db.collection('readings')
      # most of the time this will work ...
      cr = coll.find_one({}, {:sort => [['date', Mongo::DESCENDING]]})
      abort "No cr" unless cr['RGC0']
      cval = cr['RGC0'].to_i
      dv = ((cval - lv) * fact * 25.4 + 0.5).to_i
      av = AVG[@now.month-1]
      ds="%04d-%02d" % [@now.year,@now.month]
      arry << [ds,dv,av]
      t1 += av
      t2 += dv
      mx = av if av > mx
      mx = dv if dv > mx
    end

    mx = (10*(mx * 1.15))/10

    File.open("#{@out}.dat", 'w') do |f|
      arry.each do |r|
	STDERR.puts r.inspect if @verbose
	f.puts r.join(" ")
      end
    end

gplot=<<EOF
set key top center
set key box 
set grid

set style data histograms
set style histogram cluster gap 5
set boxwidth 0.8 relative
set xtic rotate by -45 scale 0

set xlabel
set format x "%Y-%m"
set xdata time
set timefmt "%Y-%m" 
set autoscale

set title "#{@title}"
set ylabel "mm"
set yrange [ 0 : #{mx} ]
show label

set terminal svg font "Droid Sans,9" rounded
set output ".#{@out}.svg"

plot '#{@out}.dat' using 1:2 :xticlabels(1) \\
    t "Actual #{t2}mm" w boxes fs pattern 1  lc rgb "red", \\
    '' u 1:($2 + 1.5):2 notitle with labels, \\
    '' using 1:3 t "Historic #{t1}mm" w lines lw 2  lc rgb "green"

set terminal pdfcairo font "Droid Sans,9" linewidth 4 rounded size 27.3cm,21cm
set output ".#{@out}.pdf"
replot

EOF

    File.open("arain.plt",'w') {|f| f.puts gplot }
    system "gnuplot arain.plt 2>/dev/null"
    Zlib::GzipWriter.open("#{@out}.svgz") do |gz|
	gz << IO.read(".#{@out}.svg")
	gz.close
    end
    File.unlink(".#{@out}.svg")
    File.rename ".#{@out}.pdf", "#{@out}.pdf"
  end
  def draw
    system("eog #{@out}.svg") if @display
    system("evince #{@out}.pdf") if @pdf
  end
end

ret = 0
if File.exists? "wx_static.dat"
  File.open("wx_static.dat") do |f|
    f.each do |l|
      if l.match(/^Rainfall: 0.0mm/)
	ret = 255
	break
      end
    end
  end
end

if ret == 0
  r = RainPlot.new
  r.conspire
  r.draw
end
exit ret
