// #define DEBUG

#ifndef DEBUG
#include "main.hpp"

int main(void) {
	start();
	return 0;
}

#else
#include"test/self_test.hpp"
int main(void) {
	self::SelfTest::qrcodeTest();
	return 0;
}
#endif
