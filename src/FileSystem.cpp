#include "FileSystem.h"

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() >= suffix.length()) {
        return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
    } else {
        return false;
    }
}

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

std::filesystem::path getCipherPath(std::string path) {
    std::string line;

    std::vector<std::string> contents = splitText(path, '/');
    if (contents.size() <= 3) {
        return path;
    }

    std::string relativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});

    std::string searchKey = contents[2] + "," + relativePath + ",";

    std::ifstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt");

    while (std::getline(fileNameMapping, line)) {
        if (line.find(searchKey) != std::string::npos) {
            std::vector<std::string> parts = splitText(line, ',');
            fileNameMapping.close();
            return "./filesystem/" + parts[0] + "/" + parts.back();
        }
    }

    fileNameMapping.close();
    return "";
}

std::filesystem::path getPlainPath(std::string path) {
    std::string line;
    
    std::vector<std::string> contents = splitText(path, '/');
    std::string cipherRelativePath;
    if (contents.size()>1){
        cipherRelativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});
    }
    else{
        cipherRelativePath = std::accumulate(contents.begin(), contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});
    }

    std::string endWith =  "," + cipherRelativePath;

    std::ifstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt");

    while (std::getline(fileNameMapping, line)) {
        if (ends_with(line,endWith)) {
            std::vector<std::string> parts = splitText(line, ',');
            parts = splitText(parts[1], '/');
            fileNameMapping.close();
            return parts.back();
        }
    }

    fileNameMapping.close();
    return "";
}

bool userExists(const std::string& username) {
    for (const auto& entry : std::filesystem::directory_iterator(PUB_KEY_LOC)) {
        if (entry.is_regular_file() && entry.path().filename() == username  + "_pub.pem") {
            return true;
        }
    }
    return false;
}

void FileSystem::changeDirectory(const std::string &dir) {
    if(dir.empty()) {
        std::cout << "Directory name not specified." << std::endl;
        return;
    }

    for (char c: dir) {
        if (std::isspace(c) || c == '?' || c == ':' || c == '\\' || c == '*' || c == '"' || c == '|' || c == ',') {
            std::cout << "Invalid characters for cd, please re-enter" << std::endl;
            return;
        }
    }

    std::filesystem::path tmpBaseDirectory = base_directory;
    std::vector<std::string> dirVector = splitText(dir,'/');

    for (const std::string& str:dirVector) {
        std::filesystem::path newPlainPath = tmpBaseDirectory;
        std::filesystem::path cipherPath = ""; 

        if (str=="..") {
            if (newPlainPath != root_directory) {
                newPlainPath = newPlainPath.parent_path();
                cipherPath = newPlainPath; 
            } else {
                std::cout << "Already at the root directory. Cannot go up." << std::endl;
                return;
            }
        } else if (str == ".") {
            continue;
        } else if (str == ".metadata") {
            std::cout << "Metadata access forbidden" << std::endl;
            return;
        } else if (tmpBaseDirectory == "./filesystem") {
            if (!userExists(str)) {
                std::cout << "Directory does not exist" << std::endl;
                return;
            }
            newPlainPath = "./filesystem/" + str;
            cipherPath = "./filesystem/" + getCipherUsername(str); 
        } else if (tmpBaseDirectory == "./filesystem/" + username) {
            newPlainPath = "./filesystem/" + username + "/" + str;
            cipherPath = getCipherPath(newPlainPath); 
        } else {
            newPlainPath = "./filesystem/" + username + "/" + getPlainPath(tmpBaseDirectory).string()+ "/"+ str;
            cipherPath = getCipherPath(newPlainPath); 
        }

        // Check if newPath is valid and within root_directory bounds    
        if (cipherPath != "" && std::filesystem::exists(cipherPath) && std::filesystem::is_directory(cipherPath) && (cipherPath.string().rfind(root_directory.string(), 0) == 0)) {
            tmpBaseDirectory = cipherPath;
        } else {
            std::cout << "Directory does not exist or is not accessible: " << dir << std::endl;
            return;
        }
    }

    base_directory = tmpBaseDirectory;
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
        if (strcmp(entry->d_name, "..") == 0 ||  strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, ".metadata") == 0 ) {
            std::cout << "d -> " << entry->d_name << std::endl;
            continue;
        }

        std::string plainPath;
        if (strcmp(directory, "./filesystem") == 0) {
            plainPath = getPlainUsername(entry->d_name);
        } else {
            // Check if the entry is a directory or a file
            std::string cipherPath = (std::string) directory + "/" + entry->d_name;
            plainPath = getPlainPath(cipherPath);
        }

        if (entry->d_type == DT_DIR) {
            // It's a directory
            std::cout << "d -> " << plainPath << std::endl;
        } else if (entry->d_type == DT_REG) {
            // It's a regular file
            std::cout << "f -> " << plainPath << std::endl;
        } else {
            // For other types, you can either ignore or print a generic type
            std::cout << "? -> " << plainPath << std::endl;
        }
    }

    // Close the directory
    closedir(dir);
}

void FileSystem::makeFile(const std::string& make_file, const std::string &user) {
    // TODO: set size limit to file
    // TODO: should be encrypted by share receiver public key
    // TODO: admin should be able to read everything
    //   if (plaintext.length() > FILE_SIZE_LIMIT) {
    // std::cerr << "Error: file size limit (512MB) exceeded '" << filepath << "'" << std::endl;
    // infile.close();
    // return;
    //   }
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

    for (char c: filename) {
        if (std::isspace(c) || c == '.' || c == '?' || c == ':' || c == '\\' || c == '*' || c == '/' || c == '"' || c == '|' || c == ',') {
            std::cout << "Invalid characters added to the filename, please re-enter" << std::endl;
            return;
        }
    }

    std::filesystem::path newPlainPath = getPlainPath(base_directory)/filename;

    std::string encFileName = std::to_string(std::hash<std::string>{}(encryptPlainText(filename, plainUsername)));
    std::filesystem::path newEncPath = base_directory/encFileName;

    std::string userPath = getCipherPath("./filesystem/" + plainUsername + "/personal").string();

    if (newEncPath.string().length() <= userPath.length() or newEncPath.string().substr(0, userPath.length()) != userPath) {
        std::cout << "Forbidden!" << std::endl;
        return;
    }

    std::string workingDirectory = "./" + getCurrentWorkingDirectory();

    std::vector<std::string> _arr = splitText(userPath,'/');

    // Check if the file already exists
    if (std::filesystem::exists(encFileName)) {
        if (std::filesystem::is_directory(encFileName)) {
            std::cout << "Unable to create file in specified path." << std::endl;
            return;
        }

        std::cout << "The file '" << encFileName << "' already exists. It will be overwritten." << std::endl;
        flag = true;
        receivers = allReceivers(_arr[2],encFileName);
    }

    if (flag) {
        addFileToFileNameMapping(newPlainPath, newEncPath);
    }

    // Create or open the file
    std::ofstream mkfile(newEncPath);

    // Check if file is opened successfully
    if (!mkfile.is_open()) {
        std::cout << "Unable to open the specified file" << std::endl;
        return;
    }

    // Write contents to the file
    mkfile << contents;
    mkfile.close();

    // Output success message based on whether the file was created or modified
    std::vector<std::string> _file = splitText(encFileName,'/');
    std::string _filename =_file[_file.size()/_file[0].size()-1];
    std::vector<std::string> workingDirectoryContents = splitText(workingDirectory,'/');
    std::string sourceUserName = workingDirectoryContents[2];
    std::string sharedPath = workingDirectory.substr(0,workingDirectory.length()-workingDirectoryContents[3].length())+"shared";

    encryptFile(plainUsername, newEncPath);

    if (flag) {
        if(receivers.size() > 0) {
            for(const std::string& str:receivers) {
                if(std::filesystem::exists("./filesystem/"+str+"/shared/"+_filename) && std::filesystem::is_regular_file("./filesystem/"+str+"/shared/"+_filename)) {
                    deleteFile("./filesystem/" + str + "/shared/" + _filename);
                }
                commandShareFile(encFileName,"./filesystem/"+sourceUserName+"/shared",_filename,str,sourceUserName);
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
    std::filesystem::path newPlainPath = "./filesystem/" + plainUsername + "/" + getPlainPath(base_directory).string() + "/" + dir;

    for (char c: dir) {
        if (std::isspace(c) || c == '.' || c == '?' || c == ':' || c == '\\' || c == '*' || c == '/' || c == '"' || c == '|' || c == ',') {
            std::cout << "Invalid characters added to the directory filename, please re-enter" << std::endl;
            return;
        }
    }

    std::string userPath = "./filesystem/" + plainUsername + "/personal";

    if (newPlainPath.string().length() <= userPath.length() or newPlainPath.string().substr(0, userPath.length()) != userPath) {
        std::cout << "Forbidden!" << std::endl;
        return;
    }

    // Check if directory already exists
    if (pathExistsInFileNameMapping(getPlainPath(base_directory).string() + "/" + dir)) {
        std::cout << "Path already exists!" << std::endl;
        return;
    }

    createDirectories(base_directory/dir);

    std::cout << "Directory created successfully" << std::endl;
}

void FileSystem::catFile(const std::string &filename) {
    std::string currentWorkingDirectory = base_directory.string();
    std::string filePlainPath = "./filesystem/" + username + "/" + getPlainPath(base_directory.string()).string() + "/" + filename;
    std::string file = getCipherPath(filePlainPath).string();

    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        std::cout << filename << " doesn't exist." << std::endl;
        return;
    }
    
    std::cout << decryptFile(plainUsername, file) << std::endl;
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

FileSystem::FileSystem(const std::string &username, bool isAdmin):username(username) {
    plainUsername = getPlainUsername(username);
    base_directory = "./filesystem/" + username;
    if(isAdmin) root_directory = "./filesystem";
    else root_directory = base_directory;
    createMappings();
}

void FileSystem::createMappings() {
    if (!std::filesystem::exists("./filesystem/.metadata/FileNameMapping.txt") && !std::filesystem::is_regular_file("./filesystem/.metadata/FileNameMapping.txt")) {
        std::filesystem::create_directories("./filesystem/.metadata");
        std::ofstream MyFile("./filesystem/.metadata/FileNameMapping.txt");
        MyFile << "";
        MyFile.close();
    }
    if (!std::filesystem::exists("./filesystem/.metadata/UsernameMapping.txt") && !std::filesystem::is_regular_file("./filesystem/.metadata/UsernameMapping.txt")) {
        std::filesystem::create_directories("./filesystem/.metadata");
        std::ofstream MyFile("./filesystem/.metadata/UsernameMapping.txt");
        MyFile << "";
        MyFile.close();
    }
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

            if (userExists(_username)) {
                std::cout << "User already exists." << std::endl;
                return;
            }

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

bool FileSystem::pathExistsInFileNameMapping(const std::string path) {
    std::ifstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt");

    if (!fileNameMapping.is_open()) {
        // std::cerr << "Failed to open the file." << std::endl;
        return false;
    }

    std::string line;
    bool found = false;

    while (std::getline(fileNameMapping, line)) {
        std::vector<std::string> parts = splitText(path, '/');
        if (parts[1] != path) {
            found = true;
            break;
        }
    }

    fileNameMapping.close();

    return found;
}

void FileSystem::addFileToFileNameMapping(const std::filesystem::path plainPath, const std::filesystem::path cipherPath) {
    std::vector<std::string> contents = splitText(cipherPath.string(), '/');
    std::string relativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});

    std::ofstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt", std::ios::app | std::ios::out);
    if (!fileNameMapping.is_open()) {
        std::cout << "Error: Could not open FileNameMapping.txt" << std::endl;
        return;
    }
    fileNameMapping << username << "," << plainPath.string() << "," << relativePath << std::endl;
    fileNameMapping.close();
}

void FileSystem::addUserToUsernameMapping(const std::string username, std::string encUsername) {
    std::ofstream usernameMapping("./filesystem/.metadata/UsernameMapping.txt", std::ios::app | std::ios::out);
    if (!usernameMapping.is_open()) {
        std::cout << "Error: Could not open UsernameMapping.txt" << std::endl;
        return;
    }
    usernameMapping << username << "," << encUsername << std::endl;
    usernameMapping.close();
}

void FileSystem::createDirectories(const std::filesystem::path path) {
    // path = "./filesystem/14518622622578385111/14584392843928120385111/new"
    size_t pos = path.string().find_last_of('/');
    if (pos == std::string::npos) {
        return;
    }

    std::string encryptedPath = path.string().substr(0, pos); // ./filesystem/14518622622578385111
    std::string plainNewDirPath = path.string().substr(pos + 1);  // personal


    username= splitText(encryptedPath,'/')[2];

    std::string plain_username=getPlainUsername(username);

    std::vector<std::string> contents = splitText(encryptedPath, '/');
    std::string relativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;}); // 14518622622578385111/14584392843928120385111
    std::string encNewDirPath = std::to_string(std::hash<std::string>{}(encryptPlainText(plainNewDirPath,plain_username)));

    std::ofstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt", std::ios::app | std::ios::out);
    if (!fileNameMapping.is_open()) {
        std::cout << "Error: Could not open FileNameMapping.txt" << std::endl;
        return;
    }

    std::string plainResult = getPlainPath(relativePath).string() + "/" + plainNewDirPath;
    if (!plainResult.empty() && plainResult[0] == '/') {
        plainResult.erase(0, 1);
    }

    std::string cipherResult = relativePath + "/" + encNewDirPath;
    if (!cipherResult.empty() && cipherResult[0] == '/') {
        cipherResult.erase(0, 1);
    }

    fileNameMapping << username << "," << plainResult << "," << cipherResult << std::endl;

    std::filesystem::create_directories(encryptedPath + "/" + encNewDirPath);
}

void FileSystem::addUser(const std::string &_username, bool isAdmin) {
    if(userExists(_username)) {
        return;
    }

    base_directory = "./filesystem/" + _username;
    std::filesystem::create_directories(base_directory/".metadata/private_keys");

    std::string publicKeyPath = "public_keys/" + _username + "_pub.pem";
    if (!std::filesystem::exists(publicKeyPath)) {
        if (createUserKey(_username,isAdmin)) {
            std::string encUsername = std::to_string(std::hash<std::string>{}(encryptPlainText(_username,_username)));
            std::filesystem::rename("./filesystem/" + _username, "./filesystem/" + encUsername);
            addUserToUsernameMapping(_username, encUsername);

            base_directory = "./filesystem/" + encUsername;
            createDirectories(base_directory/"personal");
            createDirectories(base_directory/"shared");

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
