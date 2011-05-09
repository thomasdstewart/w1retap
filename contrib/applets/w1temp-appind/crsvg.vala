using Cairo;
class WeatherImage : GLib.Object
{
    public Cairo.Context cr;
    public void create_cairo_surface(string fn, double temp)
    {
        const int H=800;
        const int W=240;
        const int R=120;
        const int CR=680;
        const int LW=120;
        const int X0=120;
        const double TWOPI = (2.0*Math.PI);
        
        var cst = new Cairo.SvgSurface (fn, W, H);
        cr = new Cairo.Context (cst);

        if (temp > 40.0)
            temp = 40.0;
        if (temp < -10)
            temp = -10;
        
        double wt = 64+(40-temp)*11;
        
        cr.set_line_cap(Cairo.LineCap.ROUND);
        cr.set_line_width(LW);
        cr.move_to(X0,60);
        cr.set_source_rgb (1, 1, 1);    
        cr.line_to(X0,wt);
        cr.stroke();
        if (temp > 25)
            cr.set_source_rgb (1, 0, 0);
        else if (temp > 20)
            cr.set_source_rgb (1, 0.75, 0.8);
        else if (temp > 15)
            cr.set_source_rgb (1, 0.65, 0);
        else if (temp > 10)
            cr.set_source_rgb (0, 1, 0);
        else if (temp > 5)
            cr.set_source_rgb (0, 1, 1);
        else
            cr.set_source_rgb (0, 0, 1);        

        cr.move_to(X0,wt);
        cr.line_to(X0,CR);
        cr.stroke();
        cr.arc(X0,CR, R, 0, TWOPI);
        cr.fill();
        cr.stroke();
        cr.save();
    }
}

void main(string[] args)
{
    double temp;

    foreach (string s in args[1:args.length])
    {
        temp = double.parse(s);
        var c = new WeatherImage();
        int itemp = (int)Math.lrint(temp);
        if (itemp > 39) itemp = 39;
        if (itemp < -10) itemp = -10;
        string ic = "w1_thermo_%04d.svg".printf(itemp);
        c.create_cairo_surface(ic, temp);
    }
}
