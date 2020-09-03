IDIR=.
INCLUDE=include/
CC = cc
CFLAGS= -I$(INCLUDE)
DEPSC = 
DEPSS = 
OBJ = socks.o sysmethods.o
OBJC = mftp.o $(OBJ)
OBJS = mftpserve.o logcon.o $(OBJ)
OBJA = mftp.o mftpserve.o logcon.o $(OBJ)

all: client serv

client: $(OBJC)
	$(CC) -o mftp $(OBJC) $(CFLAGS)

serv: $(OBJS) $(INCLUDE)server.h
	$(CC) -o mftpserve $(OBJS) $(CFLAGS)

logcon.o: logcon.c $(INCLUDE)logcon.h
	$(CC) -c logcon.c $(CFLAGS)
	
socks.o: socks.c $(INCLUDE)socks.h
	$(CC) -c socks.c $(CFLAGS)
	
sysmethods.o: sysmethods.c $(INCLUDE)sysmethods.h
	$(CC) -c sysmethods.c $(CFLAGS)
	
mftpserve.o: mftpserve.c $(DEPSS)
	$(CC) -c mftpserve.c $(CFLAGS)

mftp.o: mftp.c $(DEPSC)
	$(CC) -c mftp.c $(CFLAGS)

clean:
	-@rm $(OBJA) mftp mftpserve 
	
rc: 
	-@./mftp 49999 localhost

rs:
	-@./mftpserve
