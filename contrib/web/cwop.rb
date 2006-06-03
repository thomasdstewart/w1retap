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

require 'dbi'
require 'socket'

module CWOP
  HOST = 'rotate.aprs.net'

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
    s += Time.at(w['udate']).gmtime.strftime("@%d%H%Mz")
    s += fixll("%02d%05.2f", 'NS', stn['stnlat']) + '/' +
      fixll("%03d%05.2f", 'EW', stn['stnlong'])
    s += '_.../...g...'
    s += "t%03d" % CtoF(w['temp'])
    s += (w['rain']) ? "r%03d" % (w['rain']*100.0).to_i : ''
    s += (w['rain24']) ?"p%03d" % (w['rain24']*100.0).to_i : ''
    s += (w['humid']) ? ("h%02d" % (w['humid'].to_i % 100)) : ''
    s += "b%05d" % (w['pres']*10.0).to_i
    s += stn['software']

    if flag.nil?
      TCPSocket.open(HOST,23) do |skt|
	skt.puts "user #{stn['cwop_user']} pass -1 vers #{stn['software']}\r"
	skt.puts "#{s}\r"
      end
    else
      puts s
    end
  end  
end

if __FILE__ == $0
  flag = ARGV[0]
  now = Time.now.to_i;
  dbh = DBI.connect('dbi:Pg:sensors', '', '')
  s = dbh.execute 'select * from station'
  stn = s.fetch_hash
  s.finish
  s = dbh.execute "select * from latest order by udate desc limit 1"
  w  = s.fetch_hash
  s.finish
  flag = true if (now - w['udate']) > 180 
  CWOP.upload stn, flag, w
end
