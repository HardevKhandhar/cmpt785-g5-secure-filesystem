#include "FileSystem.h"

bool FileSystem::deleteFile(const std::filesystem::path& filePath) {
    std::cout << filePath << std::endl;
    bool result = std::filesystem::remove(filePath);
    if(result) {
        std::cout << "Shared files modifying...: " << filePath << std::endl;
    } else {
        std::cout << "Failed to modify shared files" << std::endl;
    }
    return result;
}

void FileSystem::fileShare(const std::string &sender, const std::string &filename, const std::string &receiver) {
    bool flag = false;
    std::string file = "./filesystem/" + sender + "/.metadata/shareFile.txt";
    std::vector<std::string> contents = allReceivers(sender, filename);

    for (const std::string &rec: contents) {
        if (receiver == rec) {
            flag = true;
            break; // Exit loop early if receiver is found
        }
    }

    if (!flag) {
        // Receiver not found, append to file
        std::ofstream outputFile(file, std::ios::app | std::ios::out);
        if (!outputFile.is_open()) {
            std::cout << "Error: Could not open shareFile.txt" << std::endl;
            return;
        }
        outputFile << sender << "," << filename << "," << receiver << std::endl;
    } else {
        // Receiver already present, no need to append
        return;
    }
}

std::vector<std::string> FileSystem::allReceivers(const std::string &sender, const std::string &filename) {
    std::vector<std::string> receiverlist;
    std::vector<std::string> contents = splitText(filename,'/');
    std::string _filename = (filename.find('/') != std::string::npos) ? contents[contents.size()/contents[0].size()-1] : filename;
    std::string file = "./filesystem/"+sender+"/.metadata/shareFile.txt";
    if(std::filesystem::exists(file)) {
        std::ifstream inputFile(file);

        if (!inputFile.is_open()) {
            std::cout << "Could not open shareFile.txt" << std::endl;
        }
        std::string get_l;
        while (std::getline(inputFile, get_l)) {
            std::istringstream iss(get_l);
            std::string s, f, r;
            if (std::getline(iss, s, ',') && std::getline(iss, f, ',') && std::getline(iss, r, ',')) {
                if (sender == s && _filename == f) {
                    receiverlist.push_back(r);
                }
            }
        }
    }
    return receiverlist;
}

void FileSystem::changeDirectory(const std::string &dir) {
    if(dir.empty()) {
        std::cout << "Directory name not specified." << std::endl;
        return;
    }

    std::filesystem::path newPath = base_directory;
    std::vector<std::string> newDir = splitText(dir,'/');
    for (const std::string& str:newDir) {
        if (str=="..") {
            if (newPath != root_directory) {
                newPath = newPath.parent_path();
            } else {
                std::cout << "Already at the root directory. Cannot go up." << std::endl;
                return;
            }
        } else if (str == ".metadata") {
            std::cout << "Metadata access forbidden" << std::endl;
            return;
        } else if (str != ".") {
            newPath /= str;
        }
    }

    // Check if newPath is valid and within root_directory bounds
    if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath) && (newPath.string().rfind(root_directory.string(), 0) == 0)) {
        base_directory = newPath;
    } else {
        std::cout << "Directory does not exist or is not accessible: " << dir << std::endl;
    }
}

void FileSystem::listDirectoryContents(const char *directory) {

    // Pointer to directory
    DIR *dir;

    // For each entry in the directory
    struct dirent *entry;

    // Open the current directory
    dir = opendir(directory);
    if (dir == nullptr) {
        std::cout << "Error opening directory" << std::endl;
        return;
    }

    while ((entry = readdir(dir)) != nullptr) {
        // Check if the entry is a directory or a file
        if (entry->d_type == DT_DIR) {
            // It's a directory
            std::cout << "d -> " << entry->d_name << std::endl;
        } else if (entry->d_type == DT_REG) {
            // It's a regular file
            std::cout << "f -> " << entry->d_name << std::endl;
        } else {
            // For other types, you can either ignore or print a generic type
            std::cout << "? -> " << entry->d_name << std::endl;
        }
    }

    // Close the directory
    closedir(dir);
}

void FileSystem::makeFile(const std::string& make_file, const std::string &user) {
    // Extract filename and contents from input
    std::istringstream iss(make_file);
    std::string command, filename, contents;
    bool flag = false;
    std::vector<std::string> receivers;

    iss >> command >> filename; // Read command and filename
    std::getline(iss >> std::ws, contents); // Read contents, including whitespace

    // Trim leading and trailing whitespace from filename and contents
    filename.erase(0, filename.find_first_not_of(" \t"));
    filename.erase(filename.find_last_not_of(" \t") + 1);
    contents.erase(0, contents.find_first_not_of(" \t"));
    contents.erase(contents.find_last_not_of(" \t") + 1);

    // Check if filename and contents are missing
    if (filename.empty() && contents.empty()) {
        std::cout << "<filename> and <contents> arguments are missing, please try again" << std::endl;
        return;
    } else if (filename.empty()) {
        std::cout << "Filename not specified" << std::endl;
        return;
    } else if (contents.empty()) {
        std::cout << "<content> argument is missing, please try again" << std::endl;
        return;
    }

    std::filesystem::path newPath = base_directory;
    std::vector<std::string> newDir = splitText(filename,'/');
    for (const std::string& str:newDir) {
        // Check if there are any invalid characters in the input for the directory name
        for (char c: str) {
            if (std::isspace(c) || c == '?' || c == ':' || c == '\\' || c == '*' || c == '/' || c == '"' || c == '|') {
                std::cout << "Invalid characters added to the filename, please re-enter" << std::endl;
                return;
            }
        }

        if (str=="..") {
            if (newPath != root_directory) {
                newPath = newPath.parent_path();
            } else {
                std::cout << "Already at the root directory. Cannot go up." << std::endl;
                return;
            }
        } else if (str == ".metadata") {
            std::cout << "Metadata access forbidden" << std::endl;
            return;
        } else if (str != ".") {
            newPath /= str;
        }
    }

    std::string userPath = "./filesystem/" + user + "/personal";

    if (newPath.string().length() < userPath.length() or newPath.string().substr(0, userPath.length()) != userPath) {
        std::cout << "Forbidden!" << std::endl;
        return;
    }

    std::string workingDirectory = "./" + getCurrentWorkingDirectory();
    filename = newPath;

    std::vector<std::string> _arr = splitText(userPath,'/');

    // Check if the file already exists
    if (std::filesystem::exists(filename)) {
        std::cout << "The file '" << filename << "' already exists. It will be overwritten." << std::endl;
        flag = true;
        receivers = allReceivers(_arr[2],filename);
    }

    // Create or open the file
    std::ofstream mkfile(filename);

    // Check if file is opened successfully
    if (!mkfile.is_open()) {
        std::cout << "Unable to open the specified file" << std::endl;
        return;
    }

    // Write contents to the file
    mkfile << contents;
    mkfile.close();

    // Output success message based on whether the file was created or modified
    std::vector<std::string> _file = splitText(filename,'/');
    std::string _filename =_file[_file.size()/_file[0].size()-1];
    std::vector<std::string> workingDirectoryContents = splitText(workingDirectory,'/');
    std::string sourceUserName = workingDirectoryContents[2];
    std::string sharedPath = workingDirectory.substr(0,workingDirectory.length()-workingDirectoryContents[3].length())+"shared";

    if (flag) {
        if(receivers.size() > 0) {
            for(const std::string& str:receivers) {
                if(std::filesystem::exists("./filesystem/"+str+"/shared/"+_filename) && std::filesystem::is_regular_file("./filesystem/"+str+"/shared/"+_filename)) {
                    deleteFile("./filesystem/" + str + "/shared/" + _filename);
                }
                commandShareFile(filename,"./filesystem/"+sourceUserName+"/shared",_filename,str,sourceUserName);
            }
        }
        //share the updated file
        std::cout << "File is modified successfully:  " << filename << std::endl;
    } else {
        std::cout << "File is created successfully: " << filename << std::endl;
    }
}

void FileSystem::createDirectory(const std::string &input, const std::string &user) {
    // Extract directory name from input
    std::string dir = input.substr(input.find(" ") + 1);

    std::filesystem::path newPath = base_directory;
    std::vector<std::string> newDir = splitText(dir,'/');
    for (const std::string& str:newDir) {
        // Check if there are any invalid characters in the input for the directory name
        for (char c: str) {
            if (std::isspace(c) || c == '?' || c == ':' || c == '\\' || c == '*' || c == '/' || c == '"' || c == '|') {
                std::cout << "Invalid characters added to the directory filename, please re-enter" << std::endl;
                return;
            }
        }

        if (str=="..") {
            if (newPath != root_directory) {
                newPath = newPath.parent_path();
            } else {
                std::cout << "Already at the root directory. Cannot go up." << std::endl;
                return;
            }
        } else if (str == ".metadata") {
            std::cout << "Metadata access forbidden" << std::endl;
            return;
        } else if (str != ".") {
            newPath /= str;
        }
    }
    std::string userPath = "./filesystem/" + user + "/personal";

    if (newPath.string().length() < userPath.length() or newPath.string().substr(0, userPath.length()) != userPath) {
        std::cout << "Forbidden!" << std::endl;
        return;
    }

    // Check if directory already exists
    if (std::filesystem::exists(newPath) && std::filesystem::is_directory(newPath)) {
        std::cout << "Directory already exists" << std::endl;
        return;
    }

    std::filesystem::create_directories(newPath);
    std::cout << "Directory created successfully" << std::endl;
}

void FileSystem::catFile(const std::string &filename) {
    std::string currentWorkingDirectory = getCurrentWorkingDirectory();
    std::string filePath = currentWorkingDirectory + "/" + filename;

    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::cout << line << std::endl;
        }
        file.close();
    } else {
        std::cout << filename << " doesn't exist." << std::endl;
    }
}

void FileSystem::commandShareFile(const std::filesystem::path &source, const std::filesystem::path& sharedPath ,const std::string &sourcefile, const std::string &_username, const std::string &_source_username) {
    std::filesystem::path destinationPath = "./filesystem/"+_username+"/shared/"+sourcefile;
    std::filesystem::path sharedDirectory = sharedPath/sourcefile;
    size_t found = source.string().find("shared");
    if(found != std::string::npos) {
        std::cout << "Cannot share files from the shared directory" << std::endl;
        return;
    }

    try {
        std::filesystem::copy(source,destinationPath,std::filesystem::copy_options::overwrite_existing);
        std::filesystem::permissions(destinationPath, std::filesystem::perms::owner_read | std::filesystem::perms::group_read| std::filesystem::perms::others_read, std::filesystem::perm_options::replace);
        if(!std::filesystem::exists(sharedDirectory)) {
            std::filesystem::copy(source, sharedDirectory, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::permissions(sharedDirectory, std::filesystem::perms::owner_read | std::filesystem::perms::group_read| std::filesystem::perms::others_read, std::filesystem::perm_options::replace);
        } else {
            std::filesystem::remove_all(sharedDirectory);
            std::filesystem::copy(source, sharedDirectory, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::permissions(sharedDirectory, std::filesystem::perms::owner_read | std::filesystem::perms::group_read| std::filesystem::perms::others_read, std::filesystem::perm_options::replace);
        }
        std::cout << "File has been copied to " << destinationPath << std::endl;
        fileShare(_source_username,sourcefile,_username);
    } catch (const std::filesystem::filesystem_error &e) {
        std::cout << e.what() << std::endl;
    }
};

FileSystem::FileSystem(const std::string &username, bool isAdmin, int count):username(username) {
    base_directory = "./filesystem/" + username;
    if(isAdmin) root_directory = "./filesystem";
    else root_directory = base_directory;
    createFileSystem(username,count);
}

void FileSystem::createFileSystem(const std::string &_username, int count) {
    base_directory = "./filesystem/" + _username;
    if(count == 1) {
        std::filesystem::create_directories(base_directory/".metadata/private_keys");
    }
    std::filesystem::create_directories(base_directory/"personal");
    std::filesystem::create_directories(base_directory/"shared");
}

bool fileNameIsValid(const std::string filename) {
    return !filename.empty() && filename.find("..") == std::string::npos && filename.find('/') == std::string::npos;
}

void FileSystem::processUserCommand(const std::string &command, bool isAdmin, const std::string &user) {
    if(command.substr(0,3) == "cd ") {
        changeDirectory(command.substr(3));
    } else if(command.substr(0,2) == "ls") {
        listDirectoryContents((base_directory).c_str());
    } else if(command == "pwd") {
        std::string workingDirectory = getCurrentWorkingDirectory();
        workingDirectory = "./" + workingDirectory;
        std::cout << workingDirectory << std::endl;
    } else if(command.substr(0,8) == "adduser ") {
        if(isAdmin) {
            std::vector<std::string>str = splitText(command,' ');
            std::string _username=str[1];
            createFileSystem(_username,1);
            addUser(_username);
        } else {
            std::cout << "Invalid Command" << std::endl;
        }
    } else if (command.substr(0,6) == "mkdir ") {
        createDirectory(command, user);
    } else if(command.substr(0,7) == "mkfile ") {
        makeFile(command, user);
    } else if (command.substr(0, 4) == "cat ") {
        std::string filename = command.substr(4);
        if (fileNameIsValid(filename)) {
            catFile(filename);
        } else {
            std::cout << "Provided filename is not valid." << std::endl;
        }
    } else if (command.substr(0,6) == "share ") {
        std::vector source = splitText(command,' ');
        if(source.size() < 3) {
            std::cout << "Not enough arguments." << std::endl;
            return;
        }

        std::string filename = source[1];
        if (!fileNameIsValid(filename)) {
            std::cout << "Provided filename is not valid." << std::endl;
            return;
        }

        std::string receiver = source[2];
        
        // TODO: check if receiver is valid.

        std::string sourceUserName = splitText("./" + getCurrentWorkingDirectory(), '/')[2];

        std::string filePath = "./" + getCurrentWorkingDirectory() + "/" + filename;
        std::string personalDirectory = "./filesystem/" + sourceUserName + "/personal";

        if (filePath.find(personalDirectory) == std::string::npos) {
            std::cout << "Can't share file outside your personal directory." << std::endl;
            return;
        }

        if(!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
            std::cout << "File does not exist." << std::endl;
            return;
        }

        std::string sharedPath = "./filesystem/" + receiver + "/shared";

        commandShareFile(filePath, sharedPath, filename, receiver, sourceUserName);
        // FIX: when a file content is updated, the file is also shared with the sender.
    } else {
        std::cout << "Invalid Command" << std::endl;
    }
}

void FileSystem::addUser(const std::string &_username, bool isAdmin) {
    std::string publicKeyPath = "public_keys/" + _username + "_pub.pem";
    if (!std::filesystem::exists(publicKeyPath)) {
        if (createUserKey(_username,isAdmin)) {
            std::cout << "User " << _username << " added successfully." << std::endl;
        }
    } else {
        std::cout << "User " << _username << " already exists." << std::endl;
    }
}

std::string FileSystem::getCurrentWorkingDirectory() {
    std::string result;

    std::filesystem::path canonicalBase = std::filesystem::canonical(base_directory);
    std::vector<std::string> currentPath = splitText(canonicalBase.c_str(),'/');

    bool filesystemFound = false;
    for (const auto& segment : currentPath) {
        if (filesystemFound) {
            result += "/" + segment;
        } else if (segment == "filesystem") {
            filesystemFound = true;
            result += segment;
        }
    }
    return result;
}