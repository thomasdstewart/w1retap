-- Build the database (sqlite3 or pgsql)

CREATE TABLE latest
(
	udate int,
	gstamp timestamp,
	ght real,
	gmax real,
	gmin real,
	temp real,
	pres real,
	humid int,
	rain real,
	dewpt real,
	dstamp timestamp,
	wspeed real,
	wdirn  real,
	tide   real
);

CREATE TABLE readings 
(
	date int, 
	name text, 
	value real
);

CREATE TABLE station
(
	name text,
	stnlat real,
	stnlong real,
	altitude real,
	location text,
	software text,
	url text,
	wu_user text,
	wu_pass text,
	rfact real
);

CREATE TABLE w1sensors
(
	device text,
	type text,
	abbrv1 text,
	name1 text,
	abbrv2 text,
	name2 text
);


-- INSERT INTO "station" VALUES('Jonathan & Daria''s Weather Station', 50.91532, -1.53036, 19, 'Netley Marsh', 'w1retap', 'http://www.daria.co.uk/wx/', 'fakename', 'hackmeharder', 0.0105);
-- INSERT INTO "w1sensors" VALUES('286DA467000000AD', 'Temperature', 'GHT', 'Greenhouse Temperature', NULL, NULL);
-- INSERT INTO "w1sensors" VALUES('2692354D00000095', 'Humidity', 'OHUM', 'Humidity', 'OTMP0', 'Temperature');
-- INSERT INTO "w1sensors" VALUES('12FC6B34000000A9', 'Pressure', 'OPRS', 'Pressure', 'OTMP1', 'Temperature');
-- INSERT INTO "w1sensors" VALUES('1D9BB10500000089', 'RainGauge', 'RGC0', 'Counter0', 'RGC1', 'Counter1');
