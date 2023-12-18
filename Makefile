CC = gcc
VPATH = src

media-sort: process-dir.o exif-util.o util.o process-file.o runner.o
	$(CC) $? -o $@

process-dir.o: process-dir.c
	$(CC) -c $? -o $@
    
exif-util.o: exif-util.c
	$(CC) -c $? -o $@

util.o: util.c
	$(CC) -c $? -o $@

process-file.o: process-file.c
	$(CC) -c $? -o $@

runner.o: runner.c
	$(CC) -c $? -o $@
