-- MySQL dump 9.11
--
-- Host: localhost    Database: samplelib
-- ------------------------------------------------------
-- Server version	4.0.24

--
-- Table structure for table `samples`
--
	CREATE database samples;
	USE samples;

	CREATE TABLE `samples` (
	  id int(11) NOT NULL auto_increment,
	  `filename` text NOT NULL,
	  `filedir` text,
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
	  PRIMARY KEY  (`id`)
	)
