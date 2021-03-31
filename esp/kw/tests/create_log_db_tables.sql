use log_db;

CREATE TABLE `transaction_ids` (
  `TableName` varchar(64) NOT NULL DEFAULT '',
  `NextID` int(11) NOT NULL,
  PRIMARY KEY (`TableName`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

insert into transaction_ids (tablename, NextID) values ('loggroup1', 101);
insert into transaction_ids (tablename, NextID) values ('loggroup2', 101);
insert into transaction_ids (tablename, NextID) values ('service_log', 101);

DELIMITER $$
CREATE DEFINER=`root`@`localhost` FUNCTION `get_transaction_id`(i_table_name VARCHAR(64)) RETURNS int(11)
    READS SQL DATA
    DETERMINISTIC
BEGIN
    UPDATE transaction_ids SET nextid = LAST_INSERT_ID(nextid + 1) 
        WHERE tablename = i_table_name;
    SET @next_id= LAST_INSERT_ID();   
    RETURN @next_id;
END$$
DELIMITER ;

CREATE TABLE `service_log` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `log_id` varchar(128) NOT NULL DEFAULT '',
  `transaction_id` varchar(128) DEFAULT NULL,
  `user_id` varchar(60) DEFAULT NULL,
  `user_ip_address` varchar(30) DEFAULT NULL,
  `request_user_id` varchar(60) DEFAULT NULL,
  `target_url` varchar(120) DEFAULT NULL,
  `request` text,
  `actual_response` text,
  `dateadded` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=57143 DEFAULT CHARSET=utf8;

CREATE TABLE `service_log_1` (
  `id` mediumint(9) NOT NULL AUTO_INCREMENT,
  `log_id` varchar(128) NOT NULL DEFAULT '',
  `transaction_id` varchar(128) DEFAULT NULL,
  `test_default` varchar(60) DEFAULT NULL,
  `user_ip_address` varchar(30) DEFAULT NULL,
  `request` text,
  `raw_response` text,
  `dateadded` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=15446 DEFAULT CHARSET=utf8;