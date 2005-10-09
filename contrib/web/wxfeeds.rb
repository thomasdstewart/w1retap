#!/usr/bin/ruby

require 'optparse'
require 'dbi'
require 'rss/2.0'
require 'rss/1.0'
require 'net/ftp'

$opt_s = 'dbi:Pg:sensors'
$opt_h = $opt_d = nil

opts = OptionParser.new
opts.on("-s", "--sensor_db DBNAME", String) {|val| $opt_s = val }
opts.on("-h", "--host HOST", String) {|val| $opt_h = val }
opts.on("-d", "--dir DIR", String) {|val| $opt_d = val }
rest = opts.parse(ARGV)

Dir.chdir($opt_d) unless $opt_d.nil?

dbh = DBI.connect($opt_s, 'jrh', '')
s = dbh.execute 'select * from station'
stn = s.fetch_hash
s.finish

rr = []
s = dbh.execute "select * from latest where (udate % 1800) = 0 order by udate asc"
s.fetch_hash do |r|
  rr << r
end

rss = []
t = Time.now
[ 1, 2 ].each do |v|
  rss[v] = RSS::Rss.new(v.to_s+'.0')
  chan = RSS::Rss::Channel.new
  chan.description = "Jonathan and Daria's Weather"
  chan.link = "http://www.daria.co.uk/wx/"
  chan.title = "Jonathan and Daria's Weather Station"
  chan.copyright = '(c) 2005 jh+weather@daria.co.uk'
  chan.pubDate = t
  chan.language = 'en'
  rss[v].channel = chan
  rss[v].encoding = 'utf-8'
end

last = 0
rr.each do |r|
  item = RSS::Rss::Channel::Item.new
  item.title = "Weather in Netley Marsh #{r['gstamp']}"
  fn = "rss_#{r['udate']}.html"
  c= %Q{<table>
  	<tr><td>Time</td><td>#{r['gstamp']}</td></tr>
 	<tr><td>Greenhouse</td><td>#{'%.1f' % r['temp']}°C</td></tr>
        <tr><td>Outside</td><td>#{'%.1f' % r['temp']}°C</td></tr>
        <tr><td>Pressure</td><td>#{'%.1f' % r['pres']}hPa</td></tr>
        <tr><td>Rel. Humidity</td><td>#{'%.1f' % r['humid']}%</td></tr>
        <tr><td>Rainfall</td><td>#{'%.1f' % r['rain']}in/hr</td></tr>
        <tr><td>Dew Point</td><td>#{'%.1f' % r['dewpt']}°C</td></tr>
        </table>}
  item.description = c
  item.pubDate = Time.at(r['udate'])
  item.link = "http://www.daria.co.uk/wx/#{fn}.html"
  rss[1].channel.items << item
  rss[2].channel.items << item
  if not File.exists?(fn) 
    File.open(fn,'w') do |f|
      f.print %Q{<html><head<title>weather</title><meta http-equiv="refresh"
	      content="0;url=http://www.daria.co.uk/wx/"></head>
              <body><a href="http://www.daria.co.uk/wx/">Weather</a><p>
              #{c}<p></body></html>}
    end
  end
  last = r['udate']
end

File.open('wx.rss1.xml','w') do |f|
  f.puts rss[1].to_s
end

File.open('wx.rss2.xml','w') do |f|
  f.puts rss[2].to_s
end

lrss = last - 3600*24

Dir.glob("rss_*.html") do |f|
  File.delete(f) if f < "rss_#{lrss}.html"
end

if not $opt_h.nil?
  user = pass = nil
  begin
    a = File.open(ENV['HOME']+'/.netrc').grep(/machine\s#{$opt_h}/)
    (user,pass) = a[0].match(/login\s(\S+)\spassword\s(\S+)/)[1..2]
  rescue
  end
  Net::FTP.open($opt_h) do |ftp|
    ftp.passive = ftp.binary = true
    ftp.login user,pass
    ftp.chdir 'wx'
    files = ftp.nlst
    files.each do |m|
      d = m.match(/^rss_(\d+).html$/)
      ftp.delete m if !d.nil? and d[1].to_i < lrss
    end

    files = %w{gtemp.png press.png temp.png tide.png wdirn.png wspeed.png
            humid.png thermo.png wdirn.png winddirn.png index.html
            temps.png rain.png wx_static.xml wx.rss2.xml wx.rss1.xml}
    files << "rss_#{last}.html"
    files.each do |f|
      ftp.put f
    end
  end
end


