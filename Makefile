export VERSION=r`svn info | grep Revision | cut -d ' '-f 2`
export DATE=`svn info | grep 'Last Changed Date' | cut -d ' ' -f 4`

ifndef PREFIX
	PREFIX = /usr/local
endif

ifndef WWWROOT
	WWWROOT = /var/www/localhost/htdocs
endif

ifdef STATIC
	STATIC = -static
endif

ifdef STRIP
	STRIP = true
else
	STRIP = false
endif
DOSTRIP=strip --strip-unneeded --strip-debug

all: lib include

install: lib include
	mkdir -p ${PREFIX}/include/fastcgi++
	install -o 0 -g 0 -m 644 include/fastcgi++/*.hpp ${PREFIX}/include/fastcgi++
	install -o 0 -g 0 -m 644 lib/libfastcgipp.a ${PREFIX}/lib/libfastcgipp.a
	install -o 0 -g 0 -m 755 lib/libfastcgipp.so ${PREFIX}/lib/libfastcgipp.so

examples-install: examples
	mkdir -p ${WWWROOT}/fastcgipp
	install -o 0 -g 0 -m 755 examples/*.fcgi ${WWWROOT}/fastcgipp
	install -o 0 -g 0 -m 644 examples/*.html ${WWWROOT}/fastcgipp
	install -o 0 -g 0 -m 644 examples/*.png ${WWWROOT}/fastcgipp

clean: libclean examplesclean

uninstall:
	rm -f ${PREFIX}/lib/libfastcgipp.a ${PREFIX}/lib/libfastcgipp.so
	rm -f ${PREFIX}/include/fastcgi++/*.hpp
	rm -f ${PREFIX}/share/doc/fastcgi++/*
	rm -f ${WWWROOT}/fastcgipp/*
	rmdir ${WWWROOT}/fastcgipp; true
	rmdir ${PREFIX}/include/fastcgi++; true
	rmdir ${PREFIX}/share/doc/fastcgi++; true

doc: include doxygen lib/src/*.cpp doc.hpp
	VERSION=trunk.r`svn info | grep Revision | cut -d ' ' -f 2` DATE=`svn info | grep 'Last Changed Date' | cut -d ' ' -f 4` doxygen doxygen

doc-install:
	mkdir ${PREFIX}/share/doc/fastcgi++
	install -o 0 -g 0 -m 644 doc/* ${PREFIX}/share/doc/fastcgi++

docclean:
	rm -rf doc



include: include/fastcgi++/exceptions.hpp include/fastcgi++/fcgistream.hpp include/fastcgi++/http.hpp include/fastcgi++/manager.hpp include/fastcgi++/protocol.hpp include/fastcgi++/request.hpp include/fastcgi++/transceiver.hpp

include/fastcgi++/exceptions.hpp: 

include/fastcgi++/fcgistream.hpp: include/fastcgi++/protocol.hpp

include/fastcgi++/http.hpp: include/fastcgi++/exceptions.hpp include/fastcgi++/protocol.hpp

include/fastcgi++/manager.hpp: include/fastcgi++/exceptions.hpp include/fastcgi++/protocol.hpp include/fastcgi++/transceiver.hpp

include/fastcgi++/protocol.hpp:

include/fastcgi++/request.hpp: include/fastcgi++/protocol.hpp include/fastcgi++/exceptions.hpp include/fastcgi++/transceiver.hpp include/fastcgi++/fcgistream.hpp include/fastcgi++/http.hpp

include/fastcgi++/transceiver.hpp: include/fastcgi++/protocol.hpp include/fastcgi++/exceptions.hpp



examples: examples/utf8-helloworld.fcgi examples/showgnu.fcgi examples/echo.fcgi examples/timer.fcgi examples/upload.fcgi

examplesclean:
	rm -f examples/*.fcgi

examples/echo.fcgi: examples/src/echo.cpp include/fastcgi++/request.hpp include/fastcgi++/manager.hpp
	g++ ${CXXFLAGS} ${STATIC} -o examples/echo.fcgi examples/src/echo.cpp -Llib -Iinclude -lfastcgipp -lboost_thread -pthread
ifeq ($(STRIP),true)
	strip -s examples/echo.fcgi
endif

examples/utf8-helloworld.fcgi: examples/src/utf8-helloworld.cpp include/fastcgi++/request.hpp include/fastcgi++/manager.hpp
	g++ ${CXXFLAGS} ${STATIC} -o examples/utf8-helloworld.fcgi examples/src/utf8-helloworld.cpp -Llib -Iinclude -lfastcgipp -lboost_thread -pthread
ifeq ($(STRIP),true)
	strip -s examples/utf8-helloworld.fcgi
endif

examples/showgnu.fcgi: examples/src/showgnu.cpp include/fastcgi++/request.hpp include/fastcgi++/manager.hpp
	g++ ${CXXFLAGS} ${STATIC} -o examples/showgnu.fcgi examples/src/showgnu.cpp -Llib -Iinclude -lfastcgipp -lboost_thread -pthread
ifeq ($(STRIP),true)
	strip -s examples/showgnu.fcgi
endif

examples/timer.fcgi: examples/src/timer.cpp include/fastcgi++/request.hpp include/fastcgi++/manager.hpp
	g++ ${CXXFLAGS} ${STATIC} -o examples/timer.fcgi examples/src/timer.cpp -Llib -Iinclude -lfastcgipp -lboost_thread -lboost_system -pthread
ifeq ($(STRIP),true)
	strip -s examples/timer.fcgi
endif

examples/upload.fcgi: examples/src/upload.cpp include/fastcgi++/request.hpp include/fastcgi++/manager.hpp
	g++ ${CXXFLAGS} ${STATIC} -o examples/upload.fcgi examples/src/upload.cpp -Llib -Iinclude -lfastcgipp -lboost_thread -pthread
ifeq ($(STRIP),true)
	strip -s examples/upload.fcgi
endif


lib: lib/libfastcgipp.a lib/libfastcgipp.so

libclean:
	rm -f lib/*.so lib/*.a lib/*.o

lib/libfastcgipp.a: lib/http-static.o lib/protocol-static.o lib/request-static.o lib/transceiver-static.o lib/exceptions-static.o
	ar rc lib/libfastcgipp.a lib/http-static.o lib/protocol-static.o lib/request-static.o lib/transceiver-static.o lib/exceptions-static.o
	ranlib lib/libfastcgipp.a
ifeq ($(STRIP),true)
	 strip --strip-unneeded --strip-debug lib/libfastcgipp.a
endif

lib/libfastcgipp.so: lib/http-shared.o lib/protocol-shared.o lib/request-shared.o lib/transceiver-shared.o lib/exceptions-shared.o
	g++ ${CXXFLAGS} -shared -o lib/libfastcgipp.so lib/http-shared.o lib/protocol-shared.o lib/request-shared.o lib/transceiver-shared.o lib/exceptions-shared.o -Iinclude
ifeq ($(STRIP),true)
	strip -s lib/libfastcgipp.so
endif

lib/http-static.o: lib/src/http.cpp include/fastcgi++/http.hpp
	g++ ${CXXFLAGS} -o lib/http-static.o -c lib/src/http.cpp -Iinclude

lib/http-shared.o: lib/src/http.cpp include/fastcgi++/http.hpp
	g++ ${CXXFLAGS} -fPIC -o lib/http-shared.o -c lib/src/http.cpp -Iinclude

lib/protocol-static.o: lib/src/protocol.cpp include/fastcgi++/protocol.hpp
	g++ ${CXXFLAGS} -o lib/protocol-static.o -c lib/src/protocol.cpp -Iinclude

lib/protocol-shared.o: lib/src/protocol.cpp include/fastcgi++/protocol.hpp
	g++ ${CXXFLAGS} -fPIC -o lib/protocol-shared.o -c lib/src/protocol.cpp -Iinclude

lib/request-static.o: lib/src/request.cpp include/fastcgi++/request.hpp
	g++ ${CXXFLAGS} -o lib/request-static.o -c lib/src/request.cpp -Iinclude

lib/request-shared.o: lib/src/request.cpp include/fastcgi++/request.hpp
	g++ ${CXXFLAGS} -fPIC -o lib/request-shared.o -c lib/src/request.cpp -Iinclude

lib/transceiver-static.o: lib/src/transceiver.cpp include/fastcgi++/transceiver.hpp
	g++ ${CXXFLAGS} -o lib/transceiver-static.o -c lib/src/transceiver.cpp -Iinclude

lib/transceiver-shared.o: lib/src/transceiver.cpp include/fastcgi++/transceiver.hpp
	g++ ${CXXFLAGS} -fPIC -o lib/transceiver-shared.o -c lib/src/transceiver.cpp -Iinclude

lib/exceptions-static.o: lib/src/exceptions.cpp include/fastcgi++/exceptions.hpp
	g++ ${CXXFLAGS} -o lib/exceptions-static.o -c lib/src/exceptions.cpp -Iinclude

lib/exceptions-shared.o: lib/src/exceptions.cpp include/fastcgi++/exceptions.hpp
	g++ ${CXXFLAGS} -fPIC -o lib/exceptions-shared.o -c lib/src/exceptions.cpp -Iinclude
