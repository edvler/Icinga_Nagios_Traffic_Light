#!/usr/bin/perl

use IO::Socket::INET;
use DBI;
use strict;

my $SHOW_NAMES = 1;


my $database="icinga";
my $username="root";
my $password="iamroot4MySQLNew";

my $table_hosts="icinga_hosts";
my $table_hoststatus="icinga_hoststatus";
my $table_services="icinga_services";
my $table_servicestatus="icinga_servicestatus";

my $nr_services_traffic_light="1";
my $nr_hosts_traffic_light="2";

#####
my ($sock,$data);

$sock = new IO::Socket::INET (
	PeerAddr   => '172.31.0.177:54000',
	Proto        => 'udp'
) or die "ERROR creating Socket: $!\n";

if($ARGV[0] != "") {
	$sock->send($ARGV[0]);
	$sock->flush();
	$sock->close();
	exit 0;
}

my $dbh = "";
my $sql = "";
my $sth = "";

my @ergebnis = "";

my $count_hosts_down = 0;
my $count_services_critical = 0;
my $count_services_warning = 0;

$dbh = DBI->connect( "DBI:mysql:$database",$username,$password) || die "Database connection failed: $DBI::errstr";

##hosts
$sql = qq{select count(*) as c from $table_hoststatus inner join $table_hosts on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where current_state>0 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
	$count_hosts_down = $ergebnis[0];
}


$sql = qq{select display_name from $table_hoststatus inner join $table_hosts on ($table_hosts.host_object_id = $table_hoststatus.host_object_id) where current_state>0 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

if ($SHOW_NAMES == 1) {
	print "$count_hosts_down Hosts marked as down and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " \n";
	}
}
$sth->finish();

##services critical
$sql = qq{select count(*) from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) where current_state=2 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
  $count_services_critical = $ergebnis[0];
}
$sth->finish();

$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) where current_state=2 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

if ($SHOW_NAMES == 1) {
	print "$count_services_critical services marked as critical and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
}
$sth->finish();

##services warning
$sql = qq{select count(*) from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) where current_state=1 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

while(@ergebnis=$sth->fetchrow_array)
{
  $count_services_warning = $ergebnis[0];
}
$sth->finish();

$sql = qq{select $table_services.display_name as sd, $table_hosts.display_name as hd from $table_servicestatus inner join $table_services on ($table_services.service_object_id = $table_servicestatus.service_object_id) inner join $table_hosts on( $table_hosts.host_object_id=$table_services.host_object_id) where current_state=1 and  problem_has_been_acknowledged=0;};
$sth = $dbh->prepare( $sql );
$sth->execute();

if ($SHOW_NAMES == 1) {
	print "$count_services_warning services marked as warning and not acknowledged:\n";
	while(@ergebnis=$sth->fetchrow_array)
	{
	  print " - " . $ergebnis[0] . " - " . $ergebnis[1] . " \n";
	}
}
$sth->finish();


$dbh->disconnect();


#$count_services_critical=0;

if ($count_services_critical > 0) {
	redOn($nr_services_traffic_light);
}

if ($count_services_warning > 0 and $count_services_critical==0) {
	yellowOn($nr_services_traffic_light);
}

if ($count_services_warning == 0 and $count_services_critical==0) {
	greenOn($nr_services_traffic_light);
}



if ($count_hosts_down == 0) {
	greenOn($nr_hosts_traffic_light);
}

if ($count_hosts_down > 0) {
	redOn($nr_hosts_traffic_light);
}

$sock->close();





sub redOn() {
	my $traffic_light_nr = shift;

	$sock->send($traffic_light_nr . " redOn");
	$sock->flush();
	$sock->send($traffic_light_nr . " yellowOff");
	$sock->flush();
	$sock->send($traffic_light_nr . " greenOff");
	$sock->flush();
}


sub yellowOn() {
	my $traffic_light_nr = shift;

	$sock->send($nr_services_traffic_light . " yellowOn");
	$sock->flush();
	$sock->send($nr_services_traffic_light . " redOff");
	$sock->flush();
	$sock->send($nr_services_traffic_light . " greenOff");
	$sock->flush();
}


sub greenOn() {
	my $traffic_light_nr = shift;

	$sock->send($nr_services_traffic_light . " greenOn");
	$sock->flush();
	$sock->send($nr_services_traffic_light . " redOff");
	$sock->flush();
	$sock->send($nr_services_traffic_light . " yellowOff");
	$sock->flush();
}






