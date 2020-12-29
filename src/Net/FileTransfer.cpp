#include "FileTransfer.hpp"
#include "Engine.hpp"
#include "System/FileSystem.hpp"

DEFINE_RPC(UploadFileRpc)
{
    void Serialize(ReadWriteBitStream& stream)
    {
        stream.Add(fileName).Add(contents);
    }

    void Execute() override
    {
        engine->GetServerGame()->fileTransferService.ReceiveFile(fileName, contents, fromAddress);
    }

    std::string fileName;
    std::vector<unsigned char> contents;
};

DEFINE_RPC(UploadFileResultRpc)
{
    void Serialize(ReadWriteBitStream& stream) override
    {
        stream.Add(fileName).Add(successful);
    }

    void Execute() override
    {
        if (successful)
        {
            Log("File %s uploaded to server successfully\n", fileName.c_str());
        }
        else
        {
            Log("Failed to upload %s to server\n", fileName.c_str());
        }
    }

    std::string fileName;
    bool successful;
};

bool FileTransferService::TryUploadFile(const char* path, SLNet::AddressOrGUID serverAddress)
{
    std::vector<unsigned char> fileContents;
    if (!TryReadFileContents(path, fileContents))
    {
        return false;
    }

    UploadFileRpc rpc;
    rpc.fileName = path;
    rpc.contents = std::move(fileContents);

    Log("Uploading file...\n");
    _rpcManager->Execute(serverAddress, rpc);

    return true;
}

void FileTransferService::ReceiveFile(const std::string& fileName, const std::vector<unsigned char>& contents, SLNet::AddressOrGUID fromAddress)
{
    bool success;
    FILE* file = fopen(fileName.c_str(), "wb");
    if (file == nullptr)
    {
        success = false;
        Log("Failed to receive file %s\n", fileName.c_str());
    }
    else
    {
        success = true;
        fwrite(contents.data(), 1, contents.size(), file);
        Log("Received file %s\n", fileName.c_str());
    }

    fclose(file);

    UploadFileResultRpc rpc;
    rpc.fileName = fileName;
    rpc.successful = success;

    _rpcManager->Execute(fromAddress, rpc);
}

FileTransferService::FileTransferService(RpcManager* rpcManager)
    : _rpcManager(rpcManager)
{

}

void FileTransferService::RegisterRpc()
{
    _rpcManager->Register<UploadFileRpc>();
    _rpcManager->Register<UploadFileResultRpc>();
}


