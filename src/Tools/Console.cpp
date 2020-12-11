#include <cstdio>
#include <cstdarg>
#include <Math/Rectangle.hpp>
#include <cstring>
#include <fstream>

#include "Console.hpp"


#include "Camera.hpp"
#include "ConsoleVar.hpp"
//#include "Entity/RenderLayers.hpp"
#include "Renderer/Renderer.hpp"

ConsoleCmd Console::_clearCommand("clear", ClearCommand);

void Console::ClearCommand(ConsoleCommandBinder& binder)
{
    binder.Help("Clears the console");

    for (auto& line : binder.GetConsole()->_lines)
    {
        line.clear();
    }
}

ConsoleCmd Console::_aliasCommand("alias", AliasCommand);

void Console::AliasCommand(ConsoleCommandBinder& binder)
{
    std::string name;
    std::string value;

    binder
        .Bind(name, "name")
        .Bind(value, "value")
        .Help("Binds an alias to a value to be executed in its place (like Bash aliases)");

    binder.GetConsole()->_aliases[name] = value;
}

void EchoCommand(ConsoleCommandBinder& binder)
{
    std::string value;

    binder
        .Bind(value, "value")
        .Help("Prints to the console");

    binder.GetConsole()->Log("%s\n", value.c_str());
}
ConsoleCmd echoCommand("echo", EchoCommand);

void HelpCommand(ConsoleCommandBinder& binder)
{
    std::string commandName;
    bool hasCmd = binder.TryBind(commandName, "command");
    binder.Help("Prints all the commands");

    if (hasCmd)
    {
        binder.GetConsole()->ShowHelpForCommand(commandName);
    }
    else
    {
        auto& allCmds = GetAllConsoleCommands();
        for (auto cmd : allCmds)
        {
            binder.GetConsole()->Log("%s\n", cmd.first.c_str());
        }
    }
}
ConsoleCmd helpCommand("help", HelpCommand);

ConsoleExecuteResult Console::Execute(const std::string& input)
{
    auto terms = SplitInput(input);

    return ExecuteSplitInput(terms, "");
}

ConsoleExecuteResult Console::ExecuteFile(const char* fileName)
{
    std::ifstream file;
    file.open(fileName);

    if (!file.is_open())
    {
        return ConsoleExecuteResult::Error("Failed to execute file " + std::string(fileName));
    }
    else
    {
        std::string line;
        while (std::getline(file, line))
        {
            Execute(line);
        }

        file.close();

        return ConsoleExecuteResult::Success("");
    }
}

void Console::ShowHelpForCommand(const std::string& command)
{
    auto cmd = GetConsoleCmd(command);

    if(cmd == nullptr)
    {
        Log("No such command\n");
    }
    else
    {
        ConsoleCommandBinder binder({ command }, true, this);

        try
        {
            cmd->commandHandler(binder);
        }
        catch(...)
        {
            
        }
    }
}

void Console::SerializeVariables(const char* fileName)
{
    std::ofstream varFile(fileName);

    for (auto var : GetAllConsoleVars())
    {
        if(var.second->persist)
        {
            varFile << var.second->name << " \"" << var.second->GetStringValue() << "\"\n";
        }
    }

    varFile.close();
}

void Console::LoadVariables(const char* fileName)
{
    ExecuteFile(fileName);
}

void Console::RunUserConfig(const char* fileName)
{
    ExecuteFile(fileName);
}

ConsoleExecuteResult Console::ExecuteSplitInput(const std::vector<std::string>& terms, const std::string& containingAlias)
{
    std::vector<std::vector<std::string>> commands;
    commands.resize(1);

    for (auto& term : terms)
    {
        if (term == ";" || term == "\n")
        {
            commands.emplace_back();
        }
        else
        {
            commands[commands.size() - 1].push_back(term);
        }
    }

    ConsoleExecuteResult lastSuccess = ConsoleExecuteResult::Success("");

    for (auto& command : commands)
    {
        if (command.size() == 0)
        {
            continue;
        }

        // Substitute variables
        for (int i = 1; i < command.size(); ++i)
        {
            auto& arg = command[i];
            for (int index = (int)arg.size(); index >= 0; --index)
            {
                if (arg[index] == '$')
                {
                    int start = index;
                    int end = index + 1;

                    while (end < arg.size() && (isalnum(arg[end]) || arg[end] == '-'))
                    {
                        ++end;
                    }

                    std::string varName = std::string(arg.begin() + start + 1, arg.begin() + end);
                    auto var = GetConsoleVar(varName);
                    std::string varValue = var != nullptr
                        ? var->GetStringValue()
                        : "(null)";

                    arg.replace(arg.begin() + start, arg.begin() + end, varValue.begin(), varValue.end());
                }
            }
        }

        auto name = command[0];

        // Expand aliases
        if (_aliases.count(name) != 0 && name != containingAlias)
        {
            auto aliasTerms = SplitInput(_aliases[name]);

            // Add the rest of the arguments
            aliasTerms.insert(aliasTerms.end(), command.begin() + 1, command.end());

            auto result = ExecuteSplitInput(aliasTerms, name);
            if (result.isSuccess)
            {
                lastSuccess = result;
                continue;
            }
            else
            {
                return result;
            }
        }

        auto var = GetConsoleVar(name);

        if (var != nullptr)
        {
            if (command.size() == 1)
            {
                auto value = var->GetStringValue();

                return ConsoleExecuteResult::Success(value);
            }
            else if (command.size() == 2)
            {
                auto setResult = var->TrySetValue(command[1]);

                if (setResult != ConsoleValueParseResult::Success)
                {
                    return ConsoleExecuteResult::Error("Failed to bind value to var");
                }

                lastSuccess = ConsoleExecuteResult::Success(command[1]);
                continue;
            }
            else
            {
                return ConsoleExecuteResult::Error("Expected format [varName] [newValue]");
            }
        }

        auto cmd = GetConsoleCmd(name);
        if (cmd != nullptr)
        {
            ConsoleCommandBinder binder(command, false, this);

            try
            {
                cmd->commandHandler(binder);
                lastSuccess = ConsoleExecuteResult::Success(binder.Result());
                continue;
            }
            catch (const InvalidConsoleCommandCall&)
            {
                auto errorMessage = binder.ErrorMessage()
                    + "\n"
                    + binder.Usage();

                return ConsoleExecuteResult::Error(errorMessage);
            }
        }

        return ConsoleExecuteResult::Error("Unknown command or variable");
    }

    return lastSuccess;
}

std::vector<std::string> Console::SplitInput(const std::string& input)
{
    std::vector<std::string> result;
    std::string term;

    int length = input.length();
    int i = 0;

    while (i < length)
    {
        if (isspace(input[i]))
        {
            do
            {
                ++i;
            } while (i < length && isspace(input[i]));

            if (term != "")
            {
                result.push_back(term);
                term.clear();
            }
        }
        else if (input[i] == '\r')
        {
            ++i;
        }
        else if (input[i] == ';' || input[i] == '\n')
        {
            if (term != "")
            {
                result.push_back(term);
                term.clear();
            }

            const char s[2] = { input[i], '\0' };
            result.push_back(s);

            ++i;
        }
        else if (input[i] == '\"' || input[i] == '\'')
        {
            char end = input[i];

            while (++i < length)
            {
                if (input[i] == end)
                {
                    ++i;
                    break;
                }
                else
                {
                    term += input[i];
                }
            }

            result.push_back(term);
            term.clear();
        }
        else if(input[i] == '#')    // Skip comments
        {
            break;
        }
        else
        {
            term += input[i++];
        }
    }

    if (term != "")
    {
        result.push_back(term);
    }

    return result;
}

void Console::Log(const char* format, ...)
{
    va_list list;
    va_start(list, format);

    VLog(format, list);
}

void Console::VLog(const char* format, va_list list)
{
    {
        const int maxLineSize = 4096;
        char line[maxLineSize];

        vsnprintf(line, maxLineSize, format, list);
        printf("%s", line);

        for (int i = 0; i < strlen(line); ++i)
        {
            if (line[i] == '\n')
            {
                NewLine();
            }
            else
            {
                _lines[_lines.size() - 1] += line[i];
            }
        }
    }
}


void Console::NewLine()
{
    for (int i = 0; i < (int)_lines.size() - 1; ++i)
    {
        _lines[i] = std::move(_lines[i + 1]);
    }
}

void Console::Render(Renderer* renderer, float deltaTime)
{
    _autoCompleteResultsTimer -= deltaTime;

    auto screenSize = renderer->GetCamera()->ZoomedScreenSize();
    auto characterSize = _fontSettings.spriteFont->CharacterDimension(_fontSettings.scale);
    auto maxCharsPerLine = screenSize.x / characterSize.x;

    int textHeight = MaxVisibleLines * characterSize.y;
    Rectangle consoleBounds(
        Vector2(0, 0),
        Vector2(screenSize.x, textHeight + characterSize.y * 2));

    float openSpeed = 5.0;

    if (_consoleState == ConsoleState::Closing)
    {
        _openAmount -= openSpeed * deltaTime;

        if (_openAmount <= 0)
        {
            _consoleState = ConsoleState::Closed;
            return;
        }
    }
    else if (_consoleState == ConsoleState::Opening)
    {
        _openAmount += openSpeed * deltaTime;

        if (_openAmount >= 1)
        {
            _openAmount = 1;
            _consoleState = ConsoleState::Open;
        }
    }

    float yOffset = -(1.0f - _openAmount) * consoleBounds.Height();
    consoleBounds.SetPosition(Vector2(0, yOffset));

    renderer->RenderRectangle(consoleBounds, backgroundColor, renderLayer + 0.0001);

    int y = textHeight;

    const auto minLogScroll = -MaxConsoleLines + MaxVisibleLines;
    _logScroll = Clamp(_logScroll, minLogScroll, 0);

    int lineIndex = MaxConsoleLines - 1 + _logScroll;

    // Don't draw an empty line at the bottom
    if (lineIndex == MaxConsoleLines - 1 && _lines[lineIndex].size() == 0)
    {
        --lineIndex;
    }

    while (y - characterSize.y > 0)
    {
        auto wrappedLine = WrapLine(_lines[lineIndex].c_str(), maxCharsPerLine);
        auto stringSize = _fontSettings.spriteFont->MeasureStringWithNewlines(wrappedLine.c_str(), _fontSettings.scale);
        y -= stringSize.y;
        renderer->RenderString(_fontSettings, wrappedLine.c_str(), Vector2(0, y + yOffset), renderLayer);
        --lineIndex;
    }

    int maxInputOnLine = maxCharsPerLine - 4;

    if (_cursorPosition < _cursorScroll)
    {
        _cursorScroll = _cursorPosition;
    }
    else if (_cursorPosition > _cursorScroll + maxInputOnLine)
    {
        _cursorScroll = _cursorPosition - maxInputOnLine;
    }

    std::string renderedInput;
    if (_textInput.length() > 0)
    {
        renderedInput = _textInput.substr(_cursorScroll);
        _cursorScroll = Clamp(_cursorScroll, 0, (int)_textInput.length() - 1);
    }
    else
    {
        _cursorScroll = 0;
    }

    auto displayString = "] " + renderedInput + " ";

    bool drawCursor = _blinkTimer >= BlinkTime / 2;

    _blinkTimer += deltaTime;
    if (_blinkTimer > BlinkTime)
    {
        _blinkTimer -= BlinkTime;
    }

    if (drawCursor)
    {
        displayString[_cursorPosition - _cursorScroll + 2] = '\x84';    // Cursor char
    }

    renderer->RenderString(_fontSettings, displayString.c_str(), Vector2(0, textHeight + yOffset), renderLayer);
}

void Console::SetFont(const FontSettings& fontSettings)
{
    _fontSettings = fontSettings;
}

std::string Console::WrapLine(const std::string& line, int maxCharsPerLine)
{
    std::string wrappedLine;
    int x = 0;
    int lastBreak = 0;
    int lastLineStart = 0;

    for (int i = 0; i < line.length(); ++i)
    {
        wrappedLine += line[i];

        if (line[i] == '\n')
        {
            x = 0;
            lastBreak = lastLineStart = wrappedLine.size() - 1;
        }
        else
        {
            ++x;

            if (line[i] == ' ')
            {
                lastBreak = wrappedLine.size() - 1;
            }

            if (x > maxCharsPerLine)
            {
                if (lastBreak == lastLineStart)
                {
                    // The line is greater than the entire line length
                    // No choice but to break awkwardly
                    wrappedLine.insert(wrappedLine.end() - 1, '\n');
                    lastBreak = lastLineStart = wrappedLine.size() - 1;
                    x = 0;
                }
                else
                {
                    // Wrap at the last space
                    wrappedLine[lastBreak] = '\n';
                    x = wrappedLine.size() - lastBreak;
                    lastLineStart = lastBreak;
                }
            }
        }
    }

    return wrappedLine;
}

void Console::HandleInput(Input* input)
{
    int key = input->ReadInputKeys();

    auto scrollY = input->MouseWheelY();
    if (scrollY > 0)    // Scroll up
    {
        _logScroll -= 3;
    }
    else if (scrollY < 0)
    {
        _logScroll += 3;
    }

    if (key == 0)
    {
        return;
    }
    else if (key == '\b')
    {
        if (_textInput.size() > 0 && _cursorPosition > 0)
        {
            _textInput.erase(_textInput.begin() + _cursorPosition - 1);
            --_cursorPosition;
        }
    }
    else if(key == '\t')
    {
        std::vector<std::string> allTokens;
        for(auto& cmd : GetAllConsoleCommands()) allTokens.push_back(cmd.first);
        for(auto& var : GetAllConsoleVars()) allTokens.push_back(var.first);

        std::string bestMatch = _textInput;
        std::vector<std::string> potentialMatches;

        for(auto token : allTokens)
        {
            int count = 0;
            for(int i = 0; i < Min(token.length(), bestMatch.length()); ++i)
            {
                if (token[i] == bestMatch[i]) ++count;
                else break;
            }

            if(count >= _textInput.length())
            {
                if(potentialMatches.empty())
                {
                    bestMatch = token;
                }
                else if(count < bestMatch.length())
                {
                    bestMatch = token.substr(0, count);
                }

                potentialMatches.push_back(token);
            }
        }

        _textInput = bestMatch;
        _cursorPosition = _textInput.length();

        if (_autoCompleteResultsTimer < 0)
        {
            _autoCompleteResultsTimer = 1;
        }
        else
        {
            Log("\n");

            std::sort(potentialMatches.begin(), potentialMatches.end());

            for(auto& match : potentialMatches)
            {
                Log("%s\n", match.c_str());
            }
        }
    }
    else if (key == '`')
    {
        return;
    }
    else if (key == -1)  // left arrow
    {
        _cursorPosition = Max(_cursorPosition - 1, 0);
    }
    else if (key == -2)  // right arrow
    {
        _cursorPosition = Min(_cursorPosition + 1, (int)_textInput.length());
    }
    else if (key == -3)  // up arrow
    {
        MoveToHistory(_inputHistoryIndex - 1);
    }
    else if (key == -4)  // down arrow
    {
        MoveToHistory(_inputHistoryIndex + 1);
    }
    else if (key == '\n')
    {
        Log("] %s\n", _textInput.c_str());
        auto result = Execute(_textInput);

        if (result.isSuccess)
        {
            if (result.resultValue != "")
            {
                Log("%s\n", result.resultValue.c_str());
            }
        }
        else
        {
            if (result.errorMessage != "")
            {
                Log("%s\n", result.errorMessage.c_str());
            }
        }

        if (_inputHistoryIndex != _inputHistory.size())
        {
            _inputHistory.pop_back();
        }

        if (_inputHistory.size() == 0 || _inputHistory[_inputHistory.size() - 1] != _textInput)
        {
            _inputHistory.push_back(_textInput);
        }

        _inputHistoryIndex = _inputHistory.size();
        _textInput.clear();
        _cursorPosition = 0;
        _cursorScroll = 0;
    }
    else
    {
        _textInput.insert(_textInput.begin() + _cursorPosition, (char)key);
        ++_cursorPosition;
    }
}

void Console::Open()
{
    if (_consoleState != ConsoleState::Closed)
    {
        return;
    }

    _openAmount = 0;
    _consoleState = ConsoleState::Opening;
}

void Console::Close()
{
    _consoleState = ConsoleState::Closing;
}

void Console::Update()
{
    while(logItems.Size() != 0)
    {
        auto item = logItems.Front();
        logItems.Dequeue();

        Log("%s", item.c_str());
    }
}

void Console::MoveToHistory(int index)
{
    index = Clamp(index, 0, (int)_inputHistory.size());

    if (index == _inputHistoryIndex)
    {
        return;
    }

    if (_inputHistoryIndex == _inputHistory.size())
    {
        _originalInput = _textInput;
    }

    if (index == _inputHistory.size())
    {
        _textInput = _originalInput;
    }
    else
    {
        _textInput = _inputHistory[index];
    }

    _inputHistoryIndex = index;
    _cursorScroll = 0;
    _cursorPosition = _textInput.size();
}

Console::Console()
{
    for (int i = 0; i < MaxConsoleLines; ++i)
    {
        _lines.push_back("");
    }
}
