#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    
    // push a value on the queue
    // gauranteed to push
    void push (T value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(value));
        cv.notify_one();
    }

    // pop if the queue is not empty
    // return whether popped or not
    bool try_pop (T& value) {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            return false;
        }
        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    void wait_pop (T& value) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() { return !queue.empty(); });
        value = std::move(queue.front());
        queue.pop();
    }

    int size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    bool is_producing (void) {
        bool result;
        {
            std::unique_lock<std::mutex> lock(mutex);
            result = producing;
        }
        return result;
    }

    void start_producing (void) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            producing = true;
        }
        cv.notify_all();
    }

    void stop_producing (void) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            producing = false;
        }
        cv.notify_all();
    }

private:
    mutable std::mutex mutex;
    std::queue<T> queue;
    std::condition_variable cv;
    bool producing = false;
};