#ifndef FOLDERSYNC_HPP
#define FOLDERSYNC_HPP

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <atomic>
#include <map>
#include <set>
#include <vector>

class FolderSync {
	private:
		std::string		src;
		std::string		replica;
		std::ofstream	logfile;
		int				interval;
		std::atomic<bool> stopFlag;
		std::thread		inputThread;
		std::map<std::string, std::filesystem::file_time_type> files_last_write_time;
		std::set<std::string> dirs_last_state;
		std::set<std::string> current_dirs;
		void checkDirectoryExists(std::string path);
		void checkLogFileAndCreate(const std::string path);
		void syncFolders(void);
		void log(const std::string &str);
		void monitorInput(void);
		bool hasFolderContentChanged(void);
		std::string trimPath(std::string);


	public:
		FolderSync(char **av);
		~FolderSync();

		void evalLoop(void);
};

#endif