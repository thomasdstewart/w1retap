#!/usr/bin/ruby
require 'svg/svg'
require 'zlib'

module SvgWind
  def SvgWind.makeimage dirn, ph=120, pw=120, noc=false
    svg = SVG.new('120px', '120px', '0 0 200 200')
    svg << SVG::Circle.new(100,100,98) {
      self.style = SVG::Style.new(:fill => 'white', 
				  :stroke => '#154485', :stroke_width=>4) }

    nms = ['N','NW','W','SW','S','SE','E','NE' ]
    0.step(359,15) do |i|
      rad =  i * (Math::PI / 180)
      if (i % 45) == 0
	txt = nms.shift
	sw = 4
	ln = 85
	x0 = 100 - 70 * Math.sin(rad) - txt.length * 7
	y0 = 100 - 70 * Math.cos(rad) + 7
	svg <<  SVG::Text.new(x0, y0, txt) {
	  self.style = SVG::Style.new(:font_size => '18pt', :font_family => 'sans-serif')}
      else
	sw = 2 
	ln = 90
      end
      
      x1 = 100 - 100.0 * Math.sin(rad)
      y1 = 100 - 100.0 * Math.cos(rad)
      x2 = 100 - ln * Math.sin(rad)
      y2 = 100 - ln * Math.cos(rad)
      svg << SVG::Line.new(x1, y1, x2, y2) {
	self.style = SVG::Style.new(:stroke_width => sw, :stroke=>'black' ) }
    end

    if dirn >= 0
      svg << SVG::Polygon.new(SVG::Point[100, 20, 120, 60, 105, 40, 110, 170, 100, 150, 90, 170, 95, 40, 80, 60 ]) { 
	self.transform = 'rotate('+dirn.to_s+',100,100)'
	self.style = SVG::Style.new(:fill => 'red', :fill_opacity => 0.5 )} 
      svg << SVG::Circle.new(100,100,5) {
      self.style = SVG::Style.new(:fill => '#EE5252', 
				  :stroke => '#F25B5B', :stroke_width=>2)} 
    else
      svg <<  SVG::Text.new(53, 108, "No data") {
	  self.style = SVG::Style.new(:font_size => '18pt',
				      :font_family => 'sans-serif',
				      :fill => 'red' )}

    end
    unless noc
      Zlib::GzipWriter.open('_winddirn.svgz') {|gz| gz.write svg.to_s }
    else
      File.open('_winddirn.svg','w') {|f| f.write svg.to_s }
    end
  end
end

if __FILE__ == $0
  dirn = ARGV[0].to_i || -1
  SvgWind.makeimage dirn
end
