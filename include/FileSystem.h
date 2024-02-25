#ifndef CMPT785_G5_SECURE_FILESYSTEM_FILESYSTEM_H
#define CMPT785_G5_SECURE_FILESYSTEM_FILESYSTEM_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include "Cryptography.h"
#include "StringUtil.h"
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <sstream>

class FileSystem {

private:
    std::unordered_map<std::string, std::filesystem::path> user_keyfiles;
    std::string username;
    std::filesystem::path base_directory;
    std::filesystem::path root_directory;

protected:
    static bool deleteFile(const std::filesystem::path& filePath);
    static void fileShare(const std::string &sender, const std::string &filename, const std::string &receiver);
    static std::vector<std::string> allReceivers(const std::string &sender, const std::string &filename);
    void changeDirectory(const std::string &dir);
    static void listDirectoryContents(const char *directory);
    void makeFile(const std::string& make_file, const std::string &userPath);
    static void createDirectory(const std::string &input, const std::string &userPath);
    void catFile(const std::string &filename);
    static void commandShareFile(const std::filesystem::path &source, const std::filesystem::path& sharedPath ,const std::string &sourcefile, const std::string &_username, const std::string &_source_username);

public:
    explicit FileSystem(const std::string &username, bool isAdmin=false, int count=0);
    void createFileSystem(const std::string &_username, int count=0);
    void processUserCommand(const std::string &command, bool isAdmin, const std::string &user);
    static void addUser(const std::string &_username, bool isAdmin = false);
    std::string getCurrentWorkingDirectory();
};

#endif // CMPT785_G5_FileSystem_H
