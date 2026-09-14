all: ../../bin/doscat ../../bin/hexcom

../../bin/doscat: doscat.c
	$${CC:-$(CC)} -o ../../bin/doscat doscat.c

../../bin/hexcom: hexcom.c
	$${CC:-$(CC)} -o ../../bin/hexcom hexcom.c

clean:
	rm -f *.o
	rm -f ../../bin/doscat
	rm -f ../../bin/hexcom
