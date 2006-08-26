
INSERT INTO station (name, stnlat, stnlong, altitude, location, software, url, wu_user, wu_pass, rfact, cwop_user, cwop_pass) VALUES ('Jonathan & Daria''s Weather Station', 50.915321, -1.53036, 19, 'Netley Marsh', 'w1retap', 'http://www.daria.co.uk/wx/', 'wuser', 'wpass', 0.0105, 'cwopuser', 'cwoppass');

INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('286DA467000000AD', 'DS1820', 'GHT', 'Greenhouse Temperature', '°C', NULL, NULL, NULL, NULL);
INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('10A942C10008009B', 'DS1820', 'OTMP0', 'Outside Temperatue', '°C', NULL, NULL, NULL, NULL);
INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('1093AEC100080042', 'DS1820', 'XTMP2', 'Garage Temperature', '°C', NULL, NULL, NULL, NULL);
INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('26378851000000AB', 'TAI8540', 'OHUM', 'Humidity', '%', 'OTMP2', 'Garage Temperature', '°C', NULL);
INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('12FC6B34000000A9', 'TAI8570', 'OPRS', 'Pressure', 'hPa', 'OTMP1', 'Temperature', '°C', NULL);
INSERT INTO w1sensors (device, type, abbrv1, name1, units1, abbrv2, name2, units2, params) VALUES ('1D9BB10500000089', 'TAI8575', 'RGC0', 'Counter0', ' tips', 'RGC1', 'Counter1', 'tips', NULL);

INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('GHT', 2.5, NULL, NULL);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('OTMP0', 2.5, NULL, NULL);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('OTMP1', 2.5, NULL, NULL);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('OPRS', 100, 800, 1200);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('RGC0', 50, NULL, NULL);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('RGC1', 50, NULL, NULL);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('OTMP2', 2.5, -10, 50);
INSERT INTO ratelimit (name, value, rmin, rmax) VALUES ('OHUM', 7, 0, 100.04);

