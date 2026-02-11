DEBUG = -O0 -g -fsanitize=address
REDIST = redist

subs:
	git submodule update --init --recursive
	git submodule foreach git pull origin main
	(cd $(REDIST)/vgfx && make subs && make build)

build: main.c
	cc main.c -o game -L./$(REDIST)/vgfx/lib -lvgfx -lglfw -lm $(DEBUG)

run: build
	./game
