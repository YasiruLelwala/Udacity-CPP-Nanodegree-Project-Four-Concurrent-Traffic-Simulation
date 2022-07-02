#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    
    // Perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);

    // pass unique lock to condition variable
    _cond.wait(uLock,[this]{ return !_queue.empty(); } ); 

    // remove last element from queue
    T message = std::move(_queue.back());
    _queue.pop_back();

    return message;
}


template <typename T>
void MessageQueue<T>::send(T &&message)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    //perfom vector modification under the lock
    std::lock_guard<std::mutex> ulock(_mutex);

    //std::cout << "Message " << message << " will be added to the queue" << std::endl;

    
    _queue.clear();

    /* why _queue.clear()
    At peripheral intersections Traffic is less. As number of changes in traffic light at these intersections 
    is more compared to number of vehicles approaching, there will be accumulation of Traffic light messages in _queue.
    
    By the time new vehicle approaches at any of these peripheral intersections it would be receiving out 
    some older traffic light msg. 
    That's the reason it seems vehicles are crossing red at these intersection, 
    in fact they are crossing based on some previous green signal in that _queue.

    But once new traffic light msg arrives, all the older msgs are redundant. 
    So we have to clear _queue at send (as soon as new msg arrives).

    For central intersection, _queue clear/ no clear won't be a problem, 
    as traffic is huge there it will have enough receives to keep it empty for new msg.
    */

    _queue.push_back(std::move(message));
    _cond.notify_one();

}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}


void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        TrafficLightPhase trafficLightPhase = _messageQueue.receive();

        if (trafficLightPhase == TrafficLightPhase::green) 

        return;

    }
}



TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}


void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    TrafficObject::threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this)); 
}


// virtual function which is executed in a thread

void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // initialize variables
    double cycleDuration; // duration of a cycle, will be a random value between 4 and 6 seconds
    
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    lastUpdate = std::chrono::system_clock::now();
    
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_real_distribution<double> distr(4.0, 6.0);
    
    double timeSinceLastUpdate;

    // generate the cycle duration randomly
    cycleDuration = distr(eng) * 1000;
    
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            switch (_currentPhase)
            {
                case TrafficLightPhase::red:
                _currentPhase = TrafficLightPhase::green;
                std::cout << "Traffic light has turned green" << std::endl;

                break;

                case TrafficLightPhase::green:
                _currentPhase = TrafficLightPhase::red;
                std::cout << "Traffic light has turned red" << std::endl;

                break;
            }
            std::cout << "Traffic light colour/phase has been sent to _messageQueue" << std::endl;

            _messageQueue.send(std::move(_currentPhase));

            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            // regenerate the cycle duration
            cycleDuration = distr(eng) * 1000;
            
        }
    } // eof simulation loop
}


