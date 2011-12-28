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

module Wug
  G = -9.80665
  R = 287.04
  WHOST = 'weatherstation.wunderground.com'
  WURL = '/weatherstation/updateweatherstation.php'

  def Wug.CtoF c
    ('%.1f' % ((9.0/5.0)*c + 32.0))
  end
  
  def Wug.mbtoin mb
    ("%.3f" % (mb/33.8638864))
  end

  def Wug.upload stn, flag, w
    q = {}
    q['humidity'] = (w['humid']) ? ("%.1f" % w['humid']) : 'N/A'
    q['tempf'] = CtoF w['temp']
    q['dewptf'] = (w['dewpt']) ? CtoF(w['dewpt']) : 'N/A'
    q['baromin'] = mbtoin(w['pres'])
    q['softwaretype'] = stn['software']
    q['weather'] = q['clouds'] = 'NA'
    q['dateutc'] = Time.at(w['udate']).gmtime.strftime("%Y-%m-%d %H:%M")
    q['rainin'] = w['rain']
    q['dailyrainin'] = w['rain24'] if  w['rain24']
    q['soiltempf'] = CtoF w['stemp'] if  w['stemp']
    q['solarradiation'] = w['solar'] if w['solar']

    url = WURL << '?action=updateraw&ID=' << stn['wu_user'] << 
      '&PASSWORD=' << stn['wu_pass']

    q.keys.sort.each do |s|
      url << '&' << s << '=' << q[s].to_s
    end

    uri = URI.escape('http://'+WHOST+url)
    uri = URI.parse(uri)
    if flag.nil?
      res = Net::HTTP.get_response(uri)
      puts "#{res.message} #{res.code}"
      puts res.body
    else
      puts uri
    end
  end  
end

if __FILE__ == $0
  require 'mongo'
  flag = (ARGV.size > 0)
  now = Time.now.to_i;
  conn = Mongo::Connection.new('heffalump')
  db = conn.db('wx')
  coll = db.collection('station')
  stn = coll.find_one()
  coll = db.collection('latest')
  w = coll.find_one({}, {:sort => [['udate', Mongo::DESCENDING]]})
  Wug.upload stn, flag, w
end
