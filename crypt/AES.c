#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <string.h>
#include "../includes/crypt.h"

#define AES_KEY_SIZE 32 // 256 bits
#define AES_IV_SIZE 12  // GCM standard IV size (96 bits)
#define TAG_SIZE 16     // Authentication tag size
static uint8_t tag[TAG_SIZE];
// Encrypt function
int aes_gcm_encrypt(const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext) {
    const uint8_t *aad;
    size_t aad_len = 0;
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;

    uint8_t iv[AES_IV_SIZE];
    RAND_bytes(iv, AES_IV_SIZE); // Generate a random IV

    // Embed the IV at the start of the ciphertext
    memcpy(ciphertext, iv, AES_IV_SIZE);

    // Create and initialize context
    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;

    // Initialize encryption operation
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) return -1;

    // Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1) return -1;

    // Initialize key and IV
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, AES_CRYPT_KEY, iv) != 1) return -1;

    // Add AAD data (if any)
    if (aad && aad_len > 0) {
        if (EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) return -1;
    }

    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext + AES_IV_SIZE, &len, plaintext, plaintext_len) != 1) return -1;
    ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext + AES_IV_SIZE + len, &len) != 1) return -1;
    ciphertext_len += len;

    // Get the authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1) return -1;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return AES_IV_SIZE + ciphertext_len; // Include IV size in ciphertext length
}

int aes_gcm_decrypt(const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext) {
    const uint8_t *aad;
    size_t aad_len = 0;
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len, ret;

    // Extract IV from the ciphertext
    uint8_t iv[AES_IV_SIZE];
    memcpy(iv, ciphertext, AES_IV_SIZE);

    // Create and initialize context
    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;

    // Initialize decryption operation
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) return -1;

    // Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1) return -1;

    // Initialize key and IV
    if (EVP_DecryptInit_ex(ctx, NULL, NULL, AES_CRYPT_KEY, iv) != 1) return -1;

    // Add AAD data (if any)
    if (aad && aad_len > 0) {
        if (EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) return -1;
    }

    // Decrypt ciphertext
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext + AES_IV_SIZE, ciphertext_len - AES_IV_SIZE) != 1) return -1;
    plaintext_len = len;

    // Set expected authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (void *)tag) != 1) return -1;

    // Finalize decryption (will fail if tag doesn't match)
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        return plaintext_len;
    } else {
        // Decryption failed
        return -1;
    }
}

//
//int main() {
//    uint8_t plaintext[] = "Hello, AES-GCM!";
//    uint8_t ciphertext[128];
//    uint8_t decrypted[128];
//
//    // Generate random key
//
//    printf("Plaintext: %s\n", plaintext);
//
//    // Encrypt
//    int ciphertext_len = aes_gcm_encrypt(plaintext, strlen((char *)plaintext), ciphertext);
//
//    if (ciphertext_len < 0) {
//        fprintf(stderr, "Encryption failed!\n");
//        return 1;
//    }
//
//    printf("Ciphertext: ");
//    for (int i = 0; i < ciphertext_len; i++) {
//        printf("%X", ciphertext[i]);
//    }
//    printf("\n");
//
//    // Decrypt
//    int decrypted_len = aes_gcm_decrypt(ciphertext, ciphertext_len, decrypted);
//    if (decrypted_len < 0) {
//        fprintf(stderr, "Decryption failed!\n");
//        return 1;
//    }
//
//    decrypted[decrypted_len] = '\0'; // Null-terminate the plaintext
//    printf("Decrypted: %s\n", decrypted);
//
//    return 0;
//}
