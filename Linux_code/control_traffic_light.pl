#!/usr/bin/perl

use IO::Socket::INET;
use DBI;
use strict;

#First, look at Icinga_Nagios_Traffic_Light.pde!!

#There are 2 modes avaliable. You have to enable the mode in the arduino source code too!
# 1. Send commands per UDP. It's possible to deaktivate the UDP mode in the Icinga_Nagios_Traffic_Light.pde for Arduino.
# 2. Send commands per Web/HTTP

#Please set at least one option to "yes"!
#See variables $USE_WEBSERVER and $USE_UDP

# Mode
# Choose only one option!
# If you uncomment Webservermode then the commands are sent be a web request
my $USE_WEBSERVER="no";
# If you uncomment UDP mode then the commands are sent by UDP
my $USE_UDP="yes";


# Please set the IP address and port according your settings int the webinterface of the arduino.
my $arduino_ip="192.168.0.111";
my $arduino_port="54000";


#Possible commands to send per Ethernet UDP:
#1redOn - Turn on the red sign of traffic light ONE
#2yellowOn - Turn on the yellow sign of traffic light TWO
#3greenOn - Turn on the green sign of traffic light THREE
#...

#1redOff - Turn off the red sign of traffic light ONE
#2yellowOff - Turn off the yellow sign of traffic light TWO
#...

#allOn - Turn on all signs
#allOff - Turn off all signs

#commands are case-sensitive!
#example: ./control_traffic_light.pl 1redOff


#How commands per Web work:
#Open your Browser and type one of the following URL's
#http://<ARDUINO_IP>/tl.html?tl2=greenOn - Turn on the green sign of traffic light TWO
#http://<ARDUINO_IP>/tl.html?tl2=greenOff - Turn off the green sign of traffic light TWO

#http://<ARDUINO_IP>/tl.html?tl1=redOn - Turn on the red sign of traffic light ONE
#http://<ARDUINO_IP>/tl.html?tl1=redOff - Turn off the red sign of traffic light ONE

#http://<ARDUINO_IP>/tl.html?tl1=yellowOn - Turn on the yellow sign of traffic light ONE
#http://<ARDUINO_IP>/tl.html?tl3=yellowOn - Turn on the yellow sign of traffic light THREE
#....

#you could use "wget --spider http://<ARDUINO_IP>/tl.html?tl3=yellowOn" on your linux console
#Please replace <ARDUINO_IP> with the IP of your Arduino


############ Only edit below if you want to select the actual state of icinga or nagios from the mysql database

# show names of warning and critial services and hosts on the console (produces output, dont set to 1 if this script is called from crontab)
# 0 = off, 1 = on
my $SHOW_NAMES = 1;

# mysql parameters
my $platform = "mysql";
my $database="icinga";
my $host = "127.0.0.1";
my $port = "3306";
my $username="root";
my $password="changeMe!";

# table names for the nagios or icinga installation
# for icinga the table prefix is icinga
# and for nagios the table prefix is nagios
# comment out the suitable

#my $system="nagios";
my $system="icinga";

my $table_hosts=$system . "_hosts";
my $table_hoststatus=$system . "_hoststatus";
my $table_services=$system . "_services";
my $table_servicestatus=$system . "_servicestatus";

# on which traffic light should be hosts and services displayed
my $nr_services_traffic_light="1";
my $nr_hosts_traffic_light="2";

# Dont change anything below this
###########################################################################################
my ($sock,$data);

if (not($USE_UDP  eq "yes") && not($USE_WEBSERVER eq "yes")) {
	die "Please set $USE_UDP or $USE_WEBSERVER to true!\n";
}

# create the udp socket only if UDP is used
if ($USE_UDP eq "yes") {
	$sock = new IO::Socket::INET (
	PeerAddr => "$arduino_ip:$arduino_port",
	Proto => 'udp'
	) or die "ERROR creating Socket: $!\n";

	# if there are arguments given to the script send it to the arduino and quit
	if(!($ARGV[0] eq "")) {
	$sock->send($ARGV[0]);
	$sock->flush();
	$sock->close();
	sleep(5);
	exit 0;
	}
}


# some sql variables
my $dbh = "";
my $sql = "";
my $sth = "";
my @ergebnis = "";

# variables to store the counts of the sql queries
my $count_hosts_down = 0;
my $count_services_critical = 0;
my $count_services_warning = 0;

# connect to database with the given parameters
$dbh = DBI->connect( "DBI:$platform:$database:$host:$port",$username,$password) || die "Database connection failed: $DBI::errstr";

# if the names of the hosts should be displayed select it and print it
$sql = qq{select display_name from $table_hoststatus inner join $table_hosts on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where current_state>0 and problem_has_been_acknowledged=0  and $table_hoststatus.notifications_enabled=1;};
$sth = $dbh->prepare( $sql );
$sth->execute();

print "Hosts marked as red:\n";
while(@ergebnis=$sth->fetchrow_array)
{
	$count_hosts_down++;
	if ($SHOW_NAMES == 1) {
		print " - " . $ergebnis[0] . " \n";
	}
}
$sth->finish();

# if the service names critial should be displayed select it and print it
$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) inner join $table_hoststatus  on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where $table_servicestatus.current_state=2 and $table_servicestatus.problem_has_been_acknowledged=0 and  $table_servicestatus.notifications_enabled=1 and $table_servicestatus.state_type=1 and $table_hoststatus.current_state=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

print "services marked as red:\n";
while(@ergebnis=$sth->fetchrow_array)
{
	$count_services_critical++;
	if ($SHOW_NAMES == 1) {
		print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
}
$sth->finish();


# if the service names warning should be displayed select it and print it
$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) inner join $table_hoststatus  on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where $table_servicestatus.current_state=1 and $table_servicestatus.problem_has_been_acknowledged=0 and  $table_servicestatus.notifications_enabled=1 and $table_servicestatus.state_type=1 and $table_hoststatus.current_state=0;};
#$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) where current_state=1 and problem_has_been_acknowledged=0  and $table_servicestatus.notifications_enabled=1 and $table_servicestatus.state_type=1;};
$sth = $dbh->prepare( $sql );
$sth->execute();

print "services marked as yellow:\n";
while(@ergebnis=$sth->fetchrow_array)
{
	$count_services_warning++;
	if ($SHOW_NAMES == 1) {
		print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
}
$sth->finish();

# disconnect from database
$dbh->disconnect();

print "\n";

# turn the correspondig lights on or off for the services
if ($count_services_critical > 0) {
	print "call redOn for services\n";
	redOn($nr_services_traffic_light);
}

if ($count_services_warning > 0 and $count_services_critical==0) {
	print "call yellowOn for services\n";
	yellowOn($nr_services_traffic_light);
}

if ($count_services_warning == 0 and $count_services_critical==0) {
	print "call greenOn for services\n";
	greenOn($nr_services_traffic_light);
}


# turn the correspondig lights on or off for the hosts
if ($count_hosts_down == 0) {
	print "call greenOn for hosts\n";
	greenOn($nr_hosts_traffic_light);
}

if ($count_hosts_down > 0) {
	print "call redOn for hosts\n";
	redOn($nr_hosts_traffic_light);
}

if ($USE_UDP eq "yes") {
	$sock->close();
}



# sub to turn only the red sign on on the given traffic light id
sub redOn() {
	my $traffic_light_nr = shift;

	if ($USE_WEBSERVER eq "yes") {
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=yellowOff");
		sleep(0.1);
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=greenOff");
		sleep(0.1);
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=redOn");
		sleep(0.1);
	}
	if ($USE_UDP eq "yes") {
		$sock->send($traffic_light_nr . "yellowOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "greenOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "redOn");
		$sock->flush();
		sleep(0.1);
	}
}

# sub to turn only the yellow sign on on the given traffic light id
sub yellowOn() {
	my $traffic_light_nr = shift;

	if ($USE_WEBSERVER eq "yes") {
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=redOff");
		sleep(0.1);
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=greenOff");
		sleep(0.1);
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=yellowOn");
		sleep(0.1);
	}
	if ($USE_UDP  eq "yes") {
		$sock->send($traffic_light_nr . "redOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "greenOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "yellowOn");
		$sock->flush();
		sleep(0.1);
	}
}

# sub to turn only the green sign on on the given traffic light id
sub greenOn() {
my $traffic_light_nr = shift;

	if ($USE_WEBSERVER eq "yes") {
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=redOff");
		sleep(0.1);
		system("wget --spider  http://$arduino_ip/tl.html?tl$traffic_light_nr=yellowOff");
		sleep(0.1);
		system("wget --spider http://$arduino_ip/tl.html?tl$traffic_light_nr=greenOn");
		sleep(0.1);
	} 
	if ($USE_UDP eq "yes") {
		$sock->send($traffic_light_nr . "redOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "yellowOff");
		$sock->flush();
		sleep(0.1);
		$sock->send($traffic_light_nr . "greenOn");
		$sock->flush();
		sleep(0.1);
	}
}


