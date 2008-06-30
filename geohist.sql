--
-- (C)2007 Dustin Spicuzza
--
-- Create MySQL DB for geohist. Run this with
--
--		mysql [params] < geohist.sql
--

CREATE DATABASE geohist;
USE geohist;

CREATE TABLE points (
	id			BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
	track_id	INTEGER UNSIGNED NOT NULL,
	time		DOUBLE NOT NULL,
	latitude	DOUBLE NOT NULL,
	longitude	DOUBLE NOT NULL,
	course		DOUBLE NOT NULL,
	speed		DOUBLE NOT NULL,
	climb		DOUBLE NOT NULL,
	altitude	DOUBLE NOT NULL,
	INDEX (track_id),
	INDEX (time)
);

GRANT ALL ON geohist.* TO 'geohist'@'localhost' IDENTIFIED BY 'somepassword';



