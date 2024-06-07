#include "FolderSync.hpp"

namespace fs = std::filesystem;

FolderSync::FolderSync(char **av) {
	this->src = av[1];
	checkDirectoryExists(this->src);
	this->replica = av[2];
	checkDirectoryExists(this->replica);
	this->interval = atoi(av[3]);
	if (this->interval <= 0)
		throw std::runtime_error("Interval need to be bigger than 0");
	checkLogFileAndCreate(av[4]);
	logfile.open(av[4], std::ios_base::trunc);
	if (!logfile)
        throw std::runtime_error("Error opening log file: " + std::string(av[4]));
	this->stopFlag = false;
	this->inputThread = std::thread(&FolderSync::monitorInput, this);
}

FolderSync::~FolderSync() {
	
	// std::cout << "FolderSync destructor called" << std::endl;
	if (inputThread.joinable())
		inputThread.join();
	if (logfile.is_open())
		logfile.close();
}

void FolderSync::checkLogFileAndCreate(const std::string path) {
	if (!fs::exists(path)) {
		std::ofstream tempFile(path);
		if (!tempFile)
			throw std::runtime_error("Error: Could not create log file " + path);
		tempFile.close();
	}
}

void FolderSync::monitorInput(void) {
	std::string line;
	while (true) {
		std::getline(std::cin, line);
		if (line == "exit") {
			std::cout << "Are you sure? (yes/no) ";
			std::getline(std::cin, line);
			std::cout << std::endl;
			if (line == "yes") {
				stopFlag = true;
				log("Program ended by user");
				break;
			}
		}
		else if (line == "h") {
			std::cout << "'exit' + ENTER to quit program" << std::endl;
		}
		else {
			std::cout << "'h' + ENTER for options" << std::endl;
		}
	}
}

void FolderSync::checkDirectoryExists(std::string path) {
	if (!fs::exists(path))
		throw std::runtime_error("Path does not exist: " + path);
	else if (!fs::is_directory(path))
		throw std::runtime_error("Path is not a folder: " + path);
}

static bool dirChecks(std::string path) {
	if (!fs::exists(path)) {
		std::cout << "Directory no longer exists\n";
		std::cout << "\tWaiting for the directory to be restored on: " << path << std::endl;
		return false;
	}
	else if (!fs::is_directory(path)) {
		std::cout << path << "is no longer a directory\n";
		std::cout << "\tWaiting for the directory to be restored on: " << path << std::endl;
		return false;
	}
	return true;
}

void FolderSync::log(const std::string &str) {
	if (str == "") {
		std::cout << std::endl;
		logfile << std::endl;
		return ;
	}
	//Getting the time for the log
	auto now = std::chrono::system_clock::now();
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

	//Formatting the time for the log
	std::tm* now_tm = std::localtime(&now_time_t);
	std::ostringstream oss;
	oss << "[" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "] ";
	std::string time = oss.str();

	std::cout << time << str << std::endl;

	//writing to log file
	if (logfile.is_open())
		logfile << time << str << std::endl;
}

void FolderSync::syncFolders(void) {
	if (dirChecks(this->src)) {
		if (dirChecks(this->replica)) {
			if (hasFolderContentChanged(this->src))
				return ;
		}
	}
}

std::string FolderSync::trimPath(std::string str) {
	size_t pos = str.find('/');
	if (pos != std::string::npos) {
		return str.substr(pos + 1);
	}
	return str;
}

//files are synced but if you change a file in replica it's not being replaced
//because we are only checking changes in the source folder
bool FolderSync::hasFolderContentChanged(const fs::path& path) {
	//need to add if the file exists in the replica folder
	std::map<std::string, fs::file_time_type> current_files;
	std::set<std::string> current_dirs;
	bool hasChanged = false;

	//if the file is new, log file creation
	//else if there's changes to the src files overwrite the modified file to the replica dir
	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(path)) {
		if (entry.is_regular_file()) {
			fs::file_time_type last_write_time = fs::last_write_time(entry);
			current_files[entry.path().string()] = last_write_time;
			if (files_last_write_time.count(entry.path().string()) == 0) { //file is new
				/* fs::path dest = replica / entry.path().filename();
				fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing); */
				fs::path dest = replica / fs::relative(entry.path(), src);
                fs::create_directories(dest.parent_path());
                fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
				log("Created: " + trimPath(entry.path().string()));
				hasChanged = true;
			}
			else if (files_last_write_time[entry.path().string()] != last_write_time) { //file has been updated
				/* fs::path dest = replica / entry.path().filename();
				fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing); */
				fs::path dest = replica / fs::relative(entry.path(), src);
                fs::create_directories(dest.parent_path());
                fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
				log("Copying: " + trimPath(entry.path().string()));
				hasChanged = true;
			}
		}
		else if (entry.is_directory()) { //double check this
            current_dirs.insert(entry.path().string());
        }
	}

	//Delete file from the replica dir
	for (const std::pair<const std::string, fs::file_time_type>& file_time : files_last_write_time) {
		if (current_files.count(file_time.first) == 0) {
			/* fs::path dest = replica / fs::path(file_time.first).filename();
			fs::remove(dest); */
			fs::path dest = replica / fs::relative(fs::path(file_time.first), src);
            fs::remove(dest);
			log("Removed: " + trimPath(file_time.first));
			hasChanged = true;
		}
	}

	//correct dir deletion and addition
	for (const std::string& dir : dirs_last_state) {
        if (current_dirs.count(dir) == 0) {
            fs::path dest = replica / fs::relative(fs::path(dir), src);
            fs::remove(dest);
            log("Removed: " + trimPath(dir));
            hasChanged = true;
        }
    }

	files_last_write_time = current_files;
	dirs_last_state = current_dirs; //not sure if I'll keep this - might revert to just copying files
	if (hasChanged)
		log("");
	return hasChanged;
}

// Updates the replica folder to the same state as the source folder on startup
//also needs updates on directory from hasFolderContentChanged function
void FolderSync::updateReplica(const fs::path& path) {
	std::map<std::string, fs::file_time_type> replica_files;

	//populate replica_files
	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(replica)) {
		if (entry.is_regular_file()) {
			replica_files[entry.path().string()] = fs::last_write_time(entry);
		}
	}

	//delete files from the replica dir that aren't in the source dir
	for (const std::pair<const std::string, fs::file_time_type>& file_time : replica_files) {
		if (files_last_write_time.count(file_time.first) == 0) {
			fs::remove(file_time.first);
			log("Removed: " + trimPath(file_time.first));
		}
	}

	//update files in the replica dir
	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(path)) {
		if (entry.is_regular_file()) {
			fs::file_time_type last_write_time = fs::last_write_time(entry);
			if (files_last_write_time[entry.path().string()] != last_write_time) {
				fs::path dest = replica / entry.path().filename();
				fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
				log("Updated: " + trimPath(entry.path().string()));
			}
		}
	}
}

/* 
*
*	Perform sync tasks until user input matches 'exit'
*
*	@return void
*/
void FolderSync::evalLoop(void) {
	log("Launched FolderSync\n");
	updateReplica(this->src);
	while (true) {
		
		auto start = std::chrono::high_resolution_clock::now();
		syncFolders();
		
		//get time elapse and time interval until next sync window
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		std::chrono::duration<double> sleepTime = std::chrono::seconds(this->interval) - elapsed;

		//wait for STOP cmd or end of wait cycle
		while (elapsed < std::chrono::seconds(this->interval)) {
            if (stopFlag) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            end = std::chrono::high_resolution_clock::now();
            elapsed = end - start;
        }
		if (stopFlag) {
            break;
        }
	}
}