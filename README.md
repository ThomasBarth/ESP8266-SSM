ESP8266-SSM
===========

Simple Smart Meter based on the ESP8266 SDK example.
measures how long a pin is pulled down and send this information to a PHP script which is saving the data into a SQL database.
There is also a frontend based on visualization.

1.
flash bins to the corresponding adresses
start the chip, you might want to test AT+GMR, it should tell you that it is the DataLogger.
Also you now have a AT+HELP command

2.
create SQL database table
CREATE TABLE `heating` (
    `moment`          DATETIME NOT NULL ,
    `duration`        DOUBLE   NOT NULL ,
    `outside_temp`    DOUBLE   NOT NULL ,
    `wind`            DOUBLE   NOT NULL
) ;

3.
upload content of "www" to a webserver, make sure you edit those files to have your SQL credentials etc in there.

4.
activate system

AT+CSTARTLOG=[<conid>],"<TCP/UDP>","<YOURSERVERANDPATH>",<port>


example

AT+CSTARTLOG=2,"TCP","www.whatever.com/SmartMeter/new_value.php?o=Tokio",80

