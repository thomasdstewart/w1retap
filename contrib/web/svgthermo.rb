#!/usr/bin/ruby

require 'svg/svg'
require 'zlib'

WD=200
HT=800
#HPY=HT
#HPX=WD


module SvgThermo
  def SvgThermo.makeimage temp, ph=160, pw=40, noc=false
    svg = SVG.new("#{pw}px", "#{ph}px", "0 0 #{WD} #{HT}") 
    svg << SVG::Rect.new(0, 0, WD, HT) {
    self.style = SVG::Style.new(:fill => '#ffffeb', :fill_opacity => 1)}
    
    svg << SVG::Rect.new(58,2, 96, 680, 5, 5) {
      self.style = SVG::Style.new(:fill => 'white', :fill_opacity => 1,
				  :stroke => 'black', :stroke_width=>4) }
    svg << SVG::Circle.new(100,700,96) {
      self.style = SVG::Style.new(:fill => 'pink', 
				  :stroke => 'black', :stroke_width=>4) }
    y = 480 - 12*temp
    ht = 700 - y
    svg << SVG::Rect.new(60, y, 92, ht) {
      self.style = SVG::Style.new(:fill => 'pink', :fill_opacity => 1,
				  :stroke_width=>0) }
    grp = SVG::Group.new { self.style = SVG::Style.new(:stroke => 'black' )}
    
    -10.upto(40) do |j|
      if j % 10 == 0
	wid = 98
      elsif j % 5 == 0
	wid = 48
      else
	wid = 20
      end
      
      y = 480 - 12 *j
      x1 = 106-wid/2
      x2 = 104+wid/2
      
      if wid == 98
        unless j == 40
          svg << SVG::Text.new(x1+14, y - 2, j.to_s)  {
            self.style = SVG::Style.new(:font_size => '24pt', 
                                        :font_family => 'sans-serif')}
        end
	lw = 2
      else
	lw = 1
      end
      grp << SVG::Line.new(x1, y, x2, y) { 
	self.style = SVG::Style.new(:stroke_width => lw ) }
    end 
    svg << grp
    unless noc
      Zlib::GzipWriter.open('_thermo.svgz') {|gz| gz.write svg.to_s }
    else
      File.open('_thermo.svg','w') {|f| f.write svg.to_s }
    end
  end
end

if __FILE__ == $0
  temp = ARGV[0].to_i || 0
  SvgThermo.makeimage temp
end
