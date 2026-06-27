#include "functions.h"
#include "config.h"

int main(void) {
	setSysClock();
	setPinConf();
	while(1) {
		cycle();
	}
}