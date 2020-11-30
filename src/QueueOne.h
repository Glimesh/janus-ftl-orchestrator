
template <typename T>
class Queue
{
 public:
 
  T pop()
  {
    std::lock_guard<std::mutex> lock(mutex);
    while (opt.empty())
    {
      cond_.wait(mlock);
    }
    auto item = queue_.front();
    queue_.pop();
    return item;
  }
 
  void notify(const T& item)
  {
    std::lock_guard<std::mutex> lock(mutex);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }
 
  void notify(T&& item)
  {
    std::lock_guard<std::mutex> lock(mutex);
    queue_.push(std::move(item));
    mlock.unlock();
    cond_.notify_one();
  }
 
 private:
  std::optional<T> opt;
  std::mutex mutex;
  std::condition_variable cond;
};