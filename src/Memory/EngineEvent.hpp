#include <functional>

template<typename TEvent>
struct EngineEvent
{
    using IteratorType = typename std::list<std::function<void(const TEvent& ev)>>::iterator;

    class EventHandler
    {
    public:
        IteratorType operator+=(const std::function<void(const TEvent& ev)>& callback)
        {
            _callbacks.emplace_front(callback);
            return _callbacks.begin();
        }

        void operator-=(IteratorType callback)
        {
            _callbacks.erase(callback);
        }

        void Invoke(const TEvent& ev)
        {
            for (auto& callback : _callbacks)
            {
                callback(ev);
            }
        }

    private:
        std::list<std::function<void(const TEvent& ev)>> _callbacks;
    };

    void Send()
    {
        eventHandler.Invoke(*static_cast<TEvent*>(this));
    }

    static IteratorType AddHandler(const std::function<void(const TEvent& ev)>& callback)
    {
        return eventHandler += callback;
    }

    static void RemoveHandler(IteratorType callback)
    {
        eventHandler -= callback;
    }

    static EventHandler eventHandler;
};

template<typename TEvent>
typename EngineEvent<TEvent>::EventHandler EngineEvent<TEvent>::eventHandler;
