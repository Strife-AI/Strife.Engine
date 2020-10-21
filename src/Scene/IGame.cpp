#include "IGame.hpp"
#include "Engine.hpp"

IGame::IGame(Engine* engine, SceneManager* sceneManager)
    : _engine(engine)
{
	_engine->SetGame(this, sceneManager);
}

void IGame::Run()
{
    while(_engine->ActiveGame())
    {
		_engine->RunFrame();
	}
}
