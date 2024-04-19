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

    std::vector<std::string> contents = splitText(filePath, '/');
    std::string fileRelativePath;
    if (contents.size()>3){
        fileRelativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});
    }

    std::filesystem::rename("./filesystem/.metadata/FileNameMapping.txt", "./filesystem/.metadata/DeleteFileNameMapping.txt");

    std::ifstream oldFile("./filesystem/.metadata/DeleteFileNameMapping.txt");
    std::ofstream newFile("./filesystem/.metadata/FileNameMapping.txt");

    std::string line;
    if (oldFile.is_open() && newFile.is_open()) {
        while (getline(oldFile, line)) {
            if (line.find(fileRelativePath) == std::string::npos) {
                newFile << line << std::endl;
            }
        }
        oldFile.close();
        newFile.close();

        std::remove("./filesystem/.metadata/DeleteFileNameMapping.txt");
    } else {
        std::cerr << "Unable to open file" << std::endl;
        result = false;
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
                if (sender == s && filename == f) {
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

bool FileSystem::pathExistsInFileNameMapping(const std::string username, const std::string path) {
    std::ifstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt");

    if (!fileNameMapping.is_open()) {
        return false;
    }

    std::string line;
    bool found = false;

    while (std::getline(fileNameMapping, line)) {
        std::vector<std::string> parts = splitText(line, ',');
        if (parts[0] == username && parts[1] == path) {
            found = true;
            break;
        }
    }

    fileNameMapping.close();

    return found;
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
            // parts = splitText(parts[1], '/');
            fileNameMapping.close();
            return parts[1];
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


void FileSystem::addFileToFileNameMapping(const std::filesystem::path plainPath, const std::filesystem::path cipherPath, std::string _username) {

    if(_username.empty()){
        _username=username;
    }
    
    std::vector<std::string> contents = splitText(cipherPath.string(), '/');
    std::string relativePath = std::accumulate(contents.begin() + 3, contents.end(), std::string(), [](const std::string &a, const std::string &b) {return a.empty() ? b : a + '/' + b;});

    std::ofstream fileNameMapping("./filesystem/.metadata/FileNameMapping.txt", std::ios::app | std::ios::out);
    if (!fileNameMapping.is_open()) {
        std::cout << "Error: Could not open FileNameMapping.txt" << std::endl;
        return;
    }
    fileNameMapping << _username << "," << plainPath.string() << "," << relativePath << std::endl;
    fileNameMapping.close();
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

            username = getCipherUsername(str);
            plainUsername = getPlainUsername(username);
            base_directory = "./filesystem/" + str;
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

        std::string plainDir;
        if (strcmp(directory, "./filesystem") == 0) {
            plainDir = getPlainUsername(entry->d_name);
        } else {
            // Check if the entry is a directory or a file
            std::string cipherPath = (std::string) directory + "/" + entry->d_name;
            std::string plainPath = getPlainPath(cipherPath);
            std::vector<std::string> parts = splitText(plainPath, '/');
            plainDir = parts.back();
        }

        if (entry->d_type == DT_DIR) {
            // It's a directory
            std::cout << "d -> " << plainDir << std::endl;
        } else if (entry->d_type == DT_REG) {
            // It's a regular file
            std::cout << "f -> " << plainDir << std::endl;
        } else {
            // For other types, you can either ignore or print a generic type
            std::cout << "? -> " << plainDir << std::endl;
        }
    }

    // Close the directory
    closedir(dir);
}

void FileSystem::makeFile(const std::string& make_file, const std::string &user) {
    // TODO: admin should be able to read everything

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

    std::string userPath = getCipherPath("./filesystem/" + username + "/personal").string();

    if (newEncPath.string().length() <= userPath.length() || newEncPath.string().substr(0, userPath.length()) != userPath) {
        std::cout << "Forbidden!" << std::endl;
        return;
    }

    std::string workingDirectory = "./" + getCurrentWorkingDirectory();

    // Check if the file already exists
    if (pathExistsInFileNameMapping(username, newPlainPath.string())) {
        newEncPath = getCipherPath("./filesystem/" + username + "/" + newPlainPath.string());

        if (std::filesystem::is_directory(newEncPath.string())) {
            std::cout << "Unable to create file in specified path." << std::endl;
            return;
        }

        std::cout << "The file '" << filename << "' already exists. It will be overwritten." << std::endl;
        flag = true;
        receivers = allReceivers(username, newEncPath.string());
    }

    if (!flag) {
        addFileToFileNameMapping(newPlainPath, newEncPath);
    }

    // Create or open the file
    std::ofstream mkfile(newEncPath);

    // Check the if file is opened successfully
    if (!mkfile.is_open()) {
        std::cout << "Unable to open the specified file" << std::endl;
        return;
    }

    // Write contents to the file
    mkfile << contents;
    mkfile.close();

    encryptFile(plainUsername, newEncPath);

    if (flag && receivers.size() > 0) {
        for(const std::string& str:receivers) {
            if(std::filesystem::exists(str) && std::filesystem::is_regular_file(str)) {
                deleteFile(str);
            }
            std::vector<std::string>fileParts = splitText(newEncPath.string(),'/');
            std::vector<std::string>strParts = splitText(str,'/');
            commandShareFile(newEncPath, filename, strParts[2], username);
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
    if (pathExistsInFileNameMapping(username, getPlainPath(base_directory).string() + "/" + dir)) {
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

void FileSystem::commandShareFile(const std::filesystem::path &source ,const std::string &sourcefile, const std::string &_username, const std::string &_source_username) {
    //first lets decrypt the file to print out the contents
    //open a new file with a name generated by the cipher of the second user
    //add to filenamemapping as well
    //copy the decrypted contents
    //close the file
    //set the permissions then
    std::string encFileName = std::to_string(std::hash<std::string>{}(encryptPlainText(sourcefile, getPlainUsername(_username))));
    std::filesystem::path destinationPath = getCipherPath("./filesystem/"+_username+"/shared/").string();

    std::string contents=decryptFile(getPlainUsername(_source_username),source);
    std::string newPlainPath = "shared/" + sourcefile;
    std::string newSharedEncPath = destinationPath/encFileName;
    addFileToFileNameMapping(newPlainPath, newSharedEncPath, _username);

    std::ofstream mkfile(newSharedEncPath, std::ios::app | std::ios::out);

    if (!mkfile.is_open()) {
        std::cout << "Unable to open the specified file" << std::endl;
        return;
    }

    mkfile << contents;
    mkfile.close();
    encryptFile(getPlainUsername(_username),newSharedEncPath);
    fileShare(_source_username, source, newSharedEncPath);

    std::cout << "File has been shared!" << std::endl;
};

FileSystem::FileSystem(const std::string &username, bool isAdmin):username(username), isAdmin(isAdmin) {
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
    return !filename.empty() && filename.find(".") == std::string::npos && filename.find('/') == std::string::npos;
}

void FileSystem::processUserCommand(const std::string &command, bool isAdmin, const std::string &user) {
    if (command.length() > COMMAND_SIZE_LIMIT) {
        std::cerr << "Error: command size limit (128KB) exceeded" << std::endl;
        return;
    }

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
            std::cout << "Not enough arguments!" << std::endl;
            return;
        }

        std::string filename = source[1];
        if (!fileNameIsValid(filename)) {
            std::cout << "Provided filename is not valid!" << std::endl;
            return;
        }

        std::string receiver = source[2];
        if (!userExists(receiver)) {
            std::cout << "Receiver is not valid!" << std::endl;
            return;
        }

        std::string filePath = getCipherPath("./filesystem/" + username + "/" + getPlainPath(base_directory).string() + "/" + filename).string();
        if (pathExistsInFileNameMapping(username, getPlainPath(base_directory).string() + "/" + filename)){
            commandShareFile(filePath, filename, getCipherUsername(receiver), username);
        } else {
            std::cout << "File does not exist" << std::endl;
        }
    
    } else {
        std::cout << "Invalid Command" << std::endl;
    }
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
    size_t pos = path.string().find_last_of('/');
    if (pos == std::string::npos) {
        return;
    }

    std::string encryptedPath = path.string().substr(0, pos);
    std::string plainNewDirPath = path.string().substr(pos + 1);


    std::string _username= splitText(encryptedPath,'/')[2];

    std::string plain_username=getPlainUsername(_username);

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

    fileNameMapping << _username << "," << plainResult << "," << cipherResult << std::endl;

    std::filesystem::create_directories(encryptedPath + "/" + encNewDirPath);
}

void FileSystem::addUser(const std::string &_username, bool isAdmin) {
    if(userExists(_username)) {
        return;
    }

    std::filesystem::create_directories("./filesystem/" + _username + "/" + ".metadata/private_keys");

    std::string publicKeyPath = "public_keys/" + _username + "_pub.pem";
    if (!std::filesystem::exists(publicKeyPath)) {
        if (createUserKey(_username,isAdmin)) {
            std::string encUsername = std::to_string(std::hash<std::string>{}(encryptPlainText(_username,_username)));
            std::filesystem::rename("./filesystem/" + _username, "./filesystem/" + encUsername);
            addUserToUsernameMapping(_username, encUsername);

            createDirectories("./filesystem/" + encUsername + "/" + "personal");
            createDirectories("./filesystem/" + encUsername + "/" + "shared");

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

std::string FileSystem::getPlainCurrentWorkingDirectory() {
    if (base_directory == "./filesystem") {
        return "filesystem";
    } else if (base_directory == "./filesystem/" + username) {
        return "filesystem/" + plainUsername;
    }
    return "filesystem/" + plainUsername + "/" + getPlainPath(base_directory).string();
}
