#pragma once

#include <cstdio>
#include <filesystem>
#include <queue>

#include "System/FileSystem.hpp"
#include "slikenet/peerinterface.h"

class RpcManager;


class FileTransferService
{
public:
    using Path = std::filesystem::path;

    FileTransferService(RpcManager* rpcManager);

    void RegisterRpc();
    bool TryUploadFile(const Path& path, SLNet::AddressOrGUID serverAddress);
    void ReceiveFile(const std::string& fileName, const std::vector<unsigned char>& contents, SLNet::AddressOrGUID fromAddress);

private:
    RpcManager* _rpcManager;
};