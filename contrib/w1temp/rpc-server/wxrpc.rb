#!/usr/bin/ruby
require "xmlrpc/client"

uri = ARGV[0] || "http://kanga/wx.cgi"
begin
  server = XMLRPC::Client.new2(uri)
#  result = server.call 'system.listMethods'
  result = server.call('w1retap.currentWeather')
  result.each do |r|	 
    puts "#{r['name']}: #{r['value']} #{r['units']}"
  end
rescue  Exception => e
  puts e.to_s
end

