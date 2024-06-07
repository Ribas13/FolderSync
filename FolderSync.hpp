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

class FolderSync {
	private:
		std::string		src;
		std::string		replica;
		std::ofstream	logfile;
		int				interval;
		std::atomic<bool> stopFlag;
		std::thread		inputThread;
		std::map<std::string, std::filesystem::file_time_type> files_last_write_time;
		std::set<std::string> dirs_last_state; //not sure if I'll keep this
		void checkDirectoryExists(std::string path);
		void checkLogFileAndCreate(const std::string path);
		void syncFolders(void);
		void log(const std::string &str);
		void monitorInput(void);
		bool hasFolderContentChanged(const std::filesystem::path& path);
		void updateReplica(const std::filesystem::path& path);
		std::string trimPath(std::string);


	public:
		FolderSync(char **av);
		~FolderSync();

		void evalLoop(void);
};

#endif