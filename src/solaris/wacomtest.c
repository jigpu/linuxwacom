
#include <sys/stream.h>
#include <sys/stropts.h>

#include "../include/wcmSolaris.h"

int
main(int argc, char *argv[])
{
	int fd;
	char buf[512];
	int n;
	struct input_event ie;

	fd = open("/devices/pci@0,0/pci17aa,200a@1d,2/mouse@1:mouse", 2);
	if (fd < 0) {
		perror("open failed");
		exit(1);
	}

	if (ioctl(fd, I_PUSH, "wacom") < 0) {
		perror("ioctl failed");
		exit(2);
	}

	while ((n = read(fd, &ie, sizeof(ie))) > 0) {
		printf("type = %x, code = %x, value = %x\n",
		       ie.type, ie.code, ie.value);
	}
}
