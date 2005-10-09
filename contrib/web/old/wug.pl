#!/usr/bin/perl

# Copyright (c) 2005 Jonathan Hudson <jh+w1retap@daria.co.uk>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

use DBI;
use LWP::UserAgent;
use HTTP::Request;
use POSIX qw(strftime);
use Getopt::Std;
use strict;

use constant G => -9.80665;
use constant R => 287.04;

use constant DSN => 'dbi:SQLite:dbname=/var/tmp/sensors.db';
use constant URL => 'http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php';

sub pr_sealevel($$$)
{
  my ($p0, $temp, $z) = (shift, shift, shift);
  my $zdiff = (0 - $z);
  my $kt = $temp +  273.15;
  my $x = (G * $zdiff / (R * $kt));
  my $p = $p0 * exp($x);

  $p;
}

sub CtoF($)
{
  my $d = shift;
  sprintf('%.1f', (9.0/5.0)*$d + 32);
}

sub mbtoin ($)
{
  my $mb = shift;
  sprintf("%.3f", $mb/33.8638864);
}

sub dewpt($$)
{
  my ($ct,$humid) = (shift,shift);

  my $B = ($ct > 0 ) ? 237.3 : 265.5;
  my $es = 610.78 * exp (17.2694 * $ct / ($ct + $B));
  my $e = $humid / 100 * $es;
  my $f = log ( $e / 610.78 ) / 17.2694;
  $B *  $f / ( 1 - $f);
}


my %opt;
getopts('t', \%opt);

my ($stn,$cur,$prv);
my %w;

my $dbh = DBI->connect(DSN, '', '', {'RaiseError' => 1});
my $smt = $dbh->prepare(q/select * from station/);
$smt->execute();
$stn = $smt->fetchrow_hashref;
$smt->finish;

$smt = $dbh->prepare(q/select max(date) from readings/);
$smt->execute();
my ($ds) = $smt->fetchrow_array;
$smt->finish;

my $now = time;
if (($now - $ds ) > 180)
{
  die "$now <> $ds --- No recent data" unless $opt{t};
}

$smt = $dbh->prepare(qq/select * from readings where date = $ds/);
$smt->execute();

my ($rc,$tc,$hum,$pr);

while ((($cur = $smt->fetchrow_hashref)))
{
 SW1:
   {
     ($cur->{name} eq 'OHUM') && do
     {
       $hum = $cur->{value};
       $w{humidity} = sprintf("%.1f", $hum);
       last SW1;
     };

     ($cur->{name} eq 'OPRS') && do
     {
       $pr = $cur->{value};
       last SW1;
     };

     ($cur->{name} eq 'OTMP0') && do
     {
       $tc = $cur->{value};
       $w{tempf} = CtoF($tc);
       last SW1;
     };

     ($cur->{name} eq 'RGC0') && do
     {
       $rc = $cur->{value};
       last SW1;
     };
   }
}

$w{dewptf} = CtoF(dewpt($tc, $w{humidity}));

$pr = pr_sealevel($pr,$tc, $stn->{altitude} );
$w{baromin} = mbtoin($pr);

my $dt = $ds - 3600;
$smt = $dbh->prepare(qq/select date,value from readings where
		     name = 'RGC0' and date >= $dt order by date asc limit 1/);
$smt->execute();
$prv = $smt->fetchrow_hashref;
$smt->finish;
$dbh->disconnect();

if (defined($prv->{value}) and defined($prv->{date}))
{
 my $fact = $stn->{rfact};
 $fact ||= 0.01;
 $w{rainin} = $fact*3600*($rc - $prv->{value})/($ds - $prv->{date});
}
else
{
  $w{rainin} = 0;
}

$w{dateutc} = strftime("%Y-%m-%d %H:%M",gmtime($ds));
$w{softwaretype} = $stn->{software};
$w{weather} = "NA";
$w{clouds} = "NA";
my $url = URL."?action=updateraw&ID=".$stn->{wu_user}."&PASSWORD=".
  $stn->{wu_pass};

foreach my $key (sort(keys(%w))){ $url .= "&".$key."=".$w{$key} }

if ($opt{t})
{
  print $url."\n";
}
else
{
  my $ua = LWP::UserAgent->new;
  my $req = HTTP::Request->new(GET => $url);
  my $resp = $ua->simple_request($req);
  my $result = $resp->content;
  print $result."\n";
}
exit(0);
