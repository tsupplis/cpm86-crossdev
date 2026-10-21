all: ../../bin/drpli_objfix

.PHONY: all test clean
test: all
	python3 test_objfix.py

../../bin/drpli_objfix: drpli_objfix.c
	$${CC:-$(CC)} -o ../../bin/drpli_objfix drpli_objfix.c

clean:
	rm -f *.o
	rm -f ../../bin/drpli_objfix
