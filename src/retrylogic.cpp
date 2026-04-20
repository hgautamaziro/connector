#include "retrylogic.hpp"
#include "logger.hpp"
#include <thread>
#include <chrono>
#include <iostream>
 
bool retryOperation(const std::function<bool()>& operation, int maxRetries, int baseDelayMs)
{
    int attempt = 0;
    std::cout <<" Entering retry logic" << std::endl;
    while (attempt < maxRetries) {
        attempt++;
        if (operation()) {
            return true;
        }
        /* Added to support the network failure issues 
        If network fails need exponetial delay instaed of linear (extra sec) to support system to recover */
        if (attempt < maxRetries) {
           int backoff = baseDelayMs * (1 << (attempt - 1));
           LOG_WARN("Attempt " + std::to_string(attempt) + " failed. Retrying in " + std::to_string(backoff) + " ms");
           std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
       }
    }
    LOG_ERROR("All " + std::to_string(maxRetries) + " attempts exhausted.");
    return false;
}