insert into station values('Zlatograd.com',41.3833,25.1,440.0,
 'Zlatograd, Bulgaria', 'w1retap', 'http://zlatograd.com',NULL,NULL,
 0.0,NULL,NULL);

insert into w1sensors values('106B89C4000800B9','DS18S20',
	'DS1820 Temp','Temperature','°C',NULL,NULL,NULL,NULL);
insert into w1sensors values('264E1169000000B5','MPX4115A',
	'Baro Pres','Pressure','hPa','Baro Temp','Temperature','°C',
	'34.249672152 762.374681772');
insert into w1sensors values('01F8A3880E0000A2','SHT11',
	'SHT11 RH','Humidity','%','SHT11 Temp','Temperature','°C',NULL);
insert into w1sensors values('1FCD2D020000007F','Coupler',
	'MAIN','264E1169000000B5',NULL,'AUX','01F8A3880E0000A2',NULL,NULL);

