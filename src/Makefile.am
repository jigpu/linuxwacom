SUBDIRS = @WCM_SRC_SUBDIRS@

wmanpagedir = @mandir@/man4
xsetwacommandir = @mandir@/man4
wacommanpagedir = @mandir@/man4

wmanpage_HEADERS = wacom.4x.gz
xsetwacomman_HEADERS = xsetwacom.4x.gz
wacommanpage_HEADERS = $(wmanpage_HEADERS) $(xsetwacomman_HEADERS)

wmanpage_DATA = wacom.4x
xsetwacomman_DATA = xsetwacom.4x
EXTRA_DIST = $(wmanpage_DATA) $(xsetwacomman_DATA)

$(wacommanpage_HEADERS): $(EXTRA_DIST)
	rm -f $(wacommanpage_HEADERS)
	gzip -9c < $(srcdir)/$(wmanpage_DATA) > $(wmanpage_HEADERS)
	gzip -9c < $(srcdir)/$(xsetwacomman_DATA) > $(xsetwacomman_HEADERS)
