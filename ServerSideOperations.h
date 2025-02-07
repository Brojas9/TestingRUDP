#ifndef SERVERSIDEOPERATIONS_H
#define SERVERSIDEOPERATIONS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include "Net.h"
#include "crc32.h"

#pragma warning(disable: 4996) // Required for Visual Studio

using namespace std;
using namespace net;

class FileReceiver {
public:
    FileReceiver(int port);
    void Start();

private:
    int port;
    ReliableConnection connection;
    string filename;
    ofstream outputFile;
    uint64_t expectedFileSize = 0;
    uint32_t expectedChecksum;
    chrono::high_resolution_clock::time_point startTime;

    void ReceiveFile();
    void ProcessFileInfo(unsigned char* packet, int size);
    int ProcessFileChunk(unsigned char* packet, int size);
    void FinalizeTransfer(uint64_t receivedBytes);
};

#endif // SERVERSIDEOPERATIONS_H
