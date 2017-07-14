
-- Below is to generate a list of line command to modify columns satisfying certain conditions:
-- select distinct concat('alter table ',
--        		       table_name,
-- 		       ' drop column ',
-- 		       '`',
-- 		       column_name,
-- 		       '` ;'
--                        )
		       
-- -- ' modify ',
-- -- column_name,
-- --' double ;'
-- from information_schema.COLUMNS
--  where table_schema = 'aidaTest'
--  and table_name = 'aidaSC'
--  and column_type = 'double';


-- show databases;
-- use aidaTest;
-- describe aidaSC;
--language sql
-- declare v_count int = 1;

/* 
* Prepare the database
*/
create database if not exists `aidaTest`;
use `aidaTest`;

drop table if exists `aidaSC`;
create table if not exists `aidaSC` (
`counter`    int unsigned zerofill  auto_increment,
`timer`      datetime    NOT NULL,
`ch1`	     double	, -- chid -> 0.id in AMR
`ch11`	     double	,
`ch21`	     double	,
`ch31`	     double	,
`ch40`	     double	,
`ch41`	     double	,
`ch42`	     double	,
`ch43`	     double	,
`ch44`	     double	,
`ch45`	     double	,
`ch46`	     double	,
`ch47`	     double	,
`ch48`	     double	,
`ch49`	     double	,
primary key (`counter`, `timer`)
);
describe aidaSC;

drop table if exists `aida_channels`;
create table if not exists `aida_channels` (
`chid`	     varchar(20) NOT NULL,
`id`	     varchar(20) NOT NULL,
`unit`	     varchar(20) NOT NULL,
`sensor`     varchar(20) NOT NULL,
`comment`    varchar(20) NOT NULL,
primary key(`chid`, `id`, `unit`, `sensor`, `comment`)
);

insert into `aida_channels` values
('ch1',  '0.1',  '\\u2103',   'DIGI', 'T,t'),
('ch11', '0.11', '%H',        'DIGI', 'RH,Uw'),
('ch21', '0.21', '\\u2103',   'DIGI', 'DT,td'),
('ch31', '0.31', 'mb',        'DIGI', 'AP,p mbar'),
('ch40', '0.40', '\\u2103',   'Ntc',  ''),
('ch41', '0.41', '\\u2103',   'Ntc',  ''),
('ch42', '0.42', '\\u2103',   'Ntc',  ''),
('ch43', '0.43', '\\u2103',   'Ntc',  ''),
('ch44', '0.44', '\\u2103',   'Ntc',  ''),
('ch45', '0.45', '\\u2103',   'Ntc',  ''),
('ch46', '0.46', '\\u2103',   'Ntc',  ''),
('ch47', '0.47', '\\u2103',   'Ntc',  ''),
('ch48', '0.48', '\\u2103',   'Ntc',  ''),
('ch49', '0.49', '\\u2103',   'Ntc',  '');
select * from aida_channels;
