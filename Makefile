#
# Created:  Sat 13 Dec 2014 05:07:48 PM PST
# Modified: Mon 06 Jun 2016 08:25:11 PM PDT
#

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

.PHONY: all clean
