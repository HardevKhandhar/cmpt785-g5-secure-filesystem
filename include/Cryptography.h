#ifndef CMPT785_G5_SECURE_FILESYSTEM_ENCRYPTION_H
#define CMPT785_G5_SECURE_FILESYSTEM_ENCRYPTION_H

#include <filesystem>
#include <fstream>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <iostream>
#include <iomanip>
#include <vector>

#ifndef PRIV_KEY_LOC
#define PRIV_KEY_LOC "/.metadata/private_keys/"
#endif

#ifndef PUB_KEY_LOC
#define PUB_KEY_LOC "public_keys/"
#endif

bool createUserKey(const std::string &username, bool isAdmin);

std::string encryptPlainText(const std::string &plaintext, const std::string &username);

std::string decryptCipherText(std::string ciphertext, const std::string &username);

void encryptFile(std::string username, std::string filepath);
std::string decryptFile(std::string username, std::string filepath);

#endif // CMPT785_G5_SECURE_FILESYSTEM_ENCRYPTION_H
