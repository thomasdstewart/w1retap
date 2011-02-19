#!/usr/bin/ruby

require 'rubygems'
require 'optparse'
require 'rss/2.0'
require 'rss/1.0'
require 'net/ftp'
require 'timeout'

opt_s = 'dbi:Pg:sensors'
opt_h = opt_d = nil

opts = OptionParser.new
opts.on("-s", "--sensor_db DBNAME", String) {|val| opt_s = val }
opts.on("-h", "--host HOST", String) {|val| opt_h = val }
opts.on("-d", "--dir DIR", String) {|val| opt_d = val }
opts.parse(ARGV)

Dir.chdir(opt_d) unless opt_d.nil?

rr=[]
ts = Time.now.to_s
File.open('wx_static.dat') do |f|
  f.each do |l|
    l.chomp!
    r=l.split(': ')
    next if r[1] =~ /^N\/A/
    ts = r[1] if r[0] == 'Date'
    rr  << r
  end
end

sfx = (test ?>, 'gtemp.svgz', 'gtemp.png') ? 'svgz' : 'png'

rss = []
files = %W{gtemp.#{sfx} press.#{sfx} temp.#{sfx} tide.#{sfx} wdirn.#{sfx}
 wspeed.#{sfx} humid.#{sfx} thermo.#{sfx} wdirn.#{sfx} winddirn.#{sfx}
 solar.#{sfx} index.html temps.#{sfx} rain.#{sfx} wx_static.xml wx_static.dat }

t = Time.now
if t.min == 1 || t.min == 31

  files << 'moon.png' if t.min == 1
  
  [ 1, 2 ].each do |v|
    rss[v] = RSS::Rss.new("#{v}.0")
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

  item = RSS::Rss::Channel::Item.new
  item.title = "Weather in Netley Marsh #{ts}"
  c= '<table>'
  rr.each { |r| c << "<tr><td>#{r[0]}:</td><td>#{r[1]}</td></tr>" }
  c << '</table>'
  item.description = c
  item.pubDate = t
  item.link = "http://www.daria.co.uk/wx/"
  rss[1].channel.items << item
  rss[2].channel.items << item

  File.open('wx.rss1.xml','w') do |f|
    f.puts rss[1].to_s
  end

  File.open('wx.rss2.xml','w') do |f|
    f.puts rss[2].to_s
  end

  files << 'wx.rss2.xml' << 'wx.rss1.xml'

end

if not opt_h.nil?
  user = pass = nil
  begin
    a = File.open(ENV['HOME']+'/.netrc').grep(/machine\s#{opt_h}/)
    (user,pass) = a[0].match(/login\s(\S+)\spassword\s(\S+)/)[1..2]
  rescue
  end

  Timeout.timeout(300) do
    Net::FTP.open(opt_h) do |ftp|
      ftp.passive = ftp.binary = true
      ftp.login user,pass
      ftp.chdir 'wx'
      files.each do |f|
	ftp.put f if File.exists?(f)
      end
    end
  end
end
