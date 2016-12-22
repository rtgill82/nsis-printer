#
# Created:  Sat 13 Dec 2014 05:07:48 PM PST
# Modified: Wed 21 Dec 2016 05:51:16 PM PST
#
# Copyright 2016 (C) Robert Gill
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

NSIS_HEADER = nsis/include/Printer.nsh

all: README.rst
	$(MAKE) -C src

README.rst: $(NSIS_HEADER) README.rst.in
	awk '/^;;/{f=1;next}/^[^;]/{f=0}f {print substr($$0,3)}' < $< | \
		awk '/@API_DOCUMENTATION@/{ \
			while(getline line < "-"){ \
				print line \
			} next \
		} //' $(word 2,$^) > $@

clean:
	$(MAKE) -C src clean
	-rm -f README.aux README.dvi README.html README.log README.out \
		README.pdf README.tex

.PHONY: all clean
