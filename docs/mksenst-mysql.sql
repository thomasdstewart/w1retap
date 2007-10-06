CREATE TABLE ratelimit (
    name text,
    value real,
    rmax real,
    rmin real
);


CREATE TABLE readings (
    date timestamp,
    name text,
    value real
);


CREATE TABLE replog (
    date timestamp,
    message text
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
