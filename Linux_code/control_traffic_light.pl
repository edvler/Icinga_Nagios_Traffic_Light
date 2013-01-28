#!/usr/bin/perl

use IO::Socket::INET;

my ($sock,$data);

$sock = new IO::Socket::INET (
PeerAddr   => '172.31.0.177:54000',
Proto        => 'udp'
) or die "ERROR creating Socket: $!\n";

$data = "2 greenOff";
$sock->send($data);
$sock->flush();
$sock->close();

