extern "C"{
#include "queue/queue.h"
}

#include "gtest/gtest.h"
#include <thread>
#include <atomic>
// creating queue

#define QUEUE_SIZE 5

std::atomic<int> a = 0;

void f_push( Queue *q, int count ) {
    for ( int i = 0; i < count; ++i ) {
        int *v = new int;
        *v = i;
        QueuePush( q, v );
    }
    ++a;
}

void f_pop( Queue *q, int count ) {
    
    for (  int i = 0; i < count; ++i ) {
        void *v;

        QueuePop( q, &v );

        delete (int *) v;
    }
}

class QueueTest: public testing::Test {
    public:
    Queue *q;
    
    protected:
    QueueTest() {
        this->q = QueueNew( QUEUE_SIZE );
    }

    ~QueueTest() {
        QueueDelete( &( this->q ) );
    }
};


TEST_F( QueueTest, _Creation ) {

    ASSERT_TRUE( q != nullptr);
    EXPECT_EQ( QueueMaxSize( q ), 5 );
    EXPECT_EQ( QueueLength( q ), 0 );
}

TEST_F( QueueTest, _OneThreadPushAndPop) {
    for (int i = 0; i < QueueMaxSize( q ); ++i ) {
        int *v = new int;
        *v = i;
        QueuePush( q, v );

        EXPECT_EQ( QueueLength( q ), i + 1 );
    }
    EXPECT_EQ( QueueLength( q ), QueueMaxSize( q ) );

    for (int i = 0; i < QueueMaxSize( q ); ++i ) {
        void *v;
        QueuePop( q, &v );

        EXPECT_EQ( *( ( int * ) v ), i );
        EXPECT_EQ( QueueLength( q ), QueueMaxSize( q ) - i - 1 );

        delete ( int * ) v;
    }

    EXPECT_EQ( QueueLength( q ), 0 );
}

TEST_F ( QueueTest, _1Pusher1Popper ) {
    std::thread t1( f_push, q, 50000 );
    std::thread t2( f_pop, q, 50000 );

    t1.join();
    t2.join();

    EXPECT_EQ( QueueLength( q ), 0 );

}

TEST_F( QueueTest, _2Pusher2Popper ) {
    std::thread t1( f_push, q, 25000 );
    std::thread t2( f_push, q, 50000 );
    std::thread t3( f_pop, q, 25000 );
    std::thread t4( f_pop, q, 50000 );
//
    t1.join();
    t2.join();
    //EXPECT_EQ(a, 3);

    t3.join();
    t4.join();

    EXPECT_EQ( QueueLength( q ), 0 );




}
