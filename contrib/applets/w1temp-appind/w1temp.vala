using Gtk;
using Soup;
using AppIndicator;

const string APPBASEPATH = "/usr/share/w1temp/";

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
            builder.add_from_file (Path.build_filename(APPBASEPATH, "ui",
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

class WeatherClient {
    private Indicator ci;
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

    public WeatherClient(string[] args) {
        Gtk.init(ref args);
        ci = new Indicator("w1temp",
                           "w1_thermo_0",
                           IndicatorCategory.APPLICATION_STATUS);
        ci.set_status(IndicatorStatus.ACTIVE);
        ci.set_label("???℃", "");
        ci.set_icon(Path.build_filename(APPBASEPATH, "pixmaps", "w1_thermo_0000.svg"));
    }

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
    
    private string icon_file_name (double temp)
    {
        int itemp = (int)Math.lrint(temp);
        if (itemp > 40) itemp = 40;
        if (itemp < -10) itemp = -10;
        string ic = "w1_thermo_%04d.svg".printf(itemp);
        return ic;
    }

    private void w1temp_about()
    {
        var dialog = new AboutDialog();
        dialog.set_program_name ("w1temp");
        Gdk.Pixbuf img;
        try
        {
            string fn = icon_file_name(ltemp);
            img = new Gdk.Pixbuf.from_file_at_size(
                Path.build_filename(APPBASEPATH, "pixmaps", fn), 16,64);
        } catch { img=null; }
        dialog.set_logo (img);
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
        ci.set_label(l, "");
        ci.set_icon(Path.build_filename(APPBASEPATH, "pixmaps", icon_file_name(temp)));
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
        var menu = new Menu();
        if (mess.status_code == 200) // SOUP_STATUS_OK
        {
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
                MenuItem mitem = new MenuItem.with_label(res);
                menu.append(mitem);

                if (temp != ltemp)
                {
                    set_temp_label(temp);
                    ltemp = temp;
                }
            } catch {}
        }
        else
        {
            ci.set_label("???", "");
            ltemp = -1001;
            MenuItem mitem = new MenuItem.with_label(
                "Failed: %s".printf(mess.reason_phrase));
            menu.append(mitem);
            stderr.printf("%s : %ld: %s\n",
                          new DateTime.now_local().to_string(),
                          mess.status_code,
                          mess.reason_phrase);
        }
        menu.append (new Gtk.SeparatorMenuItem ());
        basic_menu(menu);
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
    
    private void basic_menu(Menu menu)
    {
        var item = new Gtk.ImageMenuItem.with_mnemonic ("_Refresh");
        item.image = new Gtk.Image.from_stock (Gtk.Stock.REFRESH,
                                               Gtk.IconSize.MENU);
        menu.append (item);
        item.activate.connect (() => {
                get_config();
                if (nm_state == 3)
                {
                    if(sstate == 1)
                    {
                        session.cancel_message(message, 503);
                        sstate = 0;
                    }
                    ltemp = -1001;
                    start_get_data();
                }  });

        item = new Gtk.ImageMenuItem.with_mnemonic ("_Open in browser");
        item.image = new Gtk.Image.from_stock (Gtk.Stock.JUMP_TO,
                                               Gtk.IconSize.MENU);
        menu.append (item);
        item.activate.connect (() => {
                try {
                Gtk.show_uri(null, burl, Gdk.CURRENT_TIME);
                } catch (Error e) {} } );

        item = new Gtk.ImageMenuItem.with_mnemonic ("_Preferences ...");
        item.image = new Gtk.Image.from_stock (Gtk.Stock.PREFERENCES,
                                               Gtk.IconSize.MENU);
        item.activate.connect(() => { w1temp_prefs(); });
        menu.append (item);
        
        item = new Gtk.ImageMenuItem.with_mnemonic ("_About ...");
        item.image = new Gtk.Image.from_stock (Gtk.Stock.ABOUT,
                                               Gtk.IconSize.MENU);
        item.activate.connect(w1temp_about);
        menu.append (item);

        item = new Gtk.ImageMenuItem.from_stock (Gtk.Stock.QUIT, null);
        item.activate.connect(()=> {Gtk.main_quit ();});
        menu.append (item);
        menu.show_all();
        ci.set_menu(menu);
    }

    public void run()
    {
        get_config();
        var menu = new Menu();
        basic_menu(menu);
        start_nm_dbus();
        Idle.add(() => {start_get_data(); return false;});
        Gtk.main();
    }
}

static int main(string[] args) {
	var wc = new WeatherClient(args);
        wc.run();
	return 0;
}
