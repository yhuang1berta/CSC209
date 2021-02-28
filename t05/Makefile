SHELL = /bin/bash
FLAGS= -Wall -Werror -fsanitize=address -std=c99
OBJ = helpers.o
DEPENDENCIES = helpers.h

.PHONY: basic_test clean

basic_test : serialize deserialize 
	@s=test.dat > ./serialize; \
	s_r=$$?;\
	d=test.dat > ./serialize > ./deserialize;\
	d_r=$$?;\
	if [ "$$s_r" == 0 ] && [ "$$d_r" == 0 ]; then \
		echo "Basic test passed";\
	else\
		echo "Basic test failed";\
	fi

serialize : serialize.o ${OBJ}
	gcc ${FLAGS} -o $@ $^

deserialize : deserialize.o ${OBJ}
	gcc ${FLAGS} -o $@ $^

%.o : %.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

clean : 
	rm -f *.o serialize deserialize