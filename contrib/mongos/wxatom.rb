#!/usr/bin/ruby

require 'optparse'
require 'net/ftp'
require 'atom'
require 'uuid'
require 'timeout'

def mkentry f,t,c,uuid
  ts=t.to_s
  f.updated = t
  f.entries << Atom::Entry.new do |e|
    e.title = "Weather in Netley Marsh #{ts}"
    e.summary  = "Jonathan and Daria's Weather Station"
    e.links <<  Atom::Link.new(:href => "http://www.daria.co.uk/wx/")
    e.content = Atom::Content::Html.new(c)
    e.updated = t
    e.id = "urn:uuid:#{uuid.generate}"
  end
  while f.entries.size > 24
    f.entries.shift
  end
#  puts     f.entries.size
end

opt_h = opt_d = nil

opts = OptionParser.new
opts.on("-h", "--host HOST", String) {|val| opt_h = val }
opts.on("-d", "--dir DIR", String) {|val| opt_d = val }
opts.parse(ARGV)

Dir.chdir(opt_d) unless opt_d.nil?

rr=[]
t= Time.now
File.open('wx_static.dat') do |f|
  f.each do |l|
    l.chomp!
    r=l.split(': ')
    next if r[1] =~ /^N\/A/
    ts = r[1] if r[0] == 'Date'
    rr  << r
  end
end

c= '<table>'
rr.each { |r| c << "<tr><td>#{r[0]}:</td><td>#{r[1]}</td></tr>" }
c << '</table>'

uuid = UUID.new
feed = nil
if File.exists? 'wx.atom'
  feed = Atom::Feed.load_feed(File.open('wx.atom'))
  mkentry feed,t,c,uuid
else
  feed = Atom::Feed.new do |f|
    f.links << Atom::Link.new(:href => "http://www.daria.co.uk/wx/")
    f.title = "Jonathan and Daria's Weather Station"
    f.subtitle = "Weather in Netley Marsh"
    f.authors << Atom::Person.new(:name => "Jonathan Hudson")
    f.id = "urn:uuid:#{uuid.generate}"
    mkentry f,t,c,uuid
  end
end
File.open('wx.atom','w') {|f| f.puts feed.to_xml}

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
      ftp.put 'wx.atom'
    end
  end
end
