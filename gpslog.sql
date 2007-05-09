--
-- (C)2007 Dustin Spicuzza
--
-- Create MySQL DB for gpslog
--
CREATE DATABASE gpslog;
USE gpslog;

CREATE TABLE points (
	id		BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
	data	TEXT NOT NULL
);

GRANT ALL ON gpslog.* TO 'gpslog'@'localhost' IDENTIFIED BY 'somepassword';



