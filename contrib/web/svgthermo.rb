#!/usr/bin/ruby

require 'svg/svg'
require 'tempfile'

module SvgThermo
  def SvgThermo.makeimage temp, ph=150, pw=40
    svg = SVG.new('2in', '8in', '0 0 200 800')
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
	svg << SVG::Text.new(x1+4, y - 2, j.to_s)  {
	  self.style = SVG::Style.new(:font_size => 36, 
				      :font_family => 'Arial')}
	lw = 2
      else
	lw = 1
      end
      grp << SVG::Line.new(x1, y, x2, y) { 
	self.style = SVG::Style.new(:stroke_width => lw ) }
    end 
    svg << grp

    tf = Tempfile.new('thermo')
    tf.open
    tf.print svg.to_s
    tf.close
    system "rsvg -h #{ph} -w #{pw} #{tf.path} thermo.png"
  end
end

if __FILE__ == $0
  temp = ARGV[0].to_i || 0
  SvgThermo.makeimage temp
end
