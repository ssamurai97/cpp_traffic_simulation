#include <iostream>
#include <random>
#include <future>
#include<queue>
#include "TrafficLight.h"

using namespace std::chrono_literals;


/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> u_lock(msg_mutex);

    _cond.wait(u_lock, [this] {
        return !_queue.empty();
    });

    T msg = std::move(_queue.back());

    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::unique_lock<std::mutex> u_lock(msg_mutex);

    _queue.push_back(std::move(msg));

    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _msg_queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
       std::unique_lock<std::mutex> {_mutex};
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto cur_phase = _msg_queue->receive();
        if (cur_phase == green)
            break;
    }
    _condition.notify_one();
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

[[noreturn]] // virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    std::random_device rd;
    std::mt19937 engine(rd());

    std::unique_lock<std::mutex> u_lock{_mutex};

    std::cout<< "Traffic light # "<< _id << "::cycleThroughPhase " <<std::this_thread::get_id()<<'\n';
    u_lock.unlock();
    
     std::uniform_int_distribution<> dis(4.0, 6.0);
    auto duration = dis(engine);
    auto last_update = std::chrono::system_clock::now();

    while(1){
        long time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_update).count();
        //sleep every iteration 1 sec
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //traffic toggle
        if(time_elapsed >= duration){
            if(_currentPhase == red){
                _currentPhase = green;
            } else{
                _currentPhase = red;
            }

            auto msg = _currentPhase;

            auto sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _msg_queue, std::move(msg));
            sent.wait();

            //reset time for next cycle
            duration = dis(engine);
            last_update = std::chrono::system_clock::now();
        }
    }


}

