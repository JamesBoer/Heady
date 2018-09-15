/*
The Heady library is distributed under the MIT License (MIT)
https://opensource.org/licenses/MIT
See LICENSE.TXT or Heady.h for license details.
Copyright (c) 2018 James Boer
*/

#include <iostream>
#include <thread>
#include <cstring>
#include "Basic.h"


int main(int argc, char ** argv)
{

	try
	{
		Heady::Params params;
		Heady::GenerateHeader(params);
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error processing source files.  " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

