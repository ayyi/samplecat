-- MySQL dump 9.11
--
-- Host: localhost    Database: samplelib
-- ------------------------------------------------------
-- Server version	4.0.24

--
-- Table structure for table `samples`
--
	CREATE database samplecat;
	USE samplecat;

	CREATE TABLE `samples` (
	  id int(11) NOT NULL auto_increment,
	  `filename` text NOT NULL,
	  `filedir` text NOT NULL,
	  `keywords` varchar(60) default '',
	  `pixbuf` blob,
	  `length` int(22) default NULL,
	  `sample_rate` int(11) default NULL,
	  `channels` int(4) default NULL,
	  `online` int(1) default NULL,
	  `last_checked` datetime default NULL,
	  `mimetype` tinytext,
	  `notes` mediumtext,
	  `colour` tinyint(4) default NULL,
	  `peaklevel` float default NULL,
	  `ebur` text default NULL,
	  `full_path` text NOT NULL,
	  `mtime` int(22) default NULL,
	  `frames` int(22) default NULL,
	  `bit_rate` int(11) default NULL,
	  `bit_depth` int(8) default NULL,
	  `meta_data` text default NULL,
	  PRIMARY KEY  (`id`)
	)
