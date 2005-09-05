use Test::More;
eval "use Test::Pod::Coverage 0.08";
plan skip_all => "Test::Pod::Coverage 0.08 required for testing POD coverage" if $@;

my @privfcns = qw(
	Addfile 
	Hexdigest 
	B64digest 
	shaclose
	shadump 
	shadup 
	shaload 
	shaopen 
	sharewind 
	shawrite
);

all_pod_coverage_ok( { also_private => \@privfcns } );
