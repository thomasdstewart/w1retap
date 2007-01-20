CREATE TABLE ratelimit (
    name text,
    value double precision,
    rmax double precision,
    rmin double precision
);

CREATE TABLE readings (
    date integer,
    name text,
    value double precision
);

CREATE TABLE replog (
    date timestamp with time zone,
    message text
);

CREATE TABLE station (
    name text,
    stnlat double precision,
    stnlong double precision,
    altitude real,
    location text,
    software text,
    url text,
    wu_user text,
    wu_pass text,
    rfact double precision,
    cwop_user text,
    cwop_pass text
);

CREATE TABLE w1sensors (
    device text,
    type text,
    abbrv1 text,
    name1 text,
    units1 text,
    abbrv2 text,
    name2 text,
    units2 text,
    params text	
);

