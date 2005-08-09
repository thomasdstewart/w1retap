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
use IO::File;
use IO::Pipe;
use DBI;
use POSIX qw /strftime uname/;
use SVG;
use HTML::Template;
use LWP::UserAgent;
use HTTP::Request;
use XML::Writer;
use Getopt::Std;

#use constant SDSN => 'dbi:mysql:database=w1retap;host=kanga;user=jrh';
#use constant SDSN => 'dbi:Pg:dbname=w1retap user=postgres';
#use constant WDSN => 'dbi:Pg:dbname=docks user=postgres';

use constant SDSN => 'dbi:SQLite:dbname=/var/tmp/sensors.db';
use constant WDSN => 'dbi:SQLite:dbname=/var/tmp/weatherlog.db';
use constant BASEDIR => '/var/www/roo/wx';
use constant WURL => 'http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php';

use constant RAD => 0.017453292;
use constant G => -9.80665;
use constant R => 287.04;

use vars qw /%opt/;

sub d2r {$_[0]*RAD}

sub drawwind($)
{
  my $ang = shift;

# create an SVG object
  my $svg= SVG->new(width=>200,height=>200);
  $svg->title()->cdata('Wind Direction');

  my $xv =  [100, 120, 105, 110, 100,  90, 95, 80  ];
  my $yv =  [ 20,  60,  40, 120, 110, 120, 40, 60  ];

  my $points = $svg->get_path( 'x' => $xv,
			       'y' => $yv, '-type' => 'polygon');

  $svg->circle(cx=>100, cy=>100, r=>98,
	       stroke => 'rgb(21,68,133)',
	       'stroke-width'=>'4',
	       fill => 'white');

  my $j = 0;

  my @nms = ('N','NW','W','SW','S','SE','E','NE');

  for (my $i = 0; $i < 360; $i +=15)
  {
    my ($sw,$ln);
    my $txt;
    if (($i % 45 == 0))
    {
      $txt = $nms[$j];
      $sw = 4;
      $ln = 85;
      $j++;
    }
    else
    {
      $txt = undef;
      $sw = 2;
      $ln = 90;
    }

    my $x1 = 100 - 100.0 * sin(d2r($i));
    my $y1 = 100 - 100.0 * cos(d2r($i));
    my $x2 = 100 - $ln * sin(d2r($i));
    my $y2 = 100 - $ln * cos(d2r($i));

    if (defined($txt))
    {
      my $x0 = 100 - 70 * sin(d2r($i)) - length($txt)*7;
      my $y0 = 100 - 70 * cos(d2r($i)) + 7;
      $svg->text('x' => $x0, 'y' => $y0, 'style' =>
	       {'font-family'=>'Arial',
		 'font-size'=>20 }) ->cdata($txt);
    }

    $svg->line(x1=>$x1,y1=>$y1,x2=>$x2,y2=>$y2, stroke=>'black',
	       'fill-opacity'=>0,
	       'stroke-width'=> $sw);
  }

  my $rot = 'rotate('.$ang.',100,100)';
  $svg->polygon(%$points, fill=> 'red',
		'fill-opacity'=>0.5,
		transform => $rot);

  # now render the SVG object, implicitly use svg namespace
  my $fh = new IO::File "winddirn.svg","w";
  $fh->print($svg->xmlify);
  $fh->close;
}

sub drawthermo($)
{
  my $temp = shift;
  my ($svg,$y,$ht);

  $svg= SVG->new(width=>200,height=>800);

  $svg->rectangle(x=>58, y=>2, height=>680, width=>96, 'stroke-width'=>4,
		  stroke=>'black', fill=>'white', 'fill-opacity'=>1,
		  rx=>5, ry=>5);

  $svg->circle(cx=>100,,cy=>700, r=>96, 'stroke-width'=>4,
	       stroke=>'black', fill=>'pink');

  $y = 480 - 12 * $temp;
  $ht = 700-$y;

  $svg->rectangle(x=>60, y=>$y, height=>$ht, width=>92, 'stroke-width'=>0,
		  fill=>'pink', 'fill-opacity'=>1);

  my ($x1,$x2,$j,$wid);

  my $g = $svg->tag('g', style => {'stroke'=>'black', 'fill-opacity'=>0} );

  for ($j = -10; $j < 40; $j++)
  {
    if (($j % 10) == 0)
    {
      $wid = 98;
    }
    elsif (($j % 5) == 0)
    {
      $wid = 48;
    }
    else
    {
      $wid = 20;
    }
    $y = 480 - 12 * $j;
    $x1 = 106-$wid/2;
    $x2 = 104+$wid/2;
    $g->line(x1=>$x1, y1=>$y, x2=>$x2, y2=>$y,
	     'stroke-width'=>1 + ($wid == 98));

    if (($wid == 98))
    {
      $svg->text('x' => $x1+4, 'y' => $y-2,
		 'style' => 
	       {'font-family'=>'Arial','font-size'=>36})->cdata($j);
    }
  }

  # now render the SVG object, implicitly use svg namespace
  my $fh = new IO::File "thermo.svg","w";
  $fh->print($svg->xmlify);
  $fh->close;
}

=pod
sub drawplots()
{
  my $pipe = new IO::Pipe;
  $pipe->writer("gnuplot");
  print $pipe <<EOF;
set bmargin 4
set key top right
set key box
set grid

set xlabel
set format x "%H:%M"
set timefmt "%Y-%m-%d.%H:%M"
set xdata time
set xtics rotate 900

set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "wdirn.png"
set title "Wind Direction"
set yrange [ 0 : 359 ]
set ylabel "degrees"
plot 'w0.dat' using 1:2  title "Dirn" with lines lw 3

set autoscale
set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "wspeed.png"
set title "Wind Speed"
set ylabel "knots"
plot  'w0.dat' using 1:4 t "Gust" with lines lw 3, 'w0.dat' using 1:3 t "Average" with lines lw 3

set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "tide.png"
set title "Tide"
set yrange [ 0 : 5 ]
set ylabel "height (m)"
set datafile missing "-1"
plot 'w0.dat' using 1:5 t "Tide" with lines lw 3

set autoscale
set terminal png transparent  size 400,200 enhanced  xffffff x000000 x202020
set output "gtemp.png"
set title "Greenhouse"
set ylabel "\260C"
plot 'w1.dat' using 1:2 t "Temp" with lines lw 3

set terminal png transparent  size 400,200 enhanced  xffffff x000000 x202020
set output "temp.png"
set title "Outside Temperature"
#set yrange [ -10 : 30 ]
set ylabel "\260C"
plot 'w2.dat' using 1:2 t "Temp" with lines lw 3

set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "press.png"
set title "Pressure"
#set yrange [ 980 : 1040 ]
set ylabel "millibar"
plot 'w3.dat' using 1:2 t "Pressure" with lines lw 3

set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "humid.png"
set title "Relative Humidity"
#set yrange [ 0: 100 ]
set ylabel "Rel Humid %"
plot 'w4.dat' using 1:2 t "RH" with lines lw 3

set autoscale
set terminal png transparent size 400,200 enhanced  xffffff x000000 x202020
set output "temps.png"
set title "Comparative Temperatures"
set ylabel "\260C"
plot  'w5.dat' using 1:3 t "Outside" with lines lw 3, 'w5.dat' using 1:2 t "Greenhouse" with lines lw 3
EOF
  $pipe->close;
}
=cut

sub pr_msl($$$)
{
  my ($p0, $temp, $z) = (shift, shift, shift);
  my $zdiff = (0 - $z);
  my $kt = $temp +  273.15;
  my $x = (G * $zdiff / (R * $kt));
  $p0 * exp($x);
}

sub CtoF($)
{
  my $d = shift;
   (9.0/5.0)*$d + 32;
}

sub mbtoin ($)
{
  my $mb = shift;
  $mb/33.8638864;
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

############################# main ###############################
my ($hostname, $itype, $imime);

my $now = time;
my @tm = localtime($now);
my $lh;

# Default database DSNs

$opt{S} = SDSN;
$opt{W} = WDSN;

getopts('tsglSW', \%opt);

$lh = new IO::File "/var/tmp/weather.log", "a" if $opt{'l'};

if (($opt{'s'}))
{
  $imime='"image/svg+xml"';
  $itype='svg';
  $opt{p} = 1;
}
else
{
  $imime='"image/png"';
  $itype='png';
}

my $base = shift;
$base ||= BASEDIR;
chdir $base if ( -d $base);

(undef,$hostname,undef,undef,undef) = POSIX::uname();
$opt{t} = 1 unless $hostname eq 'roo';

print 'on host '.$hostname."\n" if $opt{t};

my $then = $now - 24*3600;

# Allow sensors to update (JH runs w1retap every 2 minutes, and this every 10)
sleep 10 if (($tm[1] % 10) == 0 and !$opt{t});

my $fh = new IO::File "w0.dat", "w";
my $dbh = DBI->connect($opt{W}, '', '', {'RaiseError' => 1});
my $smt = $dbh->prepare(qq/select reportdate, wind_dirn, wind_speed,
wind_speed1, tide from observations where reportdate > $then
order by reportdate /);
$smt->execute();
my $ws;
my @dock;

while ((($ws = $smt->fetchrow_arrayref))) {
  $$ws[0] = strftime("%Y-%m-%d.%H:%M", localtime($$ws[0]));
  $$ws[2] *= 0.8;
  $$ws[3] *= 0.8;
  $fh->print(join(' ',@$ws)."\n");
  @dock = @$ws;
}
$smt->finish;
$dbh->disconnect();
$fh->close;

my $fh1 = new IO::File "w1.dat", "w";
my $fh2 = new IO::File "w2.dat", "w";
my $fh3 = new IO::File "w3.dat", "w";
my $fh4 = new IO::File "w4.dat", "w";
my $fh5 = new IO::File "w5.dat", "w";

$dbh = DBI->connect($opt{S}, '', '', {'RaiseError' => 1}) or die;

$smt = $dbh->prepare(q/select * from station/);
$smt->execute();
my $stn = $smt->fetchrow_hashref;
$smt->finish;

$smt = $dbh->prepare(qq/select date, name, value from readings where
  date > $then order by date /);
$smt->execute();

my $ld = 0;
my $r0 = 0;
my ($lt0,$lt1,$r1);
my $maxt = -999;
my $mint= 999;
my ($gtemp,$temp,$press,$humid,$dewpt);
my $as;
my ($d1,$d2);
$d1 = -1;
$d2 = -2;
my ($sdate,$edate);
my $hmax = 0;

while ( (($as = $smt->fetchrow_arrayref)) )
{

  my $d = strftime("%Y-%m-%d.%H:%M", localtime($$as[0]));
 SW1:
   {
     ($$as[1] eq 'GHT') && do
     {
       $d1 = $$as[0];
       $gtemp = $$as[2];
       $maxt =  $gtemp if ($gtemp > $maxt);
       $mint = $gtemp if ($gtemp < $mint);
       $fh1->print($d.' '.$gtemp."\n");
       if ($d1 == $d2 )
       {
	 $fh5->print(join(' ',($d,$gtemp,$temp))."\n");
	 $sdate=$d unless $sdate;
	 $edate=$d;
       }
       last SW1;
     };

     ($$as[1] eq 'OTMP0') && do
     {
       $d2 = $$as[0];
       $temp = $$as[2];
       $fh2->print($d.' '.$temp."\n");
       if ($d1 == $d2 )
       {
	 $fh5->print(join(' ',($d,$gtemp,$temp))."\n");
	 $sdate=$d unless $sdate;
	 $edate=$d;
       }
       last SW1;
     };

     ($$as[1] eq 'OPRS') && do
     {
       if (defined($temp))
       {
	 $press=pr_msl($$as[2],$temp, $stn->{altitude});
	 $fh3->print($d.' '.$press."\n");
       }
       last SW1;
     };

     ($$as[1] eq 'OHUM') && do
     {
       $humid=$$as[2];
       $fh4->print($d.' '.$humid."\n");
       $hmax = $humid if (($humid > $hmax));
       last SW1;
     };

     ($$as[1] eq 'RGC0') && do
     {
       if ($r0 == 0)
       {
	 $r0 = $$as[2];
	 $lt0 = $$as[0];
       }
       $r1 = $$as[2];
       $lt1 = $$as[0];
       last SW1;
     };
   }
}
$smt->finish;

$fh1->close;
$fh2->close;
$fh3->close;
$fh4->close;
$fh5->close;

my $logdate = strftime("%Y-%m-%d %H:%M", localtime($d2));
$dewpt= dewpt($temp, $humid);

$lh->print ('+Now='.$now.' time '. strftime("%Y-%m-%d %H:%M", @tm) ."\n" .
	    ' logdate: '.$logdate.' d2='.$d2.' lt1='.$lt1.' $r1='.$r1.' fact='.$stn->{rfact}."\n") if ($opt{l});

my $rain = 0;
my $rain24 = 0;

if ($stn->{rfact})
{
  my $l1h = $lt1 - 3600;
  my $r1h = $r1;

  $smt = $dbh->prepare(qq /select date, name, value from readings where
                       date = $l1h and name='RGC0' /);
  $smt->execute();
  $as = $smt->fetchrow_arrayref;
  $smt->finish;

  if (($as))
  {
    $r1h =$$as[2];
    print "Rain1(0) $r1h\n" if $opt{t};
    $lh->print (' Rain1(0)='.$r1h.' at '.$l1h."\n") if $opt{l};
  }
  else
  {
    $smt = $dbh->prepare(qq /select date, name, value from readings where
              name = 'RGC0' and date > $l1h order by date  asc limit 1 /);
    $smt->execute();
    $as = $smt->fetchrow_arrayref;
    $smt->finish;
    if (($as))
    {
      $l1h = $$as[0];
      $r1h = $$as[2];
      $lh->print (' Rain1(1)='.$r1h.' at '.$l1h."\n") if $opt{l};
    }
  }
  $rain = 3600 * $stn->{rfact}*($r1 - $r1h)/($lt1 - $l1h);
  print "Rain $rain\n" if $opt{t};
  if (($r1 - $r0) and ($lt1 - $lt0))
  {
    $rain24 = 24*3600 * $stn->{rfact}*($r1 - $r0)/($lt1 - $lt0);
  }
  print 'Rain24 '.$rain24."\n" if $opt{t};
}
else
{
  $rain = 0;
}
$lh->print (' Rain diff '.$rain. "\n") if $opt{l};

$dock[0] =~ s/\./ /;

# Latest log
if ((($tm[1] % 10) == 0))
{
  $dbh->begin_work;
  $dbh->do(qq/delete from latest where udate < $then/);
  $dbh->do(qq/insert into latest values
    ($d2, '$logdate', $gtemp, $maxt, $mint, $temp, $press, $humid, $rain,
     $dewpt, '$dock[0]', $dock[2], $dock[1], $dock[4])/);
  $dbh->commit;
}

$smt = $dbh->prepare(q/select gstamp,rain from latest order by udate/);
$smt->execute();

my $r;
my $rmax = 0.0;

$fh = new IO::File "w6.dat", "w";
while (($r = $smt->fetchrow_arrayref))
{
  my ($date,$value) = ($$r[0],$$r[1]);
  $date =~ s/ /\./;
  $fh->print(join(' ',($date, $value))."\n");
  $rmax = $value if $value > $rmax;
}

$fh->print($edate.' '.$rain."\n");
$rmax += 0.01;

$hmax *= 1.05;
$hmax = 100 if $hmax > 100;

$dbh->disconnect();
$fh->close;

drawthermo($gtemp);
drawwind($dock[1]);

$ENV{GDFONTPATH}='/usr/share/fonts/truetype:/usr/share/fonts/truetype/freefont/';
print "Start Graphs, edate=$edate rmax=$rmax \n" if $opt{'t'};
my $t_0 = time;
my $plotopt = ($opt{'s'}) ? '-svg' : '-font FreeSans -png -crop 0.2,0.5,4.2,2.5';

my $c = 'ploticus '.$plotopt.' edate='.
	 $edate.' rmax='.$rmax.' hmax='.$hmax.' currain='.$rain.' wxcrop.plt';
system($c);
print $c."\n" if $opt{t};

if ($opt{p} and !($opt{'s'}))
{
  print "Start Convert\n" if $opt{'t'};
  foreach my $img (qw /gtemp humid press temp temps tide wdirn wspeed /)
  {
    system('rsvg -w 400 -h 200 '.$img.'.svg '.$img.'.png');
  }
}

system 'rsvg -h 150 -w 40 thermo.svg thermo.png';
system 'rsvg -h 120 -w 120 winddirn.svg winddirn.png';
print 'Done Graphs '.(time - $t_0)."s\n" if $opt{'t'};

unlink ('w0.dat','w1.dat','w2.dat','w3.dat','w4.dat','w5.dat', 'w6.dat')
  unless $opt{t};

my $template = HTML::Template->new('filename' => 'wx.tmpl.html');

my $g_temp = sprintf("%.1f&deg;C / %.1f&deg;F", $gtemp, CtoF($gtemp));
my $min_t = sprintf("%.1f&deg;C / %.1f&deg;F", $mint, CtoF($mint));
my $max_t = sprintf("%.1f&deg;C / %.1f&deg;F", $maxt, CtoF($maxt));
my $prs = sprintf("%.1f hPa / %.1f inHg", $press, mbtoin($press));
my $tmp = sprintf("%.1f&deg;C / %.1f&deg;F", $temp, CtoF($temp));


$template->param(TYPE => $itype);
$template->param(IMIME => $imime);
$template->param(DATE => $logdate);
$template->param(GTEMP => $g_temp);
$template->param(GMAX => $max_t);
$template->param(GMIN => $min_t);

$template->param(TEMP => $tmp);
$template->param(PRESS => $prs);
$template->param(HUMID => sprintf("%.1f", $humid));

my $dpt = sprintf("%.1f&deg;C / %.1f&deg;F", $dewpt, CtoF($dewpt));
my $rfall = sprintf("%.2fin / %.1fmm", $rain, $rain*25.4);
my $rfall24 = sprintf("%.2fin / %.1fmm", $rain24, $rain24*25.4);

$template->param(DEWP => $dpt);
$template->param(RAIN => $rfall);
$template->param(RAIN24 => $rfall24);

$template->param(PDATE => $dock[0]);
$template->param(WDIRN => $dock[1]);
$template->param(WSPEED => $dock[2]);
$template->param(WGUST => $dock[3]);
$template->param(TIDE => $dock[4]);

$fh = new IO::File "w.html", "w";
$fh->print ($template->output);
$fh->close;
rename "w.html","index.html";

$fh = new IO::File('wx_static.xml', 'w');
my $xml = new XML::Writer(OUTPUT => $fh, DATA_INDENT => 2,
                         DATA_MODE => 1);
$xml->xmlDecl("utf-8");

$xml->startTag('report',
	       'unixepoch' => $d2,
	       'timestamp' => strftime("%FT%T%z", localtime($d2)));

$xml->startTag('station', 'place' => 'Netley Marsh');
$xml->startTag('sensor', 'name' => 'temperature', 'value' => sprintf("%.2f",$temp), 'units' => '°C');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'pressure', 'value' => sprintf("%.2f",$press), 'units' => 'hPa');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'humidity', 'value' => sprintf("%.2f",$humid), 'units' => '%');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'rainfall', 'value' => sprintf("%.4f", $rain), 'units' => 'in/hr');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'dewpoint', 'value' => sprintf("%.2f",$dewpt), 'units' => '°C');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'greenhouse', 'value' => sprintf("%.2f",$gtemp), 'units' => '°C');
$xml->endTag('sensor');

$xml->endTag('station');


$xml->startTag('station', 'place' => 'Southampton Docks');
$xml->startTag('sensor', 'name' => 'windspeed', 'value' => $dock[2], 'units' => 'knots');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'direction', 'value' => $dock[1], 'units' => 'degrees');
$xml->endTag('sensor');

$xml->startTag('sensor', 'name' => 'tide', 'value' => $dock[4], 'units' => 'metres');
$xml->endTag('sensor');

$xml->endTag('station');
$xml->endTag('report');
$xml->end();
$fh->close;

if ($opt{'t'})
{
  print join(' ',(($now - $d2), $stn->{wu_user}, ($tm[1] % 10)))."\n";
}

if((($now - $d2) < 300) and defined($stn->{wu_user}) and ($tm[1] % 10) == 0)
{
  my $url;
  $url = WURL . '?action=updateraw';
  $url .= '&ID='.$stn->{wu_user}.'&PASSWORD='.$stn->{wu_pass};
  $url .= '&dateutc='. strftime("%Y-%m-%d %H:%M",gmtime($now)).
    '&clouds=NA&softwaretype='.$stn->{software}.'&weather=NA';
  $url .= sprintf('&baromin=%.3f&dewptf=%.1f&humidity=%.1f&rainin=%.4f'.
		  '&tempf=%.1f',
		  mbtoin($press), CtoF($dewpt), $humid, $rain, CtoF($temp));



  if ($opt{'t'})
  {
    print $url."\n";
  }
  else
  {
    my $ua = LWP::UserAgent->new;
    my $req = HTTP::Request->new(GET => $url);
    my $resp = $ua->simple_request($req);
    my $resp_data = $resp->content;

    $lh->print(' Sent '.$url."\n".' Got '.$resp_data."\n") if $opt{l};

  }
}
else
{
  print " No upload\n" if $opt{t};
}

if ($opt{l})
{
  $lh->print("-End\n");
  $lh->close;
}
