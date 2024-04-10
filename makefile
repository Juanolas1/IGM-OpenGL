todo: spinningcube

LDLIBS=-lGL -lGLEW -lglfw -lm

clean:
	rm -f *.o *~

cleanall: clean
	rm -f spinningcube