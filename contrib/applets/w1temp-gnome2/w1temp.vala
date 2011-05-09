using Panel;
using Gtk;
using Soup;

const string UIBASEPATH = "/usr/share/w1temp/";

[DBus (name = "org.freedesktop.NetworkManager")]
interface NetworkManager : GLib.Object {
    public signal void StateChanged (uint32 state);
    public abstract uint32 State {owned get;}  
}

class Prefs
{
    private bool keep = false;
    public Prefs() {}
    public bool run (ref string url, ref string burl, ref int delay)
    {
        try {
            var builder = new Builder ();
            builder.add_from_file (Path.build_filename(UIBASEPATH, "ui",
                                                       "w1temp.ui"));
            var dialog = builder.get_object ("dialog1") as Dialog;
            var button = builder.get_object ("button1") as Button;
            var entry1 = builder.get_object ("entry1") as Entry;
            var entry2 = builder.get_object ("entry2") as Entry;
            var entry3 = builder.get_object ("entry3") as Entry;            
            button.clicked.connect(() => { keep = true; });
            entry1.text = url;
            entry3.text = burl;
            entry2.text = delay.to_string();
            dialog.run ();
            if(keep)
            {
                string _url = entry1.text;
                string _burl = entry3.text;
                int _delay = int.parse(entry2.text);
                if (_delay < 120) _delay = 120;                
                if (delay == _delay && url == _url && burl == _burl)
                {
                    keep = false;
                }
                else
                {
                    url = _url;
                    burl = _burl;
                    delay = _delay;
                }
            }
            dialog.destroy();            
        } catch (Error e) {
            stderr.printf ("Could not load UI: %s\n", e.message);
        } 
        return keep;
    }
}

public class MainApplet : Panel.Applet {
    private double ltemp = -1001.0;
    private Soup.Message message;
    private Soup.Session session;
    private NetworkManager nm;
    private uint32 nm_state = 0;
    private int sstate = 0;
    private string url="";
    private int delay = 300;
    private string key = "temperature";
    private string burl = "http://roo/wx/";
    private Gtk.Label label;
    private Gtk.Image image;
    
    private void start_nm_dbus()
    {
        try {
            nm = Bus.get_proxy_sync (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     "/org/freedesktop/NetworkManager");
            nm.StateChanged.connect ((state) => {
                    nm_state = state;
                    if (nm_state == 3)
                    {
                        if(sstate == 1)
                        {
                            session.cancel_message(message, 503);
                            sstate = 0;
                        }
                        start_get_data();
                    }
                });
        } catch (IOError e) {
            stderr.printf ("%s\n", e.message);
        }
        nm_state = nm.State;
    }
   private void get_config()
    {
        var kf = new KeyFile();
        string kfile = Path.build_filename(Environment.get_home_dir(),
                                           ".config/w1retap/applet");
        try {
            kf.load_from_file(kfile, KeyFileFlags.NONE);
        } catch (Error e)  { stderr.printf(e.message); }
        try {
            url = kf.get_string("Config","url"); 
        } catch (KeyFileError k) { kf.set_string("Config","url", url); }
        try {
            burl = kf.get_string("Config","browse-url");
        } catch (KeyFileError k) { kf.set_string("Config","browse-url", burl); }
        
        try {        
            delay = kf.get_integer("Config","delay");
        } catch (KeyFileError k) {kf.set_integer("Config","delay", delay); }
        try {
            key = kf.get_string("Config","key");
        } catch (KeyFileError k) { kf.set_string("Config","key", key); }
    }

    private void save_config()
    {
        try
        {
            var kf= new KeyFile();
            string kfile = Path.build_filename(Environment.get_home_dir(),
                                               ".config/w1retap/applet");
            kf.set_string("Config","url",url);
            kf.set_integer("Config","delay",delay);
            kf.set_string("Config","key",key);
            kf.set_string("Config","browse-url", burl); 
            var s = kf.to_data();
            FileUtils.set_contents(kfile, s);
        } catch  (Error e) { stdout.printf("Save prefs %s\n", e.message); }
    }

    private string icon_file_name (double temp, string ext="png")
    {
        int itemp = (int)Math.lrint(temp);
        if (itemp > 40) itemp = 40;
        if (itemp < -10) itemp = -10;
        string ic = "w1_thermo_%04d.%s".printf(itemp,ext);
        return ic;
    }

    public static bool factory (Applet applet, string iid) {
        ((MainApplet) applet).create ();
        return true;
    }
 
    private void on_change_background (Applet applet, Panel.AppletBackgroundType type,
                                       Gdk.Color? color, Gdk.Pixmap? pixmap) {
        var rc_style = new Gtk.RcStyle ();
        applet.set_style (null);
        applet.modify_style (rc_style);
        
        switch (type) {
            case Panel.AppletBackgroundType.PIXMAP_BACKGROUND:
                style.bg_pixmap[0] = pixmap;
                set_style (style);
                break;
            case Panel.AppletBackgroundType.COLOR_BACKGROUND:
                applet.modify_bg (Gtk.StateType.NORMAL, color);
                break;
        }
    }

    private void create () {

        label = new Label ("????");
        image = new Image();
        set_temp_icon(30.0);
        var hbox1 = new HBox(false, 0);
        hbox1.pack_start(image,  true, true, 0);
        hbox1.pack_end  (label,  false, false, 0);    
        add (hbox1);    

        string menu_def=null;
        try {
            var pth = Path.build_filename(UIBASEPATH, "ui", "menu.xml");
            FileUtils.get_contents(pth, out menu_def);
        } catch (FileError e) { stderr.printf("menu: %s\n", e.message); }
            
        var verbs = new BonoboUI.Verb[4];
        var verb = BonoboUI.Verb ();
        verb.cname = "Refresh";
        verb.cb = on_refresh_clicked;
        verb.user_data = this;
        verbs[0] = verb;

        verb = BonoboUI.Verb ();
        verb.cname = "Prefs";
        verb.cb = on_prefs_clicked;
        verb.user_data = this;
        verbs[1] = verb;
        
        verb = BonoboUI.Verb ();
        verb.cname = "Open";
        verb.cb = on_open_clicked;
        verb.user_data = this;
        verbs[2] = verb;
        
        verb = BonoboUI.Verb ();        
        verb.cname = "About";
        verb.cb = on_about_clicked;
        verb.user_data = this;
        verbs[3] = verb;
        
        setup_menu (menu_def, verbs, null);
        change_background.connect (on_change_background);
        get_config();
        start_nm_dbus();
        Idle.add(() => {start_get_data(); return false;});
        show_all();
    }
 
    private static void on_refresh_clicked (BonoboUI.Component component,
                                          void* user_data, string cname) {
        var ap = (MainApplet)user_data;
        ap.get_config();
        if (ap.nm_state == 3)
        {
            if(ap.sstate == 1)
            {
                ap.session.cancel_message(ap.message, 503);
                ap.sstate = 0;
            }
            ap.ltemp = -1001;
            ap.start_get_data();
        } 
    }
    
    private static void on_prefs_clicked (BonoboUI.Component component,
                                          void* user_data, string cname) {
        var ap = (MainApplet)user_data;
        ap.w1temp_prefs();
    }

    private static void on_open_clicked (BonoboUI.Component component,
                                          void* user_data, string cname) {
        var ap = (MainApplet)user_data;
        try {
            Gtk.show_uri(null, ap.burl, Gdk.CURRENT_TIME);
        } catch (Error e) {} 
    }

    private static void on_about_clicked (BonoboUI.Component component,
                                          void* user_data, string cname) {
        var dialog = new AboutDialog();
        var ap = (MainApplet)user_data;
        
        dialog.set_program_name ("w1temp");
        Gdk.Pixbuf img;
        try
        {
            string fn = ap.icon_file_name(ap.ltemp, "svg");
            img = new Gdk.Pixbuf.from_file_at_size(
                Path.build_filename(UIBASEPATH, "pixmaps", fn), 16,64);
            dialog.set_logo (img);
        } catch { }
 
        dialog.set_version("0.0");
        dialog.set_comments("w1retap indicator app");
        dialog.set_copyright("(c) 2011 Jonathan Hudson");
        dialog.set_website("http://www.daria.co.uk/wx/");
        dialog.set_website_label("w1retap Homepage");
        dialog.run();
        dialog.destroy();
    }
 
    private void set_temp_label(double temp)
    {
        string l = "%.1f℃".printf(temp);
        label.set_text(l);
    }

    private void set_temp_icon(double temp)
    {
        var pth = Path.build_filename(UIBASEPATH, "pixmaps", icon_file_name(temp));
        image.set_from_file(pth);
        image.show();
    }
    
    private void start_get_data()
    {
        session = new Soup.SessionAsync ();
        session.timeout = 60;
        string xurl;
        xurl =  @"$url".printf(time_t());
        message = new Soup.Message ("GET", xurl);
        sstate = 1;
        session.queue_message (message, end_session);
    }

   private void end_session(Soup.Session sess, Soup.Message mess)
    {
        sstate = 0;
        if (mess.status_code == 200) // SOUP_STATUS_OK
        {
                // remove final \n
            string res = ((string) mess.response_body.data)[0:-1];
            try
            {
                var re = new Regex(@"$key:\\s(\\S+)", RegexCompileFlags.CASELESS);
                var temp = -999.0;
                MatchInfo mi;
                if (re.match(res, 0, out mi))
                {
                    temp = double.parse(mi.fetch(1));
                }
                tooltip_text = res;
                
                if (temp != ltemp)
                {
                    set_temp_icon(temp);
                    set_temp_label(temp);
                    show_all();
                    ltemp = temp;
                }
            } catch {}
        }
        else
        {
            label.set_text("???");
            ltemp = -1001;
            tooltip_text = "%s : %ld: %s".printf(
                new DateTime.now_local().to_string(),
                mess.status_code,
                mess.reason_phrase);
        }
        Timeout.add_seconds(delay, () => { start_get_data(); return false; });
    }

    private void w1temp_prefs()
    {
        var p = new Prefs();
        if (p.run(ref url, ref burl, ref delay))
        {
            save_config();
        }
    }

    public static int main (string[] args) {
        Gnome.Program.init ("w1tempApplet", "0",
                                          Gnome.libgnomeui_module,
                                          args, "sm-connect", false);
        return Applet.factory_main ("OAFIID:w1tempApplet_Factory",
                                    typeof (MainApplet), factory);
    }
}
