#include "Metrics.hpp"

void Metrics::recordRequest(bool success, double latencyMs){
    totalRequests_.fetch_add(1);
    if (!success) totalErrors_.fetch_add(1);
    latencySumMicros_.fetch_add(static_cast<uint64_t>(latencyMs * 1000));
}

void Metrics::connectionOpened(){
    activeConnections_.fetch_add(1);
}
void Metrics::connectionClosed(){
    activeConnections_.fetch_sub(1);
}

double Metrics::averageLatencyMs() const{
    uint64_t requests = totalRequests_.load();
    if (requests == 0) return 0.0;
    return(latencySumMicros_.load() / 1000.0) / requests;
}