--
-- (C)2007 Dustin Spicuzza
--
-- Create MySQL DB for geohist. Run this with
--
--		mysql [params] < geohist.sql
--

CREATE DATABASE geohist;
USE geohist;

CREATE TABLE `points` (
  `id` 			bigint(20) unsigned NOT NULL auto_increment,
  `track_id` 	int(10) unsigned NOT NULL,
  `time` 		DOUBLE NOT NULL,
  `latitude` 	DOUBLE NOT NULL,
  `longitude` 	DOUBLE NOT NULL,
  `course` 		DOUBLE NOT NULL,
  `speed` 		DOUBLE NOT NULL,
  `climb` 		DOUBLE NOT NULL,
  `altitude` 	DOUBLE NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `track_id` (`track_id`),
  KEY `time` (`time`)
);

GRANT ALL ON geohist.* TO 'geohist'@'localhost' IDENTIFIED BY 'somepassword';



