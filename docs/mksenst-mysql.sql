drop table if exists ratelimit;
CREATE TABLE ratelimit (
    name text,
    value real,
    rmax real,
    rmin real
);

drop table if exists readings;
CREATE TABLE readings (
    date timestamp,
    name text,
    value real,
    id integer auto_increment primary key
);

drop table if exists replog;
CREATE TABLE replog (
    id integer auto_increment primary key,
    date timestamp,
    message text
);

drop table if exists station;
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
    cwop_user text,
    cwop_pass text,
    lcdon integer, 
    lcdoff integer
);

drop table if exists w1sensors;
CREATE TABLE w1sensors (
    id integer auto_increment primary key,
    device text,
    type text,
    abbrv1 text,
    name1 text,
    units1 text,
    abbrv2 text,
    name2 text,	
    units2 text,	
    params text,	
    `interval` integer	
);
