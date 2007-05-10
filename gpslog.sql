--
-- (C)2007 Dustin Spicuzza
--
-- Create MySQL DB for gpslog. Run this with
--
--		mysql [params] < gpslog.sql
--

CREATE DATABASE gpslog;
USE gpslog;

CREATE TABLE points (
	id			BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
	time		DOUBLE NOT NULL,
	latitude	DOUBLE NOT NULL,
	longitude	DOUBLE NOT NULL,
	course		DOUBLE NOT NULL,
	speed		DOUBLE NOT NULL,
	climb		DOUBLE NOT NULL,
	altitude	DOUBLE NOT NULL
);

GRANT ALL ON gpslog.* TO 'gpslog'@'localhost' IDENTIFIED BY 'somepassword';



