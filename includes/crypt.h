//
// Created by BaseDCaTx on 1/16/2025.
//

#ifndef XHTTP_CRYPT_H
#define XHTTP_CRYPT_H

static const char* AES_CRYPT_KEY = "OgBaSeDcAtX12PM8asi@ciaoleajxmeffa34$$$@!,,,.asdjvidadie";
int aes_gcm_encrypt(const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext);
int aes_gcm_decrypt(const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext);

#endif //XHTTP_CRYPT_H
