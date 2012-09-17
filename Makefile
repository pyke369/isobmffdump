isobmffdump: isobmffdump.cpp
	g++ -O2 -Wall -s -o isobmffdump isobmffdump.cpp

deb:
	@debuild -i -us -uc -b

debclean:
	@debuild clean

clean:

distclean:
	rm -f isobmffdump
