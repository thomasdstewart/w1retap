//
//  Copyright 2011 (c) Jonathan Hudson  All rights reserved.
//

const Gio = imports.gi.Gio;
const Mainloop = imports.mainloop;
const St = imports.gi.St;
const Soup = imports.gi.Soup;
const Main = imports.ui.main;
const PanelMenu = imports.ui.panelMenu;
const Format = imports.misc.format;
const Gettext = imports.gettext.domain('gnome-shell');
const _ = Gettext.gettext;
const PopupMenu = imports.ui.popupMenu;
const Util = imports.misc.util;
const GLib = imports.gi.GLib;
const Gtk = imports.gi.Gtk;
const DBus = imports.dbus;
const Json = imports.gi.Json;

const NM_state = {
    UNKNOWN: 0,
    NM_STATE_ASLEEP: 10,
    NM_STATE_DISCONNECTED: 20,
    NM_STATE_DISCONNECTING: 30,
    NM_STATE_CONNECTING: 40,
    NM_STATE_CONNECTED_LOCAL: 50,
    NM_STATE_CONNECTED_SITE: 60,
    NM_STATE_CONNECTED_GLOBAL: 70
};

const W1_state = {
    WAITING: 0,
    FETCHING :1
};

const NM_DBUS_NAME = "org.freedesktop.NetworkManager";
const NM_DBUS_PATH = "/org/freedesktop/NetworkManager";
const NMInterface = {
    name: NM_DBUS_NAME,
    methods: [],
    signals: [
        { name: 'StateChanged', inSignature: '', outSignature: 'u' }
	],
    properties: [
	{ name: 'State', signature: 'u', access: 'read' }

    ]
};

const NMProxy = DBus.makeProxyClass(NMInterface);

String.prototype.format = Format.format;

function w1temp() {
    this._init();
}

w1temp.prototype = {
    __proto__: PanelMenu.Button.prototype,
    _w1tempInfo: null,
    _url: null,
    _burl: null,
    _delay: null,
    _logme: null,
    _settings: null,
    _key: null,
    _nm_state: NM_state.NM_STATE_CONNECTED_GLOBAL,
    _w1_state: W1_state.WAITING,
    _tid: 0,
    _session: null,
    _message: null,
    _logfile: null,
    _use_nm: true,
    _mtxt: null,

    _mrefresh: function() {
	let here=this;

        here._log_msg("Refresh "+ here._nm_state);
	if (here._tid > 0)
        {
            Mainloop.source_remove(here._tid);
            here._tid = 0;
        }
        if(here._w1_state == W1_state.FETCHING)
        {
	    here._session.cancel_message(here._message, 503);
            here._w1_state = W1_state.WAITING;
	}
        else if (here._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL)
        {
	    here._getW1tempInfo();
        }
    },

    _mload: function() {
	let here=this;
	Util.spawn(["xdg-open", here._burl]);
    },

    _addmopts: function() {
	let here=this;
	here.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
        let item = new PopupMenu.PopupMenuItem("Refresh");
        item.connect('activate', function () { here._mrefresh();});
        here.menu.addMenuItem(item);
	item = new PopupMenu.PopupMenuItem("Load in browser ...");
        item.connect('activate', function () { here._mload();});
        here.menu.addMenuItem(item);
    },

    _init: function() {
	this._getsettings();
	if (this._use_nm == true)
	{
	    this._nm_init();
	}

       	this._getlog();
	this._log_msg("Started");

	this._w1tempIcon = new St.Icon({
	    icon_type: St.IconType.FULLCOLOR,
            icon_name: 'view-refresh-symbolic',
            style_class: 'w1temp-status-icon'
            });
	
        this._w1tempInfo = new St.Label({ 
	text: _('...'),
            style_class: 'w1temp-status-text'});
        PanelMenu.Button.prototype._init.call(this, 0.25);

        let topBox = new St.BoxLayout();
        topBox.add_actor(this._w1tempIcon);
        topBox.add_actor(this._w1tempInfo);
        this.actor.add_actor(topBox);
	
	if(this._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL )
	{
	    this._getW1tempInfo();
	}

        Main.panel._menus.addMenu(this.menu);
	this._addmopts();
    },

    _getlog: function(){
	if(this._logfile == null)
	{
	    let u = GLib.getenv("USER");
	    let d = GLib.getenv("DISPLAY");
	    let n = d.indexOf(":");
	    let dn = d.substring(n+1, d.length);
	    this._logfile="/tmp/w1temp-"+u+"-"+dn;
	}
    },
    
    _log_msg:function(s){
	if(this._logme == true)
	{
	    let f = Gio.file_new_for_path(this._logfile);
	    let data_out = new Gio.DataOutputStream(
		{base_stream:f.append_to(Gio.FileCreateFlags.NONE, null) });
	    let dt = new GLib.DateTime().format("%FT%H:%M:%S%z");
	    data_out.put_string(dt+" "+s+"\n", null);
	    data_out.close(null);
	}
    },

    _getsettings: function(){
	let here=this;
	if(this._settings == null)
	{
	    this._settings = new Gio.Settings({ schema: 'org.gnome.shell.extensions.w1retap' });
	    this._settings_id = this._settings.connect ('changed', function() { 
		here._getsettings(); } );
	}
	this._url = this._settings.get_string ("url");
	let k = this._settings.get_string ("key");
	this._key = k.toLowerCase();
	this._burl = this._settings.get_string("browse-url");
	this._delay = this._settings.get_int("delay");
	this._logme = this._settings.get_boolean ("log-errors");
	this._use_nm = this._settings.get_boolean ("use-nm");
    },

    // retrieve the w1temp data 
    _loadw1JSON: function(url, callback) {
	let here = this;
        here._log_msg("Getting  "+ url );
        here._session = new Soup.SessionAsync();
        here._message = Soup.Message.new('GET', url);
	here._w1_state = W1_state.FETCHING;
        here._session.queue_message(here._message, function(sess, msg) {
	    here._log_msg("Fetch "+ msg.status_code + " " + msg.reason_phrase);
	    if (msg.status_code == 200)
	    {
		let jObj = JSON.parse(msg.response_body.data);
		callback.call(here, jObj); 

	    }

	    here._log_msg("Status now "+ here._nm_state);
	    if(here._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL )
	    {
		here._tid = Mainloop.timeout_add_seconds(here._delay, function() {
		    here._getW1tempInfo();
		});
	    }
	});
    },

    // retrieve a w1temp icon image
    _getIconImage: function(temp) {
        var file_name;
	let atemp = Math.round(temp);
	if(atemp >=  0)
	{
	   file_name = "w1_thermo_%04d".format(atemp);
	}
	else
	{
	   atemp=Math.abs(atemp);
           file_name = "w1_thermo_-%03d".format(atemp);	 
        }
	return file_name;
    },

    _getW1tempInfo: function() {
	let here=this;
        this._loadw1JSON(this._url, function(w1data) {
	    here._w1_state = W1_state.WAITING;
            here._w1data = w1data;
	    let d = 0;
	    var txt = "";
	    for(var i =0; i < this._w1data.length; i++)
	    {
		let wx = this._w1data[i];
		if (i > 0) { txt += "\n"};
		txt += wx.name+": "+wx.value+" "+wx.units;
		if(wx.name.toLowerCase() == here._key)
		{
		    d = wx.value;
		}
	    }
	    here._w1tempInfo.text = d + "\u2103";
	    here._w1tempIcon.icon_name =  here._getIconImage(d);
            here._log_msg(">> "+ txt);

	    if(here._mtxt != null)
	    {
		here._mtxt.destroy();
	    }
            here.menu.removeAll();
	    here.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
	    here._mtxt = new St.Label({ text: txt, style_class: 'w1temp-menu-text'});
	    here.menu.addActor(here._mtxt);
	    here._addmopts();
        });
    },

    _kill_all: function() {
	let here=this;
	if(here._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL )
	{
	    if(here._w1_state == W1_state.FETCHING)
	    {
		here._session.cancel_message(here._message, 503);
	    }
	}
	if (here._tid > 0)
	{
	    Mainloop.source_remove(here._tid);
	}
	this._dbus.destroy();
    },

    _nm_init: function() {
	let here=this;
	here._dbus = new NMProxy(DBus.system, NM_DBUS_NAME, NM_DBUS_PATH);
	here._dbus.connect('StateChanged', function(dbus,value) {
	    here._log_msg("NM state changed from " + here._nm_state + " to " + value);
	    here._nm_state = value;
	    if (here._tid > 0)
	    {
		Mainloop.source_remove(here._tid);
		here._tid = 0;
	    }
	    if(here._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL )
	    {
		if(here._w1_state == W1_state.FETCHING)
		{
		    here._session.cancel_message(here._message, 503);
		    here._w1_state = W1_state.WAITING;
		}
		here._getW1tempInfo();
	    }
	});
	here._dbus.GetRemote('State', function(value,exception) {
	    if(exception)
	    {
		log(exception);
	    }
	    else
	    {
		here._nm_state = value;
		here._log_msg("NM initial " + value);
		if(here._nm_state == NM_state.NM_STATE_CONNECTED_GLOBAL )
		{
		    here._getW1tempInfo();
		}
	    }
	});
    },
};

let w1;

function init() {
}

function enable() {
    w1 = new w1temp();
    Main.panel.addToStatusArea('w1temp', w1);
}

function disable() {
    w1._kill_all();
    w1.destroy();
}
