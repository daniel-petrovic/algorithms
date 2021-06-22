#include <ctime>
#include <cstring>
#include <random>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

constexpr unsigned MAXLEN = 256;

static std::atomic<bool> g_wants[2]{};
static std::atomic<int>  g_turn(0);

struct Data
{
    char buf[MAXLEN];
    unsigned len;
};

Data gen_random_data(char c)
{
    Data d;
    d.len = rand() % MAXLEN + 1;
    for (unsigned i = 0; i < d.len; ++i) d.buf[i] = c;
    return d;
};


void critical_section(void* d)
{
    static char msg[MAXLEN + 1] = {};

    auto data = static_cast<Data*>(d);

    for (unsigned i = 0; i < data->len; ++i)
    {
        msg[i] = data->buf[i];
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    msg[data->len] = '\0';

    fprintf(stderr, "critical_section: %s\n", msg);

}

void f1()
{
    //g_wants[0] = true;
    g_wants[0].store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    while (g_wants[1].load(std::memory_order_relaxed))
    {
        if (g_turn.load(std::memory_order_relaxed) != 0)
        {
            g_wants[0].store(false, std::memory_order_relaxed);
            while (g_turn.load(std::memory_order_relaxed) != 0) {}
            g_wants[0].store(true, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    // critical section
    auto data = gen_random_data('A');

    critical_section(&data);

    g_turn.store(1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    g_wants[0].store(false, std::memory_order_relaxed);
}

void f2()
{
    g_wants[1].store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    while (g_wants[0].load(std::memory_order_relaxed))
    {
        if (g_turn.load(std::memory_order_relaxed) != 1)
        {
            g_wants[1].store(false, std::memory_order_relaxed);
            while (g_turn.load(std::memory_order_relaxed) != 1) {} 
            g_wants[1].store(true, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_acquire);
        }
    }

    std::atomic_thread_fence(std::memory_order_acquire);

    // critical section
    auto data = gen_random_data('B');

    critical_section(&data);

    g_turn.store(0, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    g_wants[1].store(false, std::memory_order_relaxed);
}


int main()
{
    srand(time(0));

    g_turn = 0;

    for (int i = 0; i < 100; ++i)
    {
        auto t1 = std::thread(f1);
        auto t2 = std::thread(f2);


        t1.join();
        t2.join();
    }
}
