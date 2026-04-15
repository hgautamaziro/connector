#include "retrylogic.hpp"
#include <thread>
#include <chrono>
#include <iostream>
 
bool retryOperation(const std::function<bool()>& operation,
                    int maxRetries,
                    int baseDelayMs)
{
    int attempt = 0;
    std::cout <<" Entering retry logic" << std::endl;
 
    while (attempt < maxRetries) {
        attempt++;
 
        if (operation()) {
            return true;
        }
 
        int backoff = baseDelayMs * attempt;
        std::cerr << "Retry " << attempt
                  << " failed. Retrying in "
                  << backoff << " ms\n";
 
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
    }
 
    return false;
}