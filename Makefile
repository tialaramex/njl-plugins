
CFLAGS = -Wall -O2 -fPIC

LDFLAGS = -shared -Xlinker -Bdynamic -lm

PLUGINS= noise_1921.so noise_1922.so eir_1923.so risset_1924.so

all: $(PLUGINS)
	@echo
	@echo "Now copy plugins into your LADSPA plugin directory"
	@echo "e.g. /usr/local/lib/ladspa/"
	@echo

clean:
	$(RM) *.o $(PLUGINS)

%.so: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

