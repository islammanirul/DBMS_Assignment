a.out : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o
	cc am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o 

amlayer.o : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o amscan.o amprint.o
	ld -r am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o amscan.o amprint.o  -o amlayer.o

am.o : am.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c am.c

amfns.o : amfns.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amfns.c

amsearch.o : amsearch.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amsearch.c

aminsert.o : aminsert.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c aminsert.c

amscan.o : amscan.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amscan.c

amstack.o : amstack.c am.h pf.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amstack.c

amglobals.o : amglobals.c am.h
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amglobals.c

amprint.o : amprint.c am.h pf.h 
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c amprint.c
	
main.o : main.c am.h pf.h 
	cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-deprecated-non-prototype -c main.c


