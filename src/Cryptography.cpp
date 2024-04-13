#include "Cryptography.h"

bool createUserKey(const std::string &username, bool isAdmin) {
    bool ret = 0;
    int bits = 4096;
    RSA *rsa = nullptr;
    BIGNUM *bne = nullptr;
    BIO *bp_public = nullptr, *bp_private = nullptr;
    unsigned long e = RSA_F4;

    bne = BN_new();
    if (!BN_set_word(bne, e)) {
        BN_free(bne);
        return false;
    }

    rsa = RSA_new();

    if (!RSA_generate_key_ex(rsa, bits, bne, nullptr)) {
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }

    bp_public = BIO_new_file((std::string(PUB_KEY_LOC) + username + "_pub.pem").c_str(), "w+");
    if (!PEM_write_bio_RSAPublicKey(bp_public, rsa)) {
        BIO_free_all(bp_public);
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }

    bp_private = BIO_new_file(("./filesystem/" + username + std::string(PRIV_KEY_LOC) + username + "_priv.pem").c_str(), "w+");
    if (!PEM_write_bio_RSAPrivateKey(bp_private, rsa, nullptr, nullptr, 0, nullptr, nullptr)) {
        BIO_free_all(bp_private);
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }

    std::filesystem::permissions(("./filesystem/" + username + std::string(PRIV_KEY_LOC) + username + "_priv.pem").c_str(), std::filesystem::perms::owner_read | std::filesystem::perms::group_read| std::filesystem::perms::others_read, std::filesystem::perm_options::replace);

    BIO_free(bp_public);
    BIO_free(bp_private);
    RSA_free(rsa);
    BN_free(bne);

    std::cout << "Keys generated and saved for user: " << username << std::endl;
    return true;
}

std::string encryptPlainText(const std::string &plaintext, const std::string &username) {
    const int bits = 4096;
    const int max_length = bits / 8 - 42; // Adjusted for RSA_PKCS1_OAEP_PADDING
    if (plaintext.empty() || plaintext.length() > max_length) {
        std::cerr << "Plaintext is invalid due to length constraints." << std::endl;
        return "";
    }

    // Load the public key
    std::string publicKeyPath = std::string(PUB_KEY_LOC) + username + "_pub.pem";
    BIO *pubKeyFile = BIO_new_file(publicKeyPath.c_str(), "r");
    if (!pubKeyFile) {
        std::cerr << "Error opening public key file." << std::endl;
        return "";
    }

    RSA *rsaPubKey = PEM_read_bio_RSAPublicKey(pubKeyFile, nullptr, nullptr, nullptr);
    BIO_free_all(pubKeyFile); // Free BIO object once we're done with it

    if (!rsaPubKey) {
        std::cerr << "Error loading public key." << std::endl;
        ERR_print_errors_fp(stderr);
        return "";
    }

    // Prepare for encryption
    int padding = RSA_PKCS1_OAEP_PADDING;
    size_t encryptLen = RSA_size(rsaPubKey);
    std::vector<unsigned char> encryptedData(encryptLen);

    // Perform the encryption
    int result = RSA_public_encrypt(plaintext.length(), reinterpret_cast<const unsigned char *>(plaintext.c_str()),
                                    encryptedData.data(), rsaPubKey, padding);

    // Free the RSA key
    RSA_free(rsaPubKey);

    if (result == -1) {
        std::cerr << "Encryption failed." << std::endl;
        ERR_print_errors_fp(stderr);
        return "";
    }

    // Return the encrypted data as a string
    return {reinterpret_cast<char *>(encryptedData.data()), static_cast<size_t>(result)};
}

std::string decryptCipherText(std::string ciphertext, const std::string &username) {
    if (ciphertext.empty()) {
        std::cerr << "Ciphertext is empty" << std::endl;
    }

    BIO *privateKeyFile = BIO_new_file(("./filesystem/" +username+ std::string (PRIV_KEY_LOC) + username + "_priv.pem").c_str(), "r");
    if (!privateKeyFile){
        BIO_free_all(privateKeyFile);
        ERR_print_errors_fp(stderr);
        return "";
    }
    RSA *rsaPrivKey = PEM_read_bio_RSAPrivateKey(privateKeyFile, nullptr, nullptr, nullptr);

    if (rsaPrivKey == nullptr) {
        std::cerr << "Error loading private key." << std::endl;
        ERR_print_errors_fp(stderr);
    }
    int padding = RSA_PKCS1_OAEP_PADDING;

    int decryptLen = RSA_size(rsaPrivKey);
    std::vector<unsigned char> decryptedData(decryptLen);
    int result = RSA_private_decrypt(
        ciphertext.size(), reinterpret_cast<const unsigned char *>(ciphertext.data()),
        decryptedData.data(), rsaPrivKey, padding
    );

    if (result == -1) {
        std::cerr << "Decryption failed." << std::endl;
        ERR_print_errors_fp(stderr);
        RSA_free(rsaPrivKey);
    }

    RSA_free(rsaPrivKey);
    EVP_cleanup();
    ERR_free_strings();

    return std::string{reinterpret_cast<char *>(decryptedData.data()), static_cast<size_t>(result)};
}

void encryptFile(std::string username, std::string filepath) {
  // read file
  std::ifstream infile(filepath);
  if (!infile) {
    std::cerr << "Error: Could not open key file '" << filepath << "'" << std::endl;
    return;
  }
  std::string plaintext((std::istreambuf_iterator<char>(infile)),
                         std::istreambuf_iterator<char>());
  if (infile.fail() && !infile.eof()) {
    std::cerr << "Error: Failed to read data from file '" << filepath << "'" << std::endl;
    infile.close();
    return;
  }
  infile.close();

  // encrypt
  // chunk the ciphertext
  std::string ciphertext = "";

  int FILE_SIZE_LIMIT = 8 * 1024 * 512; // 512KB

  int CHUNK_SIZE = 100;

  for (int i = 0; i < plaintext.length(); i+= CHUNK_SIZE) {
    int remaining_bytes = plaintext.length() - i;
    std::string plaintext_chunk;
    if (remaining_bytes < CHUNK_SIZE) {
      plaintext_chunk = plaintext.substr(i, remaining_bytes);
    } else {
      plaintext_chunk = plaintext.substr(i, CHUNK_SIZE);
    }
    ciphertext += encryptPlainText(plaintext_chunk, username);
  }

  // write file
  std::ofstream outfile(filepath, std::ios::trunc);
  if (!outfile) {
    std::cerr << "Error: Could not open file '" << filepath << "'" << std::endl;
    return;
  }
  outfile << ciphertext;
  if (outfile.fail()) {
    std::cerr << "Error: Failed to write data to file '" << ciphertext << "'" << std::endl;
    outfile.close();
    return;
  }
  outfile.close();
  if (outfile.fail()) {
    std::cerr << "Error: Failed to close file '" << ciphertext << "'" << std::endl;
    return;
  }
}

std::string decryptFile(std::string username, std::string filepath) {
  // read file
  std::ifstream infile(filepath);
  if (!infile) {
    std::cerr << "Error: Could not open key file '" << filepath << "'" << std::endl;
    throw std::runtime_error("Invalid filepath");
  }
  std::string ciphertext((std::istreambuf_iterator<char>(infile)),
                          std::istreambuf_iterator<char>());
  if (infile.fail() && !infile.eof()) {
    std::cerr << "Error: Failed to read data from file '" << filepath << "'" << std::endl;
    infile.close();
    throw std::runtime_error("Error reading file");
  }
  infile.close();

  return decryptCipherText(ciphertext, username);
}