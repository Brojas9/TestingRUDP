#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include "Net.h"
#include "crc32.h"  // Include CRC32 checksum function
#pragma warning(disable: 4996) // required by Visual Studio

using namespace std;
using namespace net;

const int DefaultPort = 30000;
const int PacketSize = 256;
const int ProtocolId = 0x11223344;
const float TimeOut = 10.0f;

// Class to handle receiving files over UDP
class FileReceiver {
public:
    FileReceiver(int port) : connection(ProtocolId, TimeOut) {
        this->port = port;
    }

    void Start() {
        // Initialize sockets
        // This function comes from Net.h
        if (!InitializeSockets()) {
            printf("Failed to initialize sockets.\n");
            return;
        }

        // Start the server
        // This function comes from Net.h
        if (!connection.Start(port)) {
            printf("Could not start server on port %d\n", port);
            return;
        }

        // Listen for incoming connections
        // This function comes from Net.h
        connection.Listen();
        printf("Server listening on port %d...\n", port);
        ReceiveFile();
        ShutdownSockets();
    }

private:
    int port;
    ReliableConnection connection;
    string filename;
    ofstream outputFile;
    uint64_t expectedFileSize = 0;
    uint32_t expectedChecksum;
    chrono::high_resolution_clock::time_point startTime;

    void ReceiveFile() {
        bool receiving = true;
        uint64_t receivedBytes = 0;
        startTime = chrono::high_resolution_clock::now();

        while (receiving) {
            unsigned char packet[PacketSize];
            // This function comes from ReliableUDP.cpp
            int bytesReceived = connection.ReceivePacket(packet, sizeof(packet));

            if (bytesReceived > 0) {
                string packetType(reinterpret_cast<char*>(packet), 2);

                if (packetType == "FF1") {
                    ProcessFileInfo(packet, bytesReceived);
                }
                else if (packetType == "CC2") {
                    receivedBytes += ProcessFileChunk(packet, bytesReceived);
                }
                else if (packetType == "AD") {
                    receiving = false; // End of transmission
                }
            }
            // This function comes from ReliableUDP.cpp
            connection.Update(1.0f / 30.0f);
        }

        FinalizeTransfer(receivedBytes);
    }

    void ProcessFileInfo(unsigned char* packet, int size) {
        char nameBuffer[128];
        uint64_t fileSize;
        uint32_t checksum;

        // Extract file metadata from packet
        memcpy(nameBuffer, packet + 3, 128);
        nameBuffer[127] = '\0';
        filename = string(nameBuffer);

        memcpy(&fileSize, packet + 131, sizeof(fileSize));
        memcpy(&checksum, packet + 139, sizeof(checksum));
        expectedChecksum = checksum;
        expectedFileSize = fileSize;

        // Open file for writing
        outputFile.open(filename, ios::binary);
        if (!outputFile) {
            printf("Failed to open file: %s\n", filename.c_str());
            exit(1);
        }

        printf("Receiving file: %s (%llu bytes) CRC32: %u\n", filename.c_str(), expectedFileSize, expectedChecksum);
    }

    int ProcessFileChunk(unsigned char* packet, int size) {
        uint32_t chunkNumber;
        uint32_t chunkSize;
        uint32_t chunkChecksum;

        // Extract chunk metadata
        memcpy(&chunkNumber, packet + 3, sizeof(chunkNumber));
        memcpy(&chunkSize, packet + 7, sizeof(chunkSize));
        memcpy(&chunkChecksum, packet + 11, sizeof(chunkChecksum));

        // Extract file data
        vector<unsigned char> fileData(packet + 43, packet + 43 + chunkSize);

        outputFile.write(reinterpret_cast<char*>(fileData.data()), chunkSize);
        return chunkSize;
    }

    void FinalizeTransfer(uint64_t receivedBytes) {
        outputFile.close();
        printf("File transfer complete. Received %llu bytes.\n", receivedBytes);

        // Verify integrity using CRC32
        uint32_t receivedChecksum = CRC32::computeFile(filename);
        if (receivedChecksum == expectedChecksum) {
            printf("File integrity verified. CRC Match!\n");
        }
        else {
            printf("Checksum mismatch! Expected: %u, Received: %u\n", expectedChecksum, receivedChecksum);
        }

        // Calculate transfer speed
        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> duration = endTime - startTime;
        double mbps = (receivedBytes * 8.0) / (duration.count() * 1e6);
        printf("Transfer speed: %.2f Mbps\n", mbps);
    }
};

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? stoi(argv[1]) : DefaultPort;
    FileReceiver server(port);
    server.Start();
    return 0;
}
