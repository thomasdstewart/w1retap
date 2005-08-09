#!/usr/bin/perl

my $m1 = '"mailto:jh\+w1retap@daria\.co\.uk">jh\+w1retap@daria\.co\.uk';
my $m2 = '"&#109;&#97;&#105;&#108;&#116;&#111;&#58;&#106;&#104;&#43;&#119;&#49;&#114;&#101;&#116;&#97;&#112;&#64;&#100;&#97;&#114;&#105;&#97;&#46;&#99;&#111;&#46;&#117;&#107;">email';
my $m3='file:///home/jrh/Projects/w1retap/docs/';
my $m4='applet.png';
while(<>)
{
    s/$m1/$m2/g;
    s|$m3|$m4|;
    s| width="\d+" height="\d+"||;
    print;
}
