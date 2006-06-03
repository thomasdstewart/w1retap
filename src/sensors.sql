-- Build the database (sqlite3 or pgsql)
-- (PostgreSQL database dump)

CREATE TABLE daily (
    udate integer,
    rain0 real,
    rain1 real
);


CREATE TABLE latest (
    udate integer,
    gstamp timestamp with time zone,
    ght real,
    gmax real,
    gmin real,
    temp real,
    pres real,
    humid real,
    rain real,
    dewpt real,
    dstamp timestamp with time zone,
    wspeed real,
    wdirn real,
    tide real
);


CREATE TABLE monthly (
    udate integer,
    rain0 real,
    rain1 real
);


CREATE TABLE readings (
    date integer,
    name text,
    value real
);

CREATE TABLE station (
    name text,
    stnlat real,
    stnlong real,
    altitude real,
    location text,
    software text,
    url text,
    wu_user text,
    wu_pass text,
    rfact real,
    cwop_user character varying,
    cwop_pass character varying
);


CREATE TABLE w1sensors (
    device text,
    type text,
    abbrv1 text,
    name1 text,
    units1 text,
    abbrv2 text,
    name2 text,
    units2 text
);


CREATE TABLE ratelimit (
    name text,
    value real,
    rmin real,
    rmax real
);

CREATE TABLE replog (
    date timestamp with time zone,
    message text
);


