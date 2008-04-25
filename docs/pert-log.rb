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
#
#
# Uses the pertd application to display w1retap data on a Pertelian
# X2040 LCD display.
# Configured as:
# log =  w1file=|/usr/local/bin/pert-log.rb

FIFO='/tmp/pertd2.fifo'
if File.pipe?(FIFO)
  v=Array.new(6)
  ARGF.each do |l|
    case l
    when /OTMP0\s+(\S+)\s+/
      v[0] = "%5.1f" % $1.to_f
    when /GHT\s+(\S+)\s+/
      v[1] = "%5.1f" % $1.to_f
    when /OTMP1\s+(\S+)\s+/
      v[2] = "%5.1f" % $1.to_f
    when /OTMP2\s+(\S+)\s+/
      v[3] = "%5.1f" % $1.to_f
    when /STMP1\s+(\S+)\s+/
      v[4] = "%5.1f" % $1.to_f
    when /OPRS\s+(\S+)\s+/
      v[5] = "%4.0f" % $1.to_f
    end
  end
  File.open(FIFO,'a+') do |f|
    f.puts("line1\n0\n==EOD==\nExtr #{v[0]} GHT #{v[1]}\n==EOD==") if(v[0] && v[1])
    f.puts("line2\n0\n==EOD==\nIntr #{v[2]} Gge #{v[3]}\n==EOD==") if(v[2] && v[3])
    f.puts("line3\n0\n==EOD==\nSoil #{v[4]} Pres #{v[5]}\n==EOD==") if(v[4] && v[5])
    end
  end

#12345678901234567890
####################################
#Extr xx.x GHT xx.x	OTMP0 GHT
#Intr xx.x Gge xx.x	OTMP1 OTMP2
#Soil xx.x Pres xxxx	STMP1 OPRS
#Rain from cron script here  ######
