#!/usr/bin/ruby

# Copyright (c) 2006 Jonathan Hudson <jh+w1retap@daria.co.uk>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

require 'socket'

module CWOP
  HOST = 'cwop.aprs.net'
  PORT = 14580
  PORT1 = 23
  
  def CWOP.CtoF c
    ((9.0/5.0)*c + 32.0).to_i
  end

  def CWOP.fixll fmt,flg,val
    sgn = ((val < 0) ? flg[1] : flg[0]).chr
    val = val.abs
    d = val.to_i
    m = (val - d) * 60.0
    (fmt % [d,m]) + "#{sgn}"
  end

  def CWOP.upload stn, flag, w
    s = "#{stn['cwop_user']}>APRS,TCPXX*:"
    s << Time.at(w['udate']).gmtime.strftime("@%d%H%Mz")
    s << fixll("%02d%05.2f", 'NS', stn['stnlat']) + '/' +
      fixll("%03d%05.2f", 'EW', stn['stnlong'])
    s << '_.../...g...'
    s << "t%03d" % CtoF(w['temp'])
    s << ((w['rain']) ? "r%03d" % (w['rain']*100.0).to_i : '')
    s << ((w['rain24']) ? "p%03d" % (w['rain24']*100.0).to_i : '')
    s << ((w['humid']) ? ("h%02d" % (w['humid'].to_i % 100)) : '')
    s << "b%05d" % (w['pres']*10.0).to_i
    s << stn['software']

    unless flag
      TCPSocket.open(HOST,PORT) do |skt|
	skt.puts "user #{stn['cwop_user']} pass -1 vers #{stn['software']}\r"
	skt.puts "#{s}\r"
	if File.exists?("/tmp/cwop.log")
	  File.open("/tmp/cwop.log",'a') do |f|
	    f.puts "user #{stn['cwop_user']} pass -1 vers #{stn['software']}\r"
	    f.puts "#{s}\r"
	  end
	end
      end
    else
      puts s
    end
  end  
end

if __FILE__ == $0
  require 'mongo'
  require 'optparse'

  opt_s = 'roo'
  flag = nil

  ARGV.options do |opts|
    opts.on('-t','--test', 'Test mode') {flag = true }
    opts.on("-s", "--sensor_db HOST", String) {|o| opt_s = o }
    begin
      opts.parse!
    rescue
      puts opts ; exit
    end
  end

  now = Time.now.to_i;
  conn = Mongo::Connection.new(opt_s)
  db = conn.db('wx')
  coll = db.collection('station')
  stn = coll.find_one()
  coll = db.collection('latest')
  w = coll.find_one({}, {:sort => [['udate', Mongo::DESCENDING]]})
  CWOP.upload stn, flag, w
end
