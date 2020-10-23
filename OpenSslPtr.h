/**
 * @file OpenSslPtr.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @brief A set of RAII smart pointers for OpenSSL types
 * @version 0.1
 * @date 2020-10-23
 * 
 * @copyright Copyright (c) 2020 Hayden McAfee
 * 
 */

#pragma once

#include <memory>
#include <openssl/ssl.h>

/**
 * @brief unique_ptr deleter for SSL_CTX
 */
struct SslCtxFree
{
    void operator() (SSL_CTX* sslContext)
    {
        SSL_CTX_free(sslContext);
    }
};

/**
 * @brief unique_ptr RAII pointer for SSL_CTX
 */
typedef std::unique_ptr<SSL_CTX, SslCtxFree> SslCtxPtr;

/**
 * @brief unique_ptr deleter for SSL
 */
struct SslFree
{
    void operator() (SSL* ssl)
    {
        SSL_free(ssl);
    }
};

/**
 * @brief unique_ptr RAII pointer for SSL
 */
typedef std::unique_ptr<SSL, SslFree> SslPtr;

/**
 * @brief unique_ptr deleter for generic OpenSSL pointers allocated by OPENSSL_malloc
 */
struct OpenSslFree
{
    void operator() (void* openSslPtr)
    {
        OPENSSL_free(openSslPtr);
    }
};

/**
 * @brief unique_ptr RAII pointer for memory allocated with OPENSSL_malloc
 */
typedef std::unique_ptr<void, OpenSslFree> OpenSslPtr;

/**
 * @brief unique_ptr deleter for SSL_SESSION
 */
struct SslSessionFree
{
    void operator() (SSL_SESSION* sslSession)
    {
        SSL_SESSION_free(sslSession);
    }
};

/**
 * @brief unique_ptr RAII pointer for SSL_SESSION
 */
typedef std::unique_ptr<SSL_SESSION, SslSessionFree> SslSessionPtr;