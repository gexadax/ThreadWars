#include "queue"
#include "list"
#include <mutex>
#include <thread>
#include <random>
#include <memory>
#include <string>
#include <iostream>

using namespace std;

// Part structure
typedef struct Part {
    int part_id;
    float volume;
    typedef shared_ptr<struct Part> PartPtr; // smart pointer
} Part;

static bool done = false;
static bool done2 = false;

queue<Part::PartPtr> shared_queue;
queue<Part::PartPtr> shared_queue2;
mutex lock_queue;
mutex lock_queue2;
mutex lock_cout;

void locked_output(const std::string& str) {
    // Acquire a mutex for protected output
    lock_guard<mutex> raii(lock_cout);
    cout << str << endl;
}

void threadAwork(Part::PartPtr& part) {
    // Subtract from the volume and simulate a delay
    part->volume -= 2;
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

    locked_output("threadAwork finished with part " + to_string(part->part_id));
}

void threadBwork(Part::PartPtr& part) {
    // Subtract from the volume and simulate a delay
    part->volume -= 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

    locked_output("threadBwork finished with part " + to_string(part->part_id));
}

void threadCwork(Part::PartPtr& part) {
    // Subtract from the volume and simulate a delay
    part->volume -= 3;
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

    locked_output("threadCwork finished with part " + to_string(part->part_id));
}

void threadA(list<Part::PartPtr>& input) {
    srand(7777777);
    size_t size = input.size();
    for (size_t i = 0; i < size; i++) {
        // Process a part
        threadAwork(*input.begin());
        // Put it in the queue
        {
            lock_guard<mutex> raii_obj(lock_queue);
            shared_queue.push(Part::PartPtr(*input.begin()));
            input.remove(*input.begin());
            locked_output("Part was added to queue");
        }
    }
    done = true;
}

void threadB() {
    srand(100000);
    while (true) {
        Part::PartPtr part_for_work;
        {
            // Lock the mutex
            lock_queue.lock();
            if (shared_queue.empty()) {
                // If there are no elements, release the mutex
                // and sleep for 1 second before the next check
                lock_queue.unlock();
                if (done) break; // Thread exit condition
                locked_output("threadB useless check, queue is empty. Going to bed");
                this_thread::sleep_for(chrono::milliseconds(1000));
                continue;
            }
            else {
                // Retrieve an element from the queue
                part_for_work = shared_queue.front();
                shared_queue.pop();
                // Don't interfere with thread A, release the mutex
                lock_queue.unlock();
                locked_output("Part was removed from queue");
            }
        }
        // Work on the part
        threadBwork(part_for_work);

        // Pass the processed part to the second queue
        {
            lock_guard<mutex> raii_obj(lock_queue2);
            shared_queue2.push(part_for_work);
            locked_output("Part was added to queue2");
        }
    }
    done2 = true;
}

void threadC() {
    while (true) {
        Part::PartPtr part_for_work;
        {
            // Lock the mutex for the second queue
            lock_queue2.lock();
            if (shared_queue2.empty()) {
                // If there are no elements, release the mutex
                // and sleep for 1 second before the next check
                lock_queue2.unlock();
                if (done2) break; // Thread exit condition
                locked_output("threadC useless check, queue is empty. Going to bed");
                this_thread::sleep_for(chrono::milliseconds(1000));
                continue;
            }
            else {
                // Retrieve an element from the second queue
                part_for_work = shared_queue2.front();
                shared_queue2.pop();
                // Release the mutex for the second queue
                lock_queue2.unlock();
                locked_output("Part was removed from queue2");
            }
        }
        // Work on the part in thread C
        threadCwork(part_for_work);
    }
}

int main(int argc, char* argv[]) {
    list<Part::PartPtr> spare_parts;
    for (int i = 0; i < 5; i++) {
        spare_parts.push_back(Part::PartPtr(new Part{ i + 1, 10.0 }));
    }

    // Start the threads
    thread ta(threadA, std::ref(spare_parts));
    thread tb(threadB);
    thread tc(threadC);

    // Wait for completion
    ta.join();
    tb.join();
    tc.join();

    locked_output("done");
    return 0;
}
