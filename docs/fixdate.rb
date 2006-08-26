#!/usr/bin/ruby
require 'dbi'

# Migrate w1retap readings.date from UNIX epoch to SQL timestamps.
# Once the migration is completed, delete readings and rename
# readings_t to readings and set timestamp = 1 in
# ~/.config/w1retap/rc.

dbnam = ARGV[0]
sqlfile = ARGV[1] || '/tmp/fdt.sql'
sql="select date,name,value from readings"

puts dbnam
db = DBI.connect(dbnam, '', '')

f = File.open(sqlfile,'w')
f.puts "drop table readings_t;"
f.puts "create table readings_t (date timestamp with time zone, name text, value real);"
f.puts "COPY readings_t (date, name, value) FROM stdin;"
s = db.execute sql
s.each do |r|
  t=Time.at(r[0])
  r[0]="'#{t.strftime("%F %T%z")}'"
  f.puts r.join("\t")
end
f.puts("\\.")
f.puts 'alter table readings rename to readings_epoch;'
f.puts 'alter table readings_t rename to readings;'
f.close
