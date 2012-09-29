#!/usr/bin/ruby

# Copyright (c) 2005 Jonathan Hudson <jh+w1retap@daria.co.uk>
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

require 'uri'
require 'net/http'

module Owm
  WURI= 'http://openweathermap.org/data/post'

  def Owm.upload stn, test, w, acnt
    q = {}
    q['lat'] = stn[:stnlat]
    q['long'] = stn[:stnlong]
    q['alt'] = stn[:altitude]
    q['humidity'] = (w[:humid]) ? ("%.1f" % w[:humid]) : 'N/A'
    q['temp'] = w[:temp]
    q['dewpoint'] = (w[:dewpt]) if w[:dewpt]
    q['pressure'] = w[:pres]
    q['rain_1h'] = w[:rain] ? w[:rain]/25.4 : 0
    q['rain_24h'] = w[:rain24]? w[:rain24]/25.4 : 0
    q['lum'] = w[:solar] if w[:solar]
    if test
      puts q.inspect
    else
      uri = URI.parse(WURI)
      http = Net::HTTP.new(uri.host, uri.port)
      request = Net::HTTP::Post.new(uri.request_uri)
      request.basic_auth(acnt[:userid], acnt[:password])
      request.set_form_data(q)
      response = http.request(request)
    end
  end  
end

if __FILE__ == $0
  require 'sequel'
  require 'optparse'

  opt_s = 'postgres:///wx?host=/tmp/'
#  opt_s = 'sqlite:///home/jrh/Projects/wx-sequel/wx.sqlite'
  test = nil

  ARGV.options do |opts|
    opts.on('-f','--fake', 'fakemode') {test = true }
    opts.on("-s", "--sensor_db Sequel_DBname", String) {|o| opt_s = o }
    begin
      opts.parse!
    rescue
      puts opts ; exit
    end
  end
  db = Sequel.connect(opt_s)
  stn = db[:station].first
  w = db[:latest].order(:date.desc).limit(1).first
  acnt = db[:accounts].filter(:name => 'owm').first
  Owm.upload stn, test, w, acnt
end
