#pragma once

#include <cstdio>
#include <filesystem>
#include <queue>
#include "slikenet/peerinterface.h"

class RpcManager;


class FileTransferService
{
public:
    FileTransferService(RpcManager* rpcManager);

    void RegisterRpc();
    bool TryUploadFile(const char* path, SLNet::AddressOrGUID serverAddress);
    void ReceiveFile(const std::string& fileName, const std::vector<unsigned char>& contents, SLNet::AddressOrGUID fromAddress);

private:
    RpcManager* _rpcManager;
};