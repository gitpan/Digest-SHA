package Digest::SHA;

use 5.008;
use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Digest::SHA ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.

our %EXPORT_TAGS = ( 'all' => [ qw(
	sha1base64
	sha1hex
	sha256base64
	sha256hex
	sha384base64
	sha384hex
	sha512base64
	sha512hex
	shabase64
	shaclose
	shadup
	shafinish
	shahex
	shaopen
	shawrite
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.9';

require XSLoader;
XSLoader::load('Digest::SHA', $VERSION);

# Preloaded methods go here.

1;
__END__

=head1 NAME

Digest::SHA - Perl extension for Bitwise SHA-1/256/384/512

=head1 SYNOPSIS

  # Direct computation
  use Digest::SHA qw(sha1hex sha1base64 sha256hex sha256base64
		sha384hex sha384base64 sha512hex sha512base64);

  $digest = sha1hex($data, $data_length_in_bits);
  $digest = sha1base64($data, $data_length_in_bits);


  # Iterative computation
  use Digest::SHA qw(shaopen shawrite shafinish shaclose
			shahex shabase64 shadup);

  $state = shaopen($alg);	# $alg = 1, 256, 384, or 512

  shawrite($data, $data_length_in_bits, $state);
  shawrite($moredata, $moredata_length_in_bits, $state);
  shawrite($evenmoredata, $evenmoredata_length_in_bits, $state);

  shafinish($state);

  $digest = shahex($state);
  $digest = shabase64($state);

  shaclose($state);


=head1 ABSTRACT

Digest::SHA provides a Perl interface to all algorithms defined in
the NIST Secure Hash Standard (FIPS PUB 180-2).  The routines are
general-purpose, allowing digests to be calculated for bit-strings
as well as byte-strings.  The underlying code is written in C.

=head1 DESCRIPTION

Digest::SHA offers two ways to calculate digests: all-at-once, or
in stages.  The first is simpler, and often requires only one line
of Perl.  The second is more general, allowing input to be processed
iteratively in chunks.

To illustrate the difference, the following program calculates the
SHA-256 digest of "hello world" using the two different methods:

	use Digest::SHA ':all';

	my $data = "hello world";
	my @frags = ("hello", " ", "world");

	my $all_at_once = sha256hex($data, 8*length($data));

	my $state = shaopen(256);
	for (@frags) {
		shawrite($_, 8*length($_), $state);
	}
	shafinish($state);
	my $in_stages = shahex($state);

	print $all_at_once eq $in_stages ?
		"whew!\n" : "career in aluminum siding\n";

Note that the second argument of "shahex()" and "sha256hex()" is
multiplied by 8.  This is because the second argument reflects a
I<bit count>, not a byte count.

Calculating the digest value of a bit-string is equally straightforward.
Let's say the input string is 446 bits, consisting of the fragment
"110" repeated 148 times, followed by the fragment "11".  Here's
how to calculate its SHA-1 digest:

	$digest = sha1hex(pack("B*", ("110"x148)."11"), 446);

=head1 EXPORT

None by default.

=head1 EXPORTABLE FUNCTIONS

=over 4

Provided your C compiler supports 64-bit types (i.e. long long),
all of these functions will be available for use.  If it doesn't,
you won't have access to the SHA-384 and SHA-512 routines, which
require 64-bit operations.

=item B<Direct Functions>

=item I<sha1hex($data, $data_length_in_bits)>

Returns the SHA-1 digest of $data, encoded as a 40-character
hexadecimal string.

=item I<sha1base64($data, $data_length_in_bits)>

Returns the SHA-1 digest of $data, encoded as a Base64 string.

=item I<sha256hex($data, $data_length_in_bits)>

Returns the SHA-256 digest of $data, encoded as a 64-character
hexadecimal string.

=item I<sha256base64($data, $data_length_in_bits)>

Returns the SHA-256 digest of $data, encoded as a Base64 string.

=item I<sha384hex($data, $data_length_in_bits)>

Returns the SHA-384 digest of $data, encoded as a 96-character
hexadecimal string.  This function will be undefined if your C
compiler lacks support for 64-bit integral types.

=item I<sha384base64($data, $data_length_in_bits)>

Returns the SHA-384 digest of $data, encoded as a Base64 string.
This function will be undefined if your C compiler lacks support
for 64-bit integral types.

=item I<sha512hex($data, $data_length_in_bits)>

Returns the SHA-512 digest of $data, encoded as a 128-character
hexadecimal string.  This function will be undefined if your C
compiler lacks support for 64-bit integral types.

=item I<sha512base64($data, $data_length_in_bits)>

Returns the SHA-512 digest of $data, encoded as a Base64 string.
This function will be undefined if your C compiler lacks support
for 64-bit integral types.

=item B<Iterative Functions>

=item I<shaopen($alg)>

Begins the iterative calculation of a SHA digest, returning a state
variable for use by subsequent iterative "sha...()" functions.
The $alg argument determines which SHA transform will be used (e.g.
$alg = 256 corresponds to SHA-256).  This function will return a
NULL value for $alg = 384 or $alg = 512 if your C compiler lacks
support for 64-bit integral types.

=item I<shawrite($data, $data_length_in_bits, $state)>

Updates the SHA state by feeding in $data.  This function is called
repeatedly until all data has been processed.  The value of
$data_length_in_bits B<must not> exceed 2^32-1 for each individual
call of "shawrite()".  However, per the NIST standard, the total
accumulated length of the data stream may be as large as 2^64-1
for SHA-1 and SHA-256, or 2^128-1 for SHA-384 and SHA-512.

=item I<shafinish($state)>

Finalizes the SHA calculation by padding and transforming the final
block(s), and updating the state.  It is necessary to call this
function before attempting to access the final digest value through
"shahex()" or "shabase64()".  However, calling them beforehand may
be useful to folks who are interested in examining SHA's internal
state at various stages of the digest computation.

=item I<shahex($state)>

Returns the digest value, encoded as a hexadecimal string.

=item I<shabase64($state)>

Returns the digest value, encoded as a Base64 string.

=item I<shadup($state)>

Makes a copy of the current state, and returns a handle to that
copy.  This is useful in speeding up SHA calculations for data sets
that share identical headers.  See the "gillogly.hard" script in
the "t/" subdirectory for an example of how "shadup()" can improve
performance for such data sets.

=item I<shaclose($state)>

Frees all memory associated the SHA calculation initiated with the
corresponding "shaopen()" call.

=head1 SEE ALSO

Digest, Digest::SHA1

The Secure Hash Standard (FIPS PUB 180-2) can be found at:

	http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf

=head1 AUTHOR

Mark Shelor, E<lt>mshelor@comcast.netE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2003 by Mark Shelor

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
