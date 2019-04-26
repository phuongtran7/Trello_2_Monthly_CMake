// CMake_Trello2Monthly.cpp : Defines the entry point for the application.
//

#include "CMake_Trello2Monthly.h"
#include "monthly.h"

int main(int argc, char* argv[])
{
	monthly new_month;
	new_month.run();
	new_month.shutdown();
	std::getchar();
	return 0;
}