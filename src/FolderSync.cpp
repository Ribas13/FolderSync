#include "FolderSync.hpp"

namespace fs = std::filesystem;

FolderSync::FolderSync(char **av) {
	this->src = av[1];
	checkDirectoryExists(this->src);
	this->replica = av[2];
	checkDirectoryExists(this->replica);
	this->interval = atoi(av[3]);
	if (this->interval <= 0)
		throw std::runtime_error("Interval needs to be between 0 and 2147483647");
	checkLogFileAndCreate(av[4]);
	logfile.open(av[4], std::ios_base::trunc);
	if (!logfile)
        throw std::runtime_error("Error opening log file: " + std::string(av[4]));
	this->stopFlag = false;
	this->inputThread = std::thread(&FolderSync::monitorInput, this);
}

FolderSync::~FolderSync() {
	
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
			if (hasFolderContentChanged())
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

bool FolderSync::hasFolderContentChanged() {
	std::map<std::string, fs::file_time_type> current_files;
	bool hasChanged = false;
	std::vector<fs::path> toRemove;

	//Iterate over the replica directory first
	for (const auto& entry : fs::recursive_directory_iterator(replica)) {
		fs::path relativepath = fs::relative(entry.path(), replica);
		fs::path sourcePath = src/ relativepath;

		//If the file doesn't exist in the source directory, delete it
		if (!fs::exists(sourcePath)) {
			toRemove.push_back(entry.path());
			hasChanged = true;
		}
	}

	//delete the marked files and dirs
	for (const auto& path : toRemove) {
		fs::remove_all(path);
		log("Removed: " + trimPath(path.string()));
	}

	//Then iterate over the source directory
	for (const auto& entry : fs::recursive_directory_iterator(src)) {
		fs::path relativePath = fs::relative(entry.path(), src);
		fs::path replicaPath = replica / relativePath;

		//if the file is new or has been updated, copy it over
		if (!fs::exists(replicaPath) || fs::last_write_time(entry.path()) != fs::last_write_time(replicaPath)) {
			if (fs::is_directory(entry.path())) {
				if (!fs::exists(replicaPath)) {
					fs::create_directories(replicaPath);
					log("Copied directory: " + trimPath(replicaPath.string()));
					hasChanged = true;
				}
			} else {
				auto sourceTime = fs::last_write_time(entry.path());
				fs::create_directories(replicaPath.parent_path());
				fs::copy(entry.path(), replicaPath, fs::copy_options::overwrite_existing);
				fs::last_write_time(replicaPath, sourceTime);
				log("Copied file: " + trimPath(entry.path().string()));
				hasChanged = true;
			}
		}
	}
	if (hasChanged)
		log("");
	return hasChanged;
}

/* 
	Perform sync tasks until user input matches 'exit'

	@return void
*/
void FolderSync::evalLoop(void) {
	log("Launched FolderSync\n");
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