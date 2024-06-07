#include "FolderSync.hpp"
#include <iostream>


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