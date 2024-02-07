# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy hostinfo echoclient echoserver echoserverp select echoservers echoservert sharing badcnt goodcnt echoserver-pre psum-mutex race

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c
proxy: proxy.o cache.o sbuf.o csapp.o
	$(CC) $(CFLAGS) proxy.o cache.o sbuf.o csapp.o -o proxy $(LDFLAGS)


hostinfo.o: hostinfo.c
	$(CC) $(CFLAGS) -c hostinfo.c
hostinfo: csapp.o hostinfo.o
	$(CC) $(CFLAGS) csapp.o hostinfo.o -o hostinfo $(LDFLAGS)


echoclient.o: echoclient.c
	$(CC) $(CFLAGS) -c echoclient.c
echoclient: csapp.o echoclient.o
	$(CC) $(CFLAGS) csapp.o echoclient.o -o echoclient $(LDFLAGS)


echoserver.o: echoserver.c
	$(CC) $(CFLAGS) -c echoserver.c
echoserver: csapp.o echoserver.o
	$(CC) $(CFLAGS) csapp.o echoserver.o -o echoserver $(LDFLAGS)


echoserverp.o: echoserverp.c
	$(CC) $(CFLAGS) -c echoserverp.c
echoserverp: csapp.o echoserverp.o
	$(CC) $(CFLAGS) csapp.o echoserverp.o -o echoserverp $(LDFLAGS)


select.o: select.c
	$(CC) $(CFLAGS) -c select.c
select: csapp.o select.o
	$(CC) $(CFLAGS) csapp.o select.o -o select $(LDFLAGS)


echoservers.o: echoservers.c
	$(CC) $(CFLAGS) -c echoservers.c
echoservers: csapp.o echoservers.o
	$(CC) $(CFLAGS) csapp.o echoservers.o -o echoservers $(LDFLAGS)


echoservert.o: echoservert.c
	$(CC) $(CFLAGS) -c echoservert.c
echoservert: csapp.o echoservert.o
	$(CC) $(CFLAGS) csapp.o echoservert.o -o echoservert $(LDFLAGS)


sharing.o: sharing.c
	$(CC) $(CFLAGS) -c sharing.c
sharing: csapp.o sharing.o
	$(CC) $(CFLAGS) csapp.o sharing.o -o sharing $(LDFLAGS)


badcnt.o: badcnt.c
	$(CC) $(CFLAGS) -c badcnt.c
badcnt: csapp.o badcnt.o
	$(CC) $(CFLAGS) csapp.o badcnt.o -o badcnt $(LDFLAGS)


goodcnt.o: goodcnt.c
	$(CC) $(CFLAGS) -c goodcnt.c
goodcnt: csapp.o goodcnt.o
	$(CC) $(CFLAGS) csapp.o goodcnt.o -o goodcnt $(LDFLAGS)


echoserver-pre.o: echoserver-pre.c
	$(CC) $(CFLAGS) -c echoserver-pre.c
echoserver-pre: csapp.o sbuf.o echoserver-pre.o
	$(CC) $(CFLAGS) csapp.o sbuf.o echoserver-pre.o -o echoserver-pre $(LDFLAGS)


psum-mutex.o: psum-mutex.c
	$(CC) $(CFLAGS) -c psum-mutex.c
psum-mutex: csapp.o psum-mutex.o
	$(CC) $(CFLAGS) csapp.o psum-mutex.o -o psum-mutex $(LDFLAGS)


race.o: race.c
	$(CC) $(CFLAGS) -c race.c
race: csapp.o race.o
	$(CC) $(CFLAGS) csapp.o race.o -o race $(LDFLAGS)



# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

