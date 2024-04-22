#include <iostream>
#include <mutex>
#include <thread>
#include "storage/disk/concurrent_queue.h"
const int NUM_PRODUCERS = 5;
const int NUM_CONSUMERS = 5;
const int QUEUE_CAPACITY = 100;

CrazyDave::ConcurrentQueue<int, QUEUE_CAPACITY> queue;
std::mutex cout_mutex;

void Producer(int id) {
  for (int i = 0; i < 20; ++i) {
    int value = id * 20 + i;
    if (queue.push(value)) {
      //      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Producer " << id << " pushed value: " << value << std::endl;
    } else {
      //      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Producer " << id << " failed to push value: " << value << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void Consumer(int id) {
  for (int i = 0; i < 20; ++i) {
    int value;
    if (queue.pop(value)) {
      //      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Consumer " << id << " popped value: " << value << std::endl;
    } else {
      //      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Consumer " << id << " failed to pop value." << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

auto main() -> int {
  std::thread producers[NUM_PRODUCERS];
  std::thread consumers[NUM_CONSUMERS];

  // 启动生产者线程
  for (int i = 0; i < NUM_PRODUCERS; ++i) {
    producers[i] = std::thread(Producer, i);
  }

  // 启动消费者线程
  for (int i = 0; i < NUM_CONSUMERS; ++i) {
    consumers[i] = std::thread(Consumer, i);
  }

  // 等待生产者线程完成
  for (auto &producer : producers) {
    producer.join();
  }

  // 等待消费者线程完成
  for (auto &consumer : consumers) {
    consumer.join();
  }
  std::cout << "Queue head: " << queue.get_head() << "\nQueue tail: " << queue.get_tail()
            << "\nQueue size: " << (queue.get_tail() - queue.get_head() + QUEUE_CAPACITY) % QUEUE_CAPACITY;
  return 0;
}