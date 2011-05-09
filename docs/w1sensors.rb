#!/usr/bin/ruby
# -*- coding: utf-8 -*-

require 'optparse'

##
 # Copyright (C) 2008 Jonathan Hudson <jh+w1retap@daria.co.uk>
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU General Public License
 # as published by the Free Software Foundation; either version 2
 # of the License, or (at your option) any later version.
 # 
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program; if not, write to the Free Software
 # Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
##

# Create an initial w1retap configuration from w1find
# 
# ***** you may need to modify this for your own schema *****

class MakeSensors

  def initialize
    @fn=STDOUT.fileno
    @fbase=nil
    @idx = 0
    ARGV.options do |opt|
      opt.banner = %Q(#{File.basename($0)} [options] [file|stdin]
e.g. w1find DS2490-1 | w1sensors.rb -o /tmp/w1_sensors-setup.sql
     w1find DS2490-1 | w1sensors.rb | sqlite3 sensors.db)
      opt.on('-f','--file-based-config'){@fbase=true}
      opt.on('-o','--output FILE') {|o| fn=o}
      opt.on('-?', "--help", "Show this message") {puts opt; exit}
      begin
	opt.parse!
      rescue
	puts opt ; exit
      end
    end
  end

  def output f,arry,err=nil
    arry.fill(nil,arry.length..9)
    str = case @fbase
	  when false,nil
	    @idx += 1
	    arry.collect! {|x| x.nil? ? 'NULL' : "'#{x}'" }
	    "#{err}INSERT into w1sensors values (#{@idx},#{arry.join(',')});"
	  when true
	    arry.join('|')
	  end
    f.puts str
  end

  def buildconfig
    last=nil
    no=1
    cl=nil
    File.open(@fn,'w') do |f|
      ARGF.each do |l|
	if m=l.match(/\((.*?)\)\s+(\S+)\s+(\S+?):/)
	  slot=m[1]
	  id=m[2]
	  dev=m[3]
	  slot.strip!
	  cl=nil if l.match(/^\(\d+/) 
	  case dev
	  when '2409'
	    cl=id
	  when '18S20','18B20'
	    output f, [id,'DS1820',"TMP_#{no}","Temperature ##{no}",'°C']
	  when '2423'
	    output f, [id,'TAI8575',"CountA_#{no}","CounterA ##{no}",
	      'pulses',"CountB_#{no}","CounterB ##{no}",'pulses']
	  when '2438'
	    output f, [id,'DS2438',"VDD_#{no}", "VDD #{no}",'V',"TMP_#{no}",
	      "Temperature ##{no}",'°C']
	    output f, [id,'DS2438',"VAD_#{no}", "VAD #{no}",'V',"Vsens_#{no}",
	      "Vsens ##{no}",'mV']
	  when '2406'
	    if last=='2406'
	      last=nil
	    else
	      output f, [id,'TAI8570',"Pressure_#{no}", "Pressure #{no}",'hPa',
		"TMP_#{no}","Temperature ##{no}",'°C']
	    end
	  when '2450'
	    output f, [id,'TAI8515',"WDIR_#{no}","Wind Direction #{no}", '']
	  when '2760'
	    output f, [id,'DS2760', "MS_Volts_#{no}","Moisture Voltage #{no}",'V',
	      "MS_Current_#{no}","Moisture Current #{no}",'A']
	    output f, [id,'DS2760', "MS_Temp_#{no}","Moisture Temperature #{no}",
	      '°C',"MS_Accum_#{no}","Moisture Accumulator #{no}",'Ahrs']
	  when 'HB1WT'
	    output f, [id,'HB???',"HB?? #{no}","Hobbyboards with temperature #{no}", '']
	  else
	    STDERR.puts "?? #{l}"
	    typ = l.chomp.split(':')
	    output f, [id, dev,"UNK #{no}","UNKNOWN: #{typ[1]} #{no}", ''],'-- '
	  end
	  if(cl)
	    case slot
	    when /Main/
	      output f, [cl,'DS2409','MAIN',id]
	    when /Aux/
	      output f, [cl,'DS2409',nil,nil,nil,'AUX',id]
	    end
	  end
	  last=dev
	  no += 1
	end
      end
    end
  end
end

s=MakeSensors.new
s.buildconfig
