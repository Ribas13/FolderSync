#include "FolderSync.hpp"
#include <iostream>

/*

	1. verify arguments (folder paths, sync interval and log file)
		2. if src folder doesn't exist - throw exception
		3. if replica folder doesn't exist - create folder
		4. invalid sync interval (<0s) - throw exception
		5. if log file doesn't exist - create log file
	2.

*/



int main(int argc, char **argv) {
	if (argc != 5) {
		//error bad args
		std::cout << "Error: Incorrect number of arguments: ./folderSync <src folder> <replica folder> <sync interval(seconds)> <log file>" << std::endl;
		exit(1);
	}
	try {
		FolderSync folderSync(argv);
		folderSync.evalLoop();
	}
	catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}