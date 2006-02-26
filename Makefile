
ALL= noise_1921.so noise_1922.so eir_1923.so risset_1924.so

all: $(ALL)
	@echo
	@echo "Now copy plugins into your LADSPA plugin directory"
	@echo "e.g. /usr/local/lib/ladspa/"
	@echo

clean:
	rm $(ALL)

noise_1921.so: noise_1921.c
	gcc -Wall -shared -Xlinker -Bdynamic -O2 -fPIC -o noise_1921.so noise_1921.c -lm

noise_1922.so: noise_1922.c
	gcc -Wall -shared -Xlinker -Bdynamic -O2 -fPIC -o noise_1922.so noise_1922.c -lm

eir_1923.so: eir_1923.c
	gcc -Wall -shared -Xlinker -Bdynamic -O2 -fPIC -o eir_1923.so eir_1923.c -lm

risset_1924.so: risset_1924.c
	gcc -Wall -shared -Xlinker -Bdynamic -O2 -fPIC -o risset_1924.so risset_1924.c -lm
