#!/usr/bin/ruby
require 'dbi'

# Migrate w1retap database local pressures to MSL (for PgSQL)

G = 9.80665
R = 287.04

def pr_msl p0, temp, z
  kt = temp +  273.15
  x = (G * z / (R * kt))
  p0 * Math.exp(x)
end

###SDB='dbi:Pg:sensors'

dbnam = ARGV[0]

sql="select p.date,p.value,t.value from readings p,readings t where p.name='OPRS' and t.name='OTMP1' and p.date=t.date order by date asc"

puts dbnam
db = DBI.connect(dbnam, '', '')

f = File.open("/tmp/pres.sql",'w')
f.puts "delete from readings where name='OPRS' ;"
f.puts "COPY readings (date, name, value) FROM stdin;"
s = db.execute sql
s.each do |r|
  p = pr_msl r[1],r[2],19
  f.puts [r[0],'OPRS',p].join("\t")
end
f.puts("\\.")
f.close
