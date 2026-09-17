all: ../../bin/drpli_objfix

../../bin/drpli_objfix: drpli_objfix.c
	$${CC:-$(CC)} -o ../../bin/drpli_objfix drpli_objfix.c

clean:
	rm -f *.o
	rm -f ../../bin/drpli_objfix
