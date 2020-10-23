#pragma once

#include <string>
#include <vector>
#include <Renderer/Renderer.hpp>
#include <System/Input.hpp>
#include <map>

#include "ConsoleCmd.hpp"
#include "Memory/ConcurrentQueue.hpp"
#include "Memory/SpinLock.hpp"

class Renderer;

struct ConsoleAutocompleteResult
{
    std::string knownForSure;
    std::vector<std::string> suggestions;
};

struct ConsoleExecuteResult
{
    static ConsoleExecuteResult Success(const std::string& resultValue)
    {
        ConsoleExecuteResult result;
        result.isSuccess = true;
        result.resultValue = resultValue;

        return result;
    }

    static ConsoleExecuteResult Error(const std::string& errorMessage)
    {
        ConsoleExecuteResult result;
        result.isSuccess = false;
        result.errorMessage = errorMessage;

        return result;
    }

    bool isSuccess;
    std::string errorMessage;
    std::string resultValue;
};

class Console
{
public:
    Console();

    void SetFont(const FontSettings& fontSettings);

    ConsoleExecuteResult Execute(const std::string& input);
    ConsoleExecuteResult ExecuteFile(const char* fileName);
    void ShowHelpForCommand(const std::string& command);

    static void SerializeVariables(const char* fileName);
    void LoadVariables(const char* fileName);
    void RunUserConfig(const char* fileName);

    void Log(const char* format, ...) PRINTF_ARGS(2, 3);
    void VLog(const char* format, va_list list);
    void Render(Renderer* renderer, float deltaTime);
    void HandleInput(Input* input);
    
    bool IsOpen() const { return _consoleState != ConsoleState::Closed; }
    void Open();
    void Close();
    void Update();

    ConcurrentQueue<std::string> logItems;
    float renderLayer = -0.99;

private:
    enum { MaxConsoleLines = 200 };
    enum { MaxVisibleLines = 20 };
    
    enum class ConsoleState
    {
        Closed,
        Opening,
        Open,
        Closing
    };

    static constexpr Color backgroundColor = Color(0, 0, 0, 128);
    static constexpr float BlinkTime = 0.5f;

    static ConsoleCmd _clearCommand;
    static void ClearCommand(ConsoleCommandBinder& binder);

    static ConsoleCmd _aliasCommand;
    static void AliasCommand(ConsoleCommandBinder& binder);

    ConsoleExecuteResult ExecuteSplitInput(const std::vector<std::string>& terms, const std::string& containingAlias);
    void NewLine();
    std::vector<std::string> SplitInput(const std::string& input);
    std::string WrapLine(const std::string& line, int maxCharsPerLine);

    void MoveToHistory(int index);
    
    std::string _textInput;
    int _cursorPosition = 0;
    int _cursorScroll = 0;
    int _logScroll = 0;

    std::vector<std::string> _lines;

    FontSettings _fontSettings;
    ConsoleState _consoleState = ConsoleState::Closed;
    float _openAmount = 0;  // From 0 to 1
    float _blinkTimer = 0;
    std::map<std::string, std::string> _aliases;

    std::vector<std::string> _inputHistory;
    std::string _originalInput;
    int _inputHistoryIndex = 0;

    float _autoCompleteResultsTimer = 0;
};