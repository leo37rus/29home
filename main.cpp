#include <iostream>
#include <thread>
#include <mutex>

struct Node
{
    Node(int value) : _value(value), _next(nullptr), _node_mutex(new std::mutex) {}
    ~Node() { delete _node_mutex; }
    int _value;
    Node* _next;
    std::mutex* _node_mutex;
};

class FineGrainedQueue
{
public:
    FineGrainedQueue() : _head(nullptr), _queue_mutex(new std::mutex) {}
    ~FineGrainedQueue() { delete _queue_mutex; }

    void push_back(int value)
    {
        Node* newNode = new Node(value);
        if (_head == nullptr)
        {
            _head = newNode;
            return;
        }
        Node* last = _head;
        while (last->_next != nullptr)
        {
            last = last->_next;
        }
        last->_next = newNode;
    }

    void show()
    {
        Node* current = _head;
        if (isEmpty())
        {
            std::cout << "Queue is empty" << std::endl;
        }
        while (current != nullptr)
        {
            std::cout << current->_value << " ";
            current = current->_next;
        }
    }

    bool isEmpty()
    {
        return _head == nullptr;
    }

    void insertIntoMiddle(int value, int pos)
    {
        Node* newNode = new Node(value);

        _queue_mutex->lock();

        Node* current = _head;
        current->_node_mutex->lock();
        _queue_mutex->unlock();

        int currentPos = 0;
        while (currentPos < pos - 2 && current->_next)
        {
            Node* previous = current;
            current->_next->_node_mutex->lock();
            current = current->_next;
            previous->_node_mutex->unlock();
            currentPos++;
        }

        Node* nextNode = current->_next;
        current->_next = newNode;
        current->_node_mutex->unlock();
        newNode->_next = nextNode;
        
        std::lock_guard<std::mutex>lock(*_queue_mutex);
        std::cout << "\nThread ID " << std::this_thread::get_id() << "\t";
        this->show();
    }

    void remove(int value) 
    {
        Node* previous = _head;
        Node* current = _head->_next;
        _queue_mutex->lock();

        if (isEmpty())
        {
            _queue_mutex->unlock();
            return;
        }

        previous->_node_mutex->lock();
        if (current) // проверили и только потом залочили 
            current->_node_mutex->lock();
        if (previous->_value == value)
        {
            _head = current;
            _head->_next = current->_next;
            previous->_node_mutex->unlock();
            current->_node_mutex->unlock();
            _queue_mutex->unlock();
            delete previous;
            return;
        }

        _queue_mutex->unlock();

        while (current) 
        { 
            if (current->_value == value) 
            { 
                previous->_next = current->_next; 
                previous->_node_mutex->unlock(); 
                current->_node_mutex->unlock();
                delete current; 
                return; 
            } 
            Node* old_prev = previous; 
            previous = current; 
            current = current->_next; 
            old_prev->_node_mutex->unlock(); 
            if (current) // проверили и только потом залочили 
                current->_node_mutex->lock(); 
        } 
        previous->_node_mutex->unlock();
    }
    
private:
    Node* _head;
    std::mutex* _queue_mutex;
};

int main()
{
    FineGrainedQueue queue;
    
    int size = 20;
    for (size_t i = 1; i <= size; i++)
    {
        queue.push_back(2*i+1);
    }
    std::cout << "Created queue: ";
    queue.show();

    std::cout << "\n\nQueue after insert elements into middle:";
    std::thread t1(&FineGrainedQueue::insertIntoMiddle, &queue, 1000, 6);
    std::thread t2(&FineGrainedQueue::insertIntoMiddle, &queue, 2000, 11);
    std::thread t3(&FineGrainedQueue::insertIntoMiddle, &queue, 3000, 14);
    std::thread t4(&FineGrainedQueue::insertIntoMiddle, &queue, 5000, 24);
    if (t1.joinable())
        t1.join();
    if (t2.joinable())
        t2.join();
    if (t3.joinable())
        t3.join();
    if (t4.joinable())
        t4.join();

    std::thread t5(&FineGrainedQueue::remove, &queue, 37);
    if (t5.joinable())
        t5.join();
    std::cout << "\n\nQueue after remove element: ";
    queue.show();
    std::cout << "\n";

    std::thread t6(&FineGrainedQueue::remove, &queue, 3);
    if (t6.joinable())
        t6.join();
    std::cout << "\nQueue after remove first element: ";
    queue.show();
    std::cout << "\n";

    return 0;
}