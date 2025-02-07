#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include "Net.h"
#include "crc32.h"  // Include CRC32 checksum function
#include "ServerSideOperations.h"

#pragma warning(disable: 4996) // required by Visual Studio

using namespace std;
using namespace net;

const int DefaultPort = 30001;
const int PacketSize = 256;
const int ProtocolId = 0x11223344;
const float TimeOut = 10.0f;


// Class responsible for sending files over UDP
class FileSender {
public:
    FileSender(const string& serverIP, int port, const string& filename)
        : connection(ProtocolId, TimeOut) {
        int a, b, c, d;
        if (sscanf(serverIP.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            serverAddress = Address(a, b, c, d, port);
        }
        else {
            cerr << "Invalid IP address format!" << endl;
            exit(1);
        }
        this->filename = filename;
    }

    void Start() {
        // Initialize sockets
        // This function comes from Net.h
        if (!InitializeSockets()) {
            cerr << "Failed to initialize sockets." << endl;
            return;
        }

        // Start the client
        // This function comes from Net.h
        if (!connection.Start(DefaultPort)) {
            cerr << "Could not start client on port " << DefaultPort << endl;
            return;
        }

        connection.Connect(serverAddress);
        printf("Client connecting to %d.%d.%d.%d:%d\n",
            serverAddress.GetA(), serverAddress.GetB(),
            serverAddress.GetC(), serverAddress.GetD(),
            serverAddress.GetPort());

        SendFile();
        ShutdownSockets();
    }

private:
    string filename;
    Address serverAddress;
    ReliableConnection connection;

    void SendFile() {
        ifstream file(filename, ios::binary | ios::ate);
        if (!file) {
            cerr << "Error: Unable to open file " << filename << endl;
            return;
        }

        auto fileSize = file.tellg();
        file.seekg(0, ios::beg);
        uint32_t fileChecksum = CRC32::computeFile(filename);

        SendFileInfo(fileSize, fileChecksum);
        SendFileChunks(file);
        SendCompletionSignal();
    }

    // This function comes from ReliableUDP.cpp
    // Sends metadata about the file, including its name, size, and computed CRC-32 checksum to the receiver.
    void SendFileInfo(size_t fileSize, uint32_t fileChecksum) {
        unsigned char packet[PacketSize] = { 0 };
        memcpy(packet, "FF1", 3);
        memcpy(packet + 3, filename.c_str(), filename.size());
        memcpy(packet + 131, &fileSize, sizeof(fileSize));
        memcpy(packet + 139, &fileChecksum, sizeof(fileChecksum));

        // This function comes from Net.h
        connection.SendPacket(packet, PacketSize);
        cout << "Sent file info: " << filename << " (" << fileSize << " bytes) CRC32: " << fileChecksum << endl;
    }

    void SendFileChunks(ifstream& file) {
        vector<unsigned char> buffer(PacketSize - 43);
        size_t chunkNumber = 0;

        auto startTime = chrono::high_resolution_clock::now();
        while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0) {
            size_t chunkSize = file.gcount();
            uint32_t chunkChecksum = CRC32::compute(buffer);

            unsigned char packet[PacketSize] = { 0 };
            memcpy(packet, "CC2", 3);
            memcpy(packet + 3, &chunkNumber, sizeof(chunkNumber));
            memcpy(packet + 7, &chunkSize, sizeof(chunkSize));
            memcpy(packet + 11, &chunkChecksum, sizeof(chunkChecksum));
            memcpy(packet + 43, buffer.data(), chunkSize);

            // This function comes from Net.h
            connection.SendPacket(packet, PacketSize);
            chunkNumber++;
        }
        auto endTime = chrono::high_resolution_clock::now();

        chrono::duration<double> duration = endTime - startTime;
        double mbps = ((chunkNumber * PacketSize) * 8.0) / (duration.count() * 1e6);
        cout << "File transmission completed at " << mbps << " Mbps." << endl;
    }

    void SendCompletionSignal() {
        unsigned char packet[PacketSize] = { 0 };
        memcpy(packet, "AD", 2);
        // This function comes from Net.h
        connection.SendPacket(packet, PacketSize);
        cout << "Sent completion signal." << endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage:\n";
        cerr << "  Server Mode: ReliableUDP.exe server <port>\n";
        cerr << "  Client Mode: ReliableUDP.exe <server_ip> <port> <file_path>\n";
        return 1;
    }

    string mode = argv[1];

    if (mode == "server" && argc == 3) {
        int port = stoi(argv[2]);
        FileReceiver server(port);
        server.Start();
    }
    else if (argc == 4) {
        string serverIP = argv[1];
        int port = stoi(argv[2]);
        string filename = argv[3];

        FileSender client(serverIP, port, filename);
        client.Start();
    }
    else {
        cerr << "Invalid arguments!" << endl;
        return 1;
    }

    return 0;
}
