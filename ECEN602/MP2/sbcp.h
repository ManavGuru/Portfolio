#include <stdio.h>

#define MAXDATASIZE 512

struct sbcp_attribute{
	unsigned int type :16;
	unsigned int length :16;
	char payload[MAXDATASIZE];
};

struct sbcp_message{
	unsigned int version :9;
	unsigned int type :7;
	unsigned int length :16;
	struct sbcp_attribute attribute;
};
