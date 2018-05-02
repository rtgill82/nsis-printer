#
# Created:  Sat 13 Dec 2014 05:07:48 PM PST
# Modified: Tue 01 May 2018 06:02:06 PM PDT
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

PACKAGE = nsisprinter
PACKAGE_VERSION = 1.1.0

DISTFILE = $(PACKAGE)-$(PACKAGE_VERSION).zip
DISTDIR = ./$(PACKAGE)-$(PACKAGE_VERSION)
NSIS_HEADER = nsis/include/Printer.nsh

all: readme
	$(MAKE) -C src

readme: README.rst

README.rst: $(NSIS_HEADER) README.rst.in
	awk '/^;;/{f=1;next}/^[^;]/{f=0}f {print substr($$0,3)}' < $< | \
		awk '/@API_DOCUMENTATION@/{ \
			while(getline line < "-"){ \
				print line \
			} next \
		} //' $(word 2,$^) > $@

dist: all
	-rm -rf ./VS2015/.vs
	-rm -rf ./VS2015/Debug*
	-rm -rf ./VS2015/Release*
	-rm -f ./VS2015/nsisprinter.VC.*
	-rm -f ./VS2015/nsisprinter.vcxproj.user
	-rm -f ./VS2015/nsisprinter.{opensdf,sdf}
	-rm -f ./VS2015/nsisprinter.v12.suo
	mkdir -p $(DISTDIR)
	find . -not -name '.' \
		-not -wholename './.git*' \
		-not -wholename './src/.obj*' \
		-not -wholename '$(DISTDIR)*' \
		-type d -exec mkdir $(DISTDIR)/{} \;
	find . -not -name '.' \
		-not -wholename './.git*' \
		-not -wholename './src/.obj*' \
		-not -wholename '$(DISTDIR)*' \
		-type f -exec cp {} $(DISTDIR)/{} \;
	7z -tzip -mx=9 a $(DISTFILE) $(DISTDIR)
	-rm -rf $(DISTDIR)

clean:
	$(MAKE) -C src clean
	-rm -f README.{aux,dvi,html,log,out,pdf,tex}
	-rm -f $(DISTFILE)

.PHONY: all clean
