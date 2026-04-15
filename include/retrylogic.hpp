#pragma once
#include <functional>
 
bool retryOperation(const std::function<bool()>& operation,
                    int maxRetries,
                    int baseDelayMs = 1000);