#ifndef CMPT785_G5_SECURE_FILESYSTEM_FILESYSTEM_H
#define CMPT785_G5_SECURE_FILESYSTEM_FILESYSTEM_H

#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <sstream>
#include <unistd.h>
#include <unordered_map>

#include "Cryptography.h"
#include "StringUtil.h"

class FileSystem {

private:
    std::unordered_map<std::string, std::filesystem::path> user_keyfiles;
    std::string username;
    std::string plainUsername;
    std::filesystem::path base_directory;
    std::filesystem::path root_directory;

protected:
    static bool deleteFile(const std::filesystem::path& filePath);
    static void fileShare(const std::string &sender, const std::string &filename, const std::string &receiver);
    static std::vector<std::string> allReceivers(const std::string &sender, const std::string &filename);
    void changeDirectory(const std::string &dir);
    static void listDirectoryContents(const char *directory);
    void makeFile(const std::string& make_file, const std::string &user);
    void createDirectory(const std::string &input, const std::string &user);
    void catFile(const std::string &filename);
    void addFileToFileNameMapping(const std::filesystem::path plainPath, const std::filesystem::path cipherPath, std::string _username="");
    void addUserToUsernameMapping(const std::string username, std::string encUsername);
    void createDirectories(const std::filesystem::path path);
    bool pathExistsInFileNameMapping(const std::string user, const std::string path);
    void commandShareFile(const std::filesystem::path &source,const std::string &sourcefile, const std::string &_username, const std::string &_source_username);

public:
    explicit FileSystem(const std::string &username, bool isAdmin=false);
    void createFileSystem(const std::string &_username);
    void createMappings();
    void processUserCommand(const std::string &command, bool isAdmin, const std::string &user);
    void addUser(const std::string &_username, bool isAdmin = false);
    std::string getCurrentWorkingDirectory();
    std::string getPlainCurrentWorkingDirectory();

    const int COMMAND_SIZE_LIMIT = 512 * 1000; // 512KB
};

#endif // CMPT785_G5_FileSystem_H
