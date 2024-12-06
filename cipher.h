#ifndef CIPHER_H
#define CIPHER_H

#include <QString>
#include <QFile>
#include <QDebug>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/QTextCodec>
#else
#include <QtCore5Compat/QTextCodec>
#endif

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

class Cipher
{
public:
    Cipher();
    ~Cipher();

    int encrypt_aes128_ecb(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *ciphertext);
    int decrypt_aes128_ecb(unsigned char *ciphertext, int ciphertext_len, unsigned char  *key, unsigned char *plaintext);
    QByteArray encrypt_aes128_ecb(const QByteArray &plaintext, const QByteArray &key);
    QByteArray decrypt_aes128_ecb(const QByteArray &ciphertext, const QByteArray &key);
private:
    EVP_CIPHER_CTX *ctx;
    void handleErrors(void);

};

#endif // CIPHER_H
