/*
 * Copyright 2018, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Core.h"

int main()
{
	for (int i = 0; i < 10; i++) {
		int iterations = 200;
		bool fail = false;
		int first = Core::RandomNumber(0, 10);
		int last = Core::RandomNumber(first, 10);
		std::cout << "Number between " << first << " and " << last << ": ";
		while (iterations-- > 0) {
			int num = Core::RandomNumber(first, last);
			if (num < first || num > last)
				fail = true;
		}
		if (fail)
			std::cout << "Test failed!" << std::endl;
		else
			std::cout << "Test passed!" << std::endl;
	}
}
