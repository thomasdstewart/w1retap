all: w1temp

w1temp : w1temp.vala
	valac --pkg libpanelapplet-2.0 --pkg gtk+-2.0 --pkg libsoup-2.4 --pkg json-glib-1.0 --thread w1temp.vala

clean:
	rm -f w1temp *~

install: w1temp
	install -d /usr/share/w1temp/ui /usr/share/w1temp/pixmaps
	install ../share/w1temp/ui/* /usr/share/w1temp/ui/
	install ../share/w1temp/pixmaps/* /usr/share/w1temp/pixmaps/
	install -s w1temp /usr/libexec
	install ../share/w1temp/schema/org.w1retap.w1temp.gschema.xml /usr/share/glib-2.0/schemas/
	glib-compile-schemas /usr/share/glib-2.0/schemas/

dist:   clean
	tar --exclude=\*.tar.xz -cf w1temp-gnome2.tar.xz -C ../.. applets/w1temp-gnome2 applets/share


