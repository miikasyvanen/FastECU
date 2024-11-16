#include "cipher.h"

//Size rounded up to closest 16 byte limit
qsizetype size16(qsizetype size)
{
    return size + 16 - (size % 16);
}

Cipher::Cipher()
{
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    // Initialise the library
}

Cipher::~Cipher()
{
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
}

void Cipher::handleErrors(void)
{
    //ERR_print_errors_fp(stderr);
    //abort();
    qDebug() << "Cipher error";
}

QByteArray Cipher::encrypt_aes128_ecb(const QByteArray &plaintext, const QByteArray &key)
{
    unsigned char ciphertext[size16(plaintext.size())];
    int ciphertext_len = encrypt_aes128_ecb((unsigned char *)plaintext.constData(),
                                            plaintext.size(),
                                            (unsigned char *)key.constData(),
                                            ciphertext);
    QByteArray encrypted((const char *)ciphertext, ciphertext_len);
    return encrypted;
}

QByteArray Cipher::decrypt_aes128_ecb(const QByteArray &ciphertext, const QByteArray &key)
{
    unsigned char plaintext[size16(ciphertext.size())];
    int plaintext_len = decrypt_aes128_ecb((unsigned char *)ciphertext.constData(),
                                            ciphertext.size(),
                                            (unsigned char *)key.constData(),
                                            plaintext);
    QByteArray encrypted((const char *)plaintext, plaintext_len);
    return encrypted;
}

int Cipher::encrypt_aes128_ecb(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *ciphertext)
{
    int len;

    int ciphertext_len;

    EVP_CIPHER_CTX_set_padding(ctx, 0);
    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * In this example we are using 128 bit AES (i.e. a 128 bit key).
    */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL))
        handleErrors();

    /* Provide the message to be encrypted, and obtain the encrypted output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    * this stage.
    */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();

    ciphertext_len += len;

    return ciphertext_len;
}

int Cipher::decrypt_aes128_ecb(unsigned char *ciphertext, int ciphertext_len, unsigned char  *key, unsigned char *plaintext)
{
    int len;

    int plaintext_len;

    EVP_CIPHER_CTX_set_padding(ctx, 0);
    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * In this example we are using 128 bit AES (i.e. a 128 bit key). The
  */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL))
        handleErrors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    return plaintext_len;
}

