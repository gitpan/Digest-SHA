# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl 1.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use Test::More tests => 8;

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

use strict;
use integer;

use Digest::SHA qw(shaopen shawrite shafinish shahex shaclose shadup);

#	SHA-1 Test Vectors from Jim Gillogly (jim@acm.org)
#
#	In the following we use the notation bitstring#n to mean a bitstring
#	repeated n (in decimal) times, and we use | for concatenation.
#	Therefore 110#3|1 is 1101101101.
#
#	Here is a set near 2^32 bits to test the roll-over in the length
#	field from one to two 32-bit words:
#
#	110#1431655764|11 1eef5a18 969255a3 b1793a2a 955c7ec2 8cd221a5
#	110#1431655765|   7a1045b9 14672afa ce8d90e6 d19b3a6a da3cb879
#	110#1431655765|1  d5e09777 a94f1ea9 240874c4 8d9fecb6 b634256b
#	110#1431655765|11 eb256904 3c3014e5 1b2862ae 6eb5fb4e 0b851d99
#
#	011#1431655764|01 4CB0C4EF 69143D5B F34FC35F 1D4B19F6 ECCAE0F2
#	011#1431655765    47D92F91 1FC7BB74 DE00ADFC 4E981A81 05556D52
#	011#1431655765|0  A3D7438C 589B0B93 2AA91CC2 446F06DF 9ABC73F0
#	011#1431655765|01 3EEE3E1E 28DEDE2C A444D68D A5675B2F AAAB3203

my @vecs110 = (	# 110 rep 1431655764
	"11", "1eef5a18969255a3b1793a2a955c7ec28cd221a5",
	"110", "7a1045b914672aface8d90e6d19b3a6ada3cb879",
	"1101", "d5e09777a94f1ea9240874c48d9fecb6b634256b",
	"11011", "eb2569043c3014e51b2862ae6eb5fb4e0b851d99"
);

my @vecs011 = (	# 011 rep 1431655764
	"01", "4cb0c4ef69143d5bf34fc35f1d4b19f6eccae0f2",
	"011", "47d92f911fc7bb74de00adfc4e981a8105556d52",
	"0110", "a3d7438c589b0b932aa91cc2446f06df9abc73f0",
	"01101", "3eee3e1e28dede2ca444d68da5675b2faaab3203"
);

my $reps = 1 << 14;
my $loops = int(1431655764 / $reps);
my $rest = 3 * (1431655764 - $loops * $reps);

sub state110 {
	my $bitstr = pack("B*", "110" x $reps);
	my $state = shaopen(1);
	for (my $i = 0; $i < $loops; $i++) {
		shawrite($bitstr, 3 * $reps, $state);
	}
	shawrite($bitstr, $rest, $state);
	return($state);
}

sub state011 {
	my $bitstr = pack("B*", "011" x $reps);
	my $state = shaopen(1);
	for (my $i = 0; $i < $loops; $i++) {
		shawrite($bitstr, 3 * $reps, $state);
	}
	shawrite($bitstr, $rest, $state);
	return($state);
}

my $i;
my $state;

my $state110 = state110();
for ($i = 0; $i < @vecs110/2; $i++) {
	$state = shadup($state110);
	shawrite(pack("B*", $vecs110[2*$i]), length($vecs110[2*$i]), $state);
	shafinish($state);
	is(
		shahex($state),
		$vecs110[2*$i+1],
		'110 x 1431655764 . ' . $vecs110[2*$i]
	);
	shaclose($state);
}
shaclose($state110);

my $state011 = state011();
for ($i = 0; $i < @vecs011/2; $i++) {
	$state = shadup($state011);
	shawrite(pack("B*", $vecs011[2*$i]), length($vecs011[2*$i]), $state);
	shafinish($state);
	is(
		shahex($state),
		$vecs011[2*$i+1],
		'011 x 1431655764 . ' . $vecs011[2*$i]
	);
	shaclose($state);
}
shaclose($state011);
