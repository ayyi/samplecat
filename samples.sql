-- MySQL dump 9.11
--
-- Host: localhost    Database: samplelib
-- ------------------------------------------------------
-- Server version	4.0.24

--
-- Table structure for table `samples`
--

CREATE TABLE samples (
  id int(11) NOT NULL auto_increment,
  filename text NOT NULL,
  filedir text,
  keywords varchar(60) default '',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Dumping data for table `samples`
--

INSERT INTO samples VALUES (1,'808 kik','/home/tim/','drums kik');

