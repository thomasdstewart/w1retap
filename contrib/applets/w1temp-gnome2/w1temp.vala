using Panel;
using Gtk;
using Soup;

const string UIBASEPATH = "/usr/share/w1temp/";

enum w1state { WAITING=0, FETCHING}
// Combines 0.8 and 0.9 APIs ...
enum nmstate {UNKNOWN=0, ASLEEP=1, CONNECTING=2, CONNECTED=3, DISCONNECTED=4,
              NM_STATE_ASLEEP           = 10,
              NM_STATE_DISCONNECTED     = 20,
              NM_STATE_DISCONNECTING    = 30,
              NM_STATE_CONNECTING       = 40,
              NM_STATE_CONNECTED_LOCAL  = 50,
              NM_STATE_CONNECTED_SITE   = 60,
              NM_STATE_CONNECTED_GLOBAL = 70}

[DBus (name = "org.freedesktop.NetworkManager")]
interface NetworkManager : GLib.Object {
    public signal void StateChanged (uint32 state);
    public abstract uint32 State {owned get;}  
}

class Prefs
{
    private bool keep = false;
    public Prefs() {}
    public bool run (ref string _url, ref string _burl, ref int _delay)
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
            entry1.text = _url;
            entry3.text = _burl;
            entry2.text = _delay.to_string();
            dialog.run ();
            if(keep)
            {
                _url = entry1.text;
                _burl = entry3.text;
                _delay = int.parse(entry2.text);
                if(_delay < 5)
                    _delay = 5;
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
    private uint32 nm_state = nmstate.UNKNOWN;
    private int sstate = w1state.WAITING;
    private string url {get; set; default="";}
    private int delay {get; set; default=300;}
    private int fdelay=2;
    private string key {get; set; default="temperature";}
    private string burl {get; set; default="http://roo/wx/";} 
    private Gtk.Label label;
    private Gtk.Image image;
    private bool logme = false;
    private GLib.Settings settings;
    private string logfile=null;
    private uint tid;
    
    private string getlog()
    {
        if (logfile == null)
        {
            var u = Environment.get_user_name();
            var d = Environment.get_variable("DISPLAY");
            var n = d.index_of(":");
            var dn = d.slice(n+1, d.length);
            logfile="/tmp/.w1temp-%s-%s".printf(u,dn);
        }
        return logfile;
    }

    private void log_msg(string txt)
    {
        var fp = FileStream.open(getlog(),"a");
        if(fp != null)
        {
            var dt = new DateTime.now_local().to_string();
            fp.printf("%s : %s\n", dt, txt);
        }
    }
    
    private void start_nm_dbus()
    {
        try {
            nm = Bus.get_proxy_sync (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     "/org/freedesktop/NetworkManager");
            nm.StateChanged.connect ((state) =>
                {
                    nm_state = state;
                    if(tid > 0) {Source.remove(tid);}
                    if (nm_state == nmstate.CONNECTED ||
                        nm_state == nmstate.NM_STATE_CONNECTED_GLOBAL)
                    {
                        if(sstate == w1state.FETCHING)
                        {
                            session.cancel_message(message, 503);
                            sstate = w1state.WAITING;
                        }
                        fdelay = 2;                        
                        start_get_data(); 
                    }
                    if(logme)
                        log_msg("nm_state %ld".printf(nm_state));
                });
        } catch (IOError e) {
            stderr.printf ("%s\n", e.message);
            if(logme)
                log_msg(e.message);
        }
        nm_state = nm.State;
        if(logme)
            log_msg("nm_startup %ld".printf(nm_state));

    }
   private void get_config()
    {
        url = settings.get_string ("url");
        burl = settings.get_string("browse-url");
        key = settings.get_string("key");
        delay = settings.get_int ("delay");
        if(delay < 5)
            delay = 5;
        logme = settings.get_boolean ("log-errors");
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
        settings = new GLib.Settings ("org.w1retap.w1temp");
        settings.changed.connect (() => { get_config(); } );
        get_config();
        start_nm_dbus();
        Idle.add(start_get_data);
        show_all();
    }
 
    private static void on_refresh_clicked (BonoboUI.Component component,
                                          void* user_data, string cname) {
        var ap = (MainApplet)user_data;
        ap.get_config();
        ap.ltemp = -1001;
        if(ap.tid > 0) {Source.remove(ap.tid);}
        if(ap.sstate == w1state.FETCHING)
        {
            ap.session.cancel_message(ap.message, 503);
            ap.sstate = w1state.WAITING;
        }
        else if (ap.nm_state == nmstate.CONNECTED ||
                 ap.nm_state == nmstate.NM_STATE_CONNECTED_GLOBAL)
        {
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


        string vers = (ap.ltemp > -100) ? "%.1f".printf(ap.ltemp) : "0.0";
        dialog.set_version(vers);
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
    
    private bool start_get_data()
    {
        tid=0;
        if (nm_state == nmstate.CONNECTED ||
            nm_state == nmstate.NM_STATE_CONNECTED_GLOBAL)
        {
            session = new Soup.SessionAsync ();
            session.timeout = (delay * 3) / 4;
            string xurl;
            xurl =  @"$url".printf(time_t());
            message = new Soup.Message ("GET", xurl);
            sstate = w1state.FETCHING;
            session.queue_message (message, end_session);
        }
        return false;
    }

   private void end_session(Soup.Session sess, Soup.Message mess)
    {
        int timer=delay;
        var status = "";
        var builder = new StringBuilder ();
        sstate = w1state.WAITING;
        if (mess.status_code == Soup.KnownStatusCode.OK) // SOUP_STATUS_OK
        {
            try
            {
                var jsp = new Json.Parser ();
                jsp.load_from_data ((string) mess.response_body.flatten().data, -1);
                var root_object = jsp.get_root().get_array();
                var temp = -999.9;
                foreach (var node in root_object.get_elements ()) {
                    var obj = node.get_object ();
                    var k = obj.get_string_member ("name");
                    var v = obj.get_string_member ("value");
                    var u = obj.get_string_member ("units");
                    builder.append("%s: %s %s\n".printf(k,v,u));
                    if(0 == k.ascii_casecmp("temperature"))
                    {
                        temp= double.parse(v);
                    }
                }
                tooltip_text = builder.str.chomp();
                if (temp != ltemp)
                {
                    set_temp_icon(temp);
                    set_temp_label(temp);
                    show_all();
                    ltemp = temp;
                }
            } catch (Error e)
            {
                tooltip_text = "Failed %s\n".printf(e.message);
            }
            fdelay = 2;
        }
        else
        {
            label.set_text("???");
            timer = fdelay;
            status = " [%d]".printf(timer);
            ltemp = -1001;
            var txt = "%s%s : %ld: %s".printf(
                new DateTime.now_local().to_string(),
                status, mess.status_code, mess.reason_phrase);
            tooltip_text = txt;
            if(fdelay < delay)
                fdelay = fdelay + fdelay/2;
        }
        tid = Timeout.add_seconds(timer, start_get_data);
        if(logme)
        {
            var txt = "status %ld%s - %s".printf(
                mess.status_code, status, mess.reason_phrase);
            log_msg(txt);
        }
    }

    private void w1temp_prefs()
    {
        var p = new Prefs();
        var _url=url;
        var _burl=burl;
        var _delay=delay;
        var res = p.run(ref _url, ref _burl, ref _delay);
        if (res)
        {
            if (_delay < 120) _delay = 120;                
            if (delay != _delay) 
            {
                delay = _delay;
                settings.set ("delay","u", delay);
            }
            if (url != _url)
            {
                url = _url;
                settings.set_string ("url", url);
            }
            if (burl != _burl)
            {
                burl = _burl;
                settings.set_string("browse-url", burl);
            }
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
