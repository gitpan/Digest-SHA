use Test::More qw(no_plan);
use strict;
use integer;
use Digest::SHA qw(sha384_hex sha512_hex);
use File::Basename qw(dirname);
use File::Spec;

my @sharsp = (
	"34aa973cd4c4daa4f61eeb2bdbad27316534016f",
	"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
	"9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b07b8b3dc38ecc4ebae97ddd87f3d8985",
	"e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b"
);

my @name = (
	'shadump/shaload: SHA-1("a" x 1000000)',
	'shadump/shaload: SHA-256("a" x 1000000)',
	'shadump/shaload: SHA-384("a" x 1000000)',
	'shadump/shaload: SHA-512("a" x 1000000)',
);

my @ext = (1, 256, 384, 512);
my $data = "a" x 999998;
my $skip;

for (my $i = 0; $i < 4; $i++) {
	$skip = 0;
	if ($ext[$i] == 384) {
		$skip = sha384_hex("") ? 0 : 1;
	}
	if ($ext[$i] == 512) {
		$skip = sha512_hex("") ? 0 : 1;
	}
	SKIP: {
		my $state;
		my $file;
		my $filename;
		skip("64-bit operations not supported", 1) if $skip;
		$filename = dirname($0) . "/state/state.$ext[$i]";
		$file = File::Spec->canonpath($filename);
		unless ($state = Digest::SHA->load($file)) {
			$state = Digest::SHA->new($ext[$i]);
			$state->add($data);
			$state->dump($file);
			$state->load($file);
		}
		$state->add_bits($data, 16);
		is(
			$state->hexdigest,
			$sharsp[$i],
			$name[$i]
		);
	}
}
