#!/usr/bin/perl

use IO::Socket::INET;
use DBI;
use strict;

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


# The IP address and port according to the file Icinga_Nagios_Traffic_Light.ino.
# To use the Script with command-line parameters, this is all you have to configure.
my $arduino_ip="172.31.0.177";
my $arduino_port="54000";


############ Only edit if you want to select the actual state of icinga or nagios from the mysql database
# show names of warning and critial services and hosts on the console (produces output, dont set to 1 if this script is called from crontab)
# 0 = off, 1 = on
my $SHOW_NAMES = 1;

# mysql parameters
my $platform = "mysql";
my $database="icinga";
my $host = "127.0.0.1";
my $port = "3306";
my $username="root";
my $password="iamroot4MySQLNew";

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

# create the udp socket
$sock = new IO::Socket::INET (
	PeerAddr   => "$arduino_ip:$arduino_port",
	Proto        => 'udp'
) or die "ERROR creating Socket: $!\n";

# if there are arguments send it to the arduino and quit
if(!($ARGV[0] eq "")) {
	$sock->send($ARGV[0]);
	$sock->flush();
	$sock->close();
	exit 0;
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

# select to find all hosts wich are down
$sql = qq{select count(*) as c from $table_hoststatus inner join $table_hosts on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where current_state>0 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
	$count_hosts_down = $ergebnis[0];
}

# if the names of the hosts should be displayed select it and print it
if ($SHOW_NAMES == 1) {
	$sql = qq{select display_name from $table_hoststatus inner join $table_hosts on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where current_state>0 and  problem_has_been_acknowledged=0;};
	$sth = $dbh->prepare( $sql );
	$sth->execute();

	print "$count_hosts_down Hosts marked as down and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " \n";
	}
	$sth->finish();
}

# services critical
$sql = qq{select count(*) from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) where current_state=2 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
  $count_services_critical = $ergebnis[0];
}
$sth->finish();

# if the service names critial should be displayed select it and print it
if ($SHOW_NAMES == 1) {
	$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) where current_state=2 and  problem_has_been_acknowledged=0;};
	$sth = $dbh->prepare( $sql );
	$sth->execute();

	print "$count_services_critical services marked as critical and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
	$sth->finish();
}

# services warning
$sql = qq{select count(*) from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) where current_state=1 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
  $count_services_warning = $ergebnis[0];
}
$sth->finish();

# if the service names warning should be displayed select it and print it
if ($SHOW_NAMES == 1) {
	$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) where current_state=1 and  problem_has_been_acknowledged=0;};
	$sth = $dbh->prepare( $sql );
	$sth->execute();

	print "$count_services_warning services marked as warning and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
$sth->finish();
}

# disconnect from database
$dbh->disconnect();


$count_services_critical=0;

# turn the correspondig lights on or off for the services
if ($count_services_critical > 0) {
	redOn($nr_services_traffic_light);
}

if ($count_services_warning > 0 and $count_services_critical==0) {
	yellowOn($nr_services_traffic_light);
}

if ($count_services_warning == 0 and $count_services_critical==0) {
	greenOn($nr_services_traffic_light);
}


# turn the correspondig lights on or off for the hosts
if ($count_hosts_down == 0) {
	greenOn($nr_hosts_traffic_light);
}

if ($count_hosts_down > 0) {
	redOn($nr_hosts_traffic_light);
}

$sock->close();




# sub to turn only the red sign on on the given traffic light id
sub redOn() {
	my $traffic_light_nr = shift;

	$sock->send($traffic_light_nr . "redOn");
	$sock->flush();
	sleep(0.5);
	$sock->send($traffic_light_nr . "yellowOff");
	$sock->flush();
	sleep(0.5);
	$sock->send($traffic_light_nr . "greenOff");
	$sock->flush();
	sleep(0.5);
}

# sub to turn only the yellow sign on on the given traffic light id
sub yellowOn() {
	my $traffic_light_nr = shift;

	$sock->send($nr_services_traffic_light . "yellowOn");
	$sock->flush();
	sleep(0.5);
	$sock->send($nr_services_traffic_light . "redOff");
	$sock->flush();
	sleep(0.5);
	$sock->send($nr_services_traffic_light . "greenOff");
	$sock->flush();
	sleep(0.5);
}

# sub to turn only the green sign on on the given traffic light id
sub greenOn() {
	my $traffic_light_nr = shift;

	$sock->send($nr_services_traffic_light . "greenOn");
	$sock->flush();
	sleep(0.5);
	$sock->send($nr_services_traffic_light . "redOff");
	$sock->flush();
	sleep(0.5);
	$sock->send($nr_services_traffic_light . "yellowOff");
	$sock->flush();
	sleep(0.5);
}






