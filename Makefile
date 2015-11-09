
all:
	$(MAKE) -C cyhostboot all
	$(MAKE) -C ihex2cyacd all

install: all
	$(MAKE) -C cyhostboot install
	$(MAKE) -C ihex2cyacd install

clean:
	$(MAKE) -C cyhostboot clean
	$(MAKE) -C ihex2cyacd clean
