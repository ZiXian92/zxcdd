#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main(){
	int res, fd;
	char c;

	fd = open("/dev/zxcdd", O_RDWR);
	if(fd<0){
		perror("Failed to open device.");
		return errno;
	}

	// Normal read
	printf("Doing normal read.\n");
	res = pread(fd, &c, 1, 0);
	if(res>=0){
		printf("%d bytes read.\n", res);
		if(res) printf("Data read is %c.\n", c);
	} else perror("Something went wrong reading normally.");

	// NULL pointer read
	printf("Attempt to read to NULL pointer.\n");
	res = pread(fd, NULL, 1, 0);
	if(res<0){ perror("Reading to NULL buffer."); }
	else printf("Read to NULL buffer should give an error...\n");

	// Offset out of range
	printf("Attempt to read from outside the 1 byte.\n");
	res = pread(fd, &c, 1, 1);
	if(res<0){ perror("Reading to NULL buffer."); }
	else printf("Read outside boundary should give an error...\n");

	// Test read multiple bytes
	printf("Attempt to read multiple bytes from 1-byte device.\n");
	c = 'A';
	res = pread(fd, &c, 2, 0);
	if(res>=0){
		printf("%d bytes read.\n", res);
		printf("Data read is %c.\n", c);
	} else perror("Something went wrong with reading multiple bytes.");

	// Normal write
	c = 'A';
	res = pwrite(fd, &c, 1, 0);
	if(res>=0){
		printf("%d bytes written.\n", res);
		res = pread(fd, &c, 1, 0);
		if(res>0) printf("Device data is now %c.\n", c);
	} else perror("Somethign went wrong with normal write.");

	// Multibyte write
	// Error but 1st byte is written
	res = pwrite(fd, "MNO", 3, 0);
	if(res<0){
		perror("Writing too many bytes to 1-byte device.");
		res = pread(fd, &c, 1, 0);
		if(res>0) printf("Device data is now %c.\n", c);
	} else printf("Writing many bytes should write 1st byte and give error...\n");

	// Write from NULL
	res = pwrite(fd, NULL, 1, 0);
	if(res<0){
		perror("Writing from NULL.");
		res = pread(fd, &c, 1, 0);
		if(res>0) printf("Device data is stil %c.\n", c);
	} else printf("Writing from NULL should give an error...\n");

	// Write beyond boundary
	c = 'K';
	res = pwrite(fd, &c, 1, 1);
	if(res<0){
		perror("Writing beyond device boundary.");
		res = pread(fd, &c, 1, 0);
		if(res>0) printf("Device data is stil %c.\n", c);
	} else printf("Writing beyond boundary should give error...\n");

	close(fd);
	return 0;
}
