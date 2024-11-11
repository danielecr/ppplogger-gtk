#!/usr/bin/perl

my $message="messages.5";


open(LOGFILE, $message) or die "I could not open $message: $!";

#id_inizio
#DEVICE
#IFNAME
#IPLOCAL
#IPREMOTE
#PEERNAME
#SPEED
#ORIG_UID
#PPPLOGNAME
#inizio
#PPPD_PID
#id_fine

#id_fine
#CONNECT_TIME
#BYTES_SENT
#BYTES_RCVD
#SPEED
#ORIG_UID
#PPPLOGNAME
#CALL_FILE
#DNS1
#DNS2
#fine
#PPPD_PID
#id_inizio

%month = ("Jan"=>1,"Apr"=>4,"May"=>5,"Jun"=>6,"Jul"=>7,"Dec"=>12);

$prog = 0;

my %connections;

while($line = <LOGFILE>) {
    if ($line =~ /pppd\[(.*)\]/ ) {
	my $PPP_PID = $1;
	my $condata;
	if ($condata = $connections{"$PPP_PID"}) {
	    # nil
	} else {
	    my %dhash = ("PPPD_PID"=>$PPP_PID);
	    #my $href = \%dhash;
	    $connections{"$PPP_PID"} = \%dhash;
	    $condata = $connections{"$PPP_PID"};
	}
	# well now I "parse" the line
	my $timestamp;
	if ($line =~ /^(.{15}).*$/) {
	    $timestamp = $1;
	    $$condata{'timestamp'} =$1;
	}
	if ($line =~ /^(.{15}).*Connect:\ (.*)\ <-->\ (.*)$/) {
	    print "timest: " . $1 . " ". $2 . " e " . $3 . "\n";
	    $$condata{'DEVICE'} = $3;
	    $$condata{'IFNAME'} = $2;
	    if ($timestamp =~ /(.{3})[^0-9]*([0-9]{1,2})[^0-9]*([0-9]{1,2}):([0-9]{1,2}):([0-9]{1,2})/) {
		$$condata{'START_TIME'} = '2005-'.$month{$1} ."-$2 $3:$4:$5";
	    }
	} elsif ($line =~ /started\ by\ ([^,]*),\ uid\ (.*)/ ) {
	    $$condata{'PPPLOGNAME'} = $1;
	    $$condata{'ORIG_UID'} = $2;
	    print $1 . " e " . $2 . "\n";
	} elsif ($line =~ /Connect\ time\ ([0-9]*)\.([0-9]*)\ minutes./ ) {
	    my $minute = $1;
	    my $second = $2;
	    print "time: " . $1 .":" . ($2 / 10) *60 ."\n";
	    $$condata{'CONNECT_TIME'} = $1 * 60 + ($2 / 10) *60;
	} elsif ($line =~ /ocal.*IP.*address[^0-9]*([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})/ ) {
	    $$condata{'IPLOCAL'} = $1;
	} elsif ($line =~ /remote.*IP.*address[^0-9]*([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})/ ) {
	    $$condata{'IPREMOTE'} = $1;
	} elsif ($line =~ /Sent\ (.*)\ bytes,\ received\ (.*)\ bytes./ ) {
	    my $sum = $1 + $2;
	    print "sent" . $1 . "received" . $2 . "sum= " . $sum . "\n";
	    $$condata{'BYTES_SENT'} = $1;
	    $$condata{'BYTES_RCVD'} = $2;
	    if ($timestamp =~ /(.{3})[^0-9]*([0-9]{1,2})[^0-9]*([0-9]{1,2}):([0-9]{1,2}):([0-9]{1,2})/) {
		$$condata{'END_TIME'} = '2005-'.$month{$1} ."-$2 $3:$4:$5";
	    }
	    # chiudi connessione
	    #$connections{$PPP_PID . "_$prog"} = $connections{$PPPD_PID};
	    $connections{$PPP_PID . "_$prog"} = $condata;
	    $prog++;
	}
    }
}

eval {

use DBI;

$host_name = "localhost";
$db_name = "pppdconnection";
$db_user = "pppd";
$db_pwd = "pppd";

my $dsn = "DBI:mysql:host=$host_name;database=$db_name";

my $dbh;
$dbh = DBI->connect_cached ($dsn, $db_user, $db_pwd,
			    {PrintError => 0, RaiseError => 1});
#$dbh = WebDB::connect_cached();
#&try_connect();

use Term::ANSIColor;

$connections{"a"}= "b";
print "AAAAOOOOO\n";
for $nome (keys %connections) {
    if ($nome =~ /[0-9]*_[0-9]*/ ) {
	print color 'bold';
	print "$nome\n";
	print color 'reset';
	$con = $connections{$nome};
	for $k (keys % $con) {
	    print color 'blue';
	    print "$k ";
	    print color 'reset';
	    print $$con{$k} . "\n";
	}
	$qry_start = "INSERT INTO start_connect(DEVICE,IFNAME,IPLOCAL,IPREMOTE,ORIG_UID,PPPLOGNAME,PPPD_PID,inizio) " .
	    "VALUES('".$$con{'DEVICE'} . "','" .
	    $$con{'IFNAME'} . "','" .
	    $$con{'IPLOCAL'} . "','" .
	    $$con{'IPREMOTE'} . "','" .
	    $$con{'ORIG_UID'} . "','" .
	    $$con{'PPPLOGNAME'} . "','" .
	    $$con{'PPPD_PID'} . "','" .
	    $$con{'START_TIME'} . "')"
	    ;
	$dbh->do($qry_start);
	$id_inizio = $dbh->last_insert_id(undef,undef,undef,undef);
	$qry_end = "INSERT INTO end_connect(CONNECT_TIME,BYTES_SENT,BYTES_RCVD,ORIG_UID,PPPLOGNAME,PPPD_PID,fine,id_inizio) ".
	    "VALUES('".$$con{'CONNECT_TIME'} . "','" .
	    $$con{'BYTES_SENT'} . "','" .
	    $$con{'BYTES_RCVD'} . "','" .
	    $$con{'ORIG_UID'} . "','" .
	    $$con{'PPPLOGNAME'} . "','" .
	    $$con{'PPPD_PID'} . "','" .
	    $$con{'START_TIME'} . "'," .
	    $id_inizio . ")"
	    ;
	$dbh->do($qry_end);
	$id_fine = $dbh->last_insert_id(undef,undef,undef,undef);
	$qry_update = "UPDATE start_connect SET id_fine=$id_fine WHERE id_inizio = $id_inizio";
	$dbh->do($qry_update,undef,undef);
	#exit(0);
    }
    #print $connections{$nome}{'timestamp'};
    #print " E IL PID: " .$connections{$nome}{'PPPD_PID'};
    #print $nome ${$connections{$nome}}{'timestamp'} . "\n";
}
print "AAAAOOOOO\n";
}; if ($@) {
    #&logga("ERR: connessione DB -> " . $@);
    die("ERR: connessione DB -> " . $@);
}

close LOGFILE;


exit();


