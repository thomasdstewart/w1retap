#!/usr/bin/perl -w

use strict;
use SVG; 
use Image::LibRSVG;

use vars qw/$svg $temp $y $ht/;

for ($temp = -5; $temp < 40; $temp++)
{
  my $col;
  SW:
  {
    ($temp > 25) && do
    {
      $col = "red";
      last SW;
    };

    ($temp > 20) && do
    {
      $col = "pink";
      last SW;
    };

    ($temp > 15) && do
    {
      $col = "orange";
      last SW;
    };

    ($temp > 10) && do
    {
      $col = "green";
      last SW;
    };

    ($temp > 5) && do
    {
      $col = "cyan";
      last SW;
    };

    $col = "blue";
    last SW;
 }

  $svg= SVG->new(width=>200,height=>800);

  $svg->rectangle(x=>58, y=>2, height=>680, width=>98, 'stroke-width'=>1,
		  stroke=>'black', fill=>'white', 'fill-opacity'=>1,
		  rx=>5, ry=>5);

  $svg->circle(cx=>100,,cy=>700, r=>98, 'stroke-width'=>1,
	       stroke=>'black', fill=>$col);

  $y = 480 - 12 * $temp;
  $ht = 700-$y;

  $svg->rectangle(x=>60, y=>$y, height=>$ht, width=>92, 'stroke-width'=>1,
		  fill=>$col, 'fill-opacity'=>1);

  my $xml = $svg->xmlify;
  my $rsvg = new Image::LibRSVG();
  $rsvg->loadFromStringAtMaxSize($xml,6,22);
  $rsvg->saveAs("thermo_".$temp.".png" );
}
