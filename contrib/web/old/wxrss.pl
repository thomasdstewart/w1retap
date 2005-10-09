#!/usr/bin/perl -w

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

use strict;
use XML::RSS;
use IO::File;
use Net::FTP;
use DBI;
use POSIX qw(strftime);

#use constant DSN => 'dbi:Pg:dbname=w1retap user=postgres';
use constant DSN => 'dbi:SQLite:dbname=/var/tmp/sensors.db';
use constant RDF => 'wx.xml';

use vars qw/$rss $date/;

sub doftp($$)
{
  my ($host,$last) = (shift,shift);

  # Assume we get user,password from .netrc
  my $ftp = Net::FTP->new($host, Debug => 0, Passive => 1)
    or die 'Cannot connect to '.$host.": $@";

  my @files = qw /gtemp.png press.png temp.png tide.png wdirn.png wspeed.png
		  humid.png thermo.png wdirn.png winddirn.png index.html
		  temps.png rain.png wx.xml wx_static.xml/;

  $ftp->login(undef,undef) or die "Cannot login ", $ftp->message;
  $ftp->binary;
  $ftp->cwd('wx');

  push(@files, 'rss_'.$last.'.html');
  foreach my $f (@files)
  {
    $ftp->put($f) if ( -e $f);
  }

  # delete any file 10 hours old
  my $lrss = $last - 3600*24;
  $ftp->delete('rss_'.$lrss.'.html');
  $ftp->quit ();
}

sub add_rss
{
  my $l = shift;
  my $c='<table>';
  $c .= '<tr><td>Time</td><td>'.$l->{gstamp}.'</td></tr>';
  $c .= '<tr><td>Greenhouse</td><td>'.sprintf('%.1f', $l->{ght}).
    'C</td></tr>';
  $c .= '<tr><td>Outside</td><td>'.sprintf('%.1f', $l->{temp}).
    'C</td></tr>';
  $c .= '<tr><td>Pressure</td><td>'.sprintf('%.1f', $l->{pres}).'hPa</td></tr>';
  $c .= '<tr><td>Rel. Humidity</td><td>'.sprintf('%.1f', $l->{humid}).
    '%</td></tr>';
  $c .= '<tr><td>Rainfall</td><td>'.sprintf('%.1f', $l->{rain}).
    'in/hr</td></tr>';
  $c .= '<tr><td>Dew Point</td><td>'.sprintf('%.1f', $l->{dewpt}).
    'C</td></tr>';
  $c .= '</table>';

  if (! -e  'rss_'.$l->{udate}.'.html')
  {
    my $fh = new IO::File 'rss_'.$l->{udate}.'.html',"w";
    $fh->print('<html><head<title>weather</title><meta http-equiv="refresh" '.
	     'content="0;url=http://www.daria.co.uk/wx/"></head>'.
	     '<body><a href="http://www.daria.co.uk/wx/">Weather</a><p>'.
	     $c .'<p></body></html>');
    $fh->close;
  }

  $c =~ s/</&lt;/g;
  $c =~ s/>/&gt;/g;

  $rss->add_item(title => 'Weather in Netley Marsh '. $l->{gstamp},
		 link => 'http://www.daria.co.uk/wx/rss_'.$l->{udate}.'.html',
		 description => $c,
		 mode => 'insert',
		 my => {'tag' => $l->{udate}}
		);
  $l->{udate};
}

my $host = shift;
my $dir = shift;

chdir $dir if $dir;

$date = POSIX::strftime("%FT%TZ", gmtime());
$rss = new XML::RSS;

$rss->add_module(prefix=>'my', uri=>'http://www.daria.co.uk/my/');
my $dbh = DBI->connect(DSN, '', '', {'RaiseError' => 1});
my $smt = $dbh->prepare(q/select * from station/);
$smt->execute();
my $stn = $smt->fetchrow_hashref;
$smt->finish;

my $last;

my $l;
$rss->channel(
	      title => "Jonathan and Daria's Weather",
              link => 'http://www.daria.co.uk/wx/',
              description => 
	      'A personal weather station in Netley Marsh, Hampshire',
              dc => {rights    => 'Copyright (c) 2005 Jonathan Hudson',
                     creator   => 'jh+weather@daria.co.uk',
                     language  => 'en',
		     date => $date
		    },

	      syn => {
		      updatePeriod     => "hourly",
		      updateFrequency  => "1",
		      updateBase       => "1901-01-01T00:00+00:00",
		     }
	     );

$smt = $dbh->prepare(q/select * from latest where (udate % 1800) = 0 order by udate asc/);
$smt->execute();

while (( $l = $smt->fetchrow_hashref))
{
  $last = add_rss($l);
}

$smt->finish;
$dbh->disconnect;

$rss->{output} = '1.0';
$rss->save(RDF);

doftp($host, $last) if $host;
my $lrss = $last - 3600*24;
unlink ('rss_'.$lrss.'.html');
