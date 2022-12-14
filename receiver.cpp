#include <iostream>
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#include <stdio.h>
#include <time.h>

using namespace std;

/**
 *  Custom handler
 */
class MyHandler : public AMQP::LibEvHandler
{
private:
    /**
     *  Method that is called when a connection error occurs
     *  @param  connection
     *  @param  message
     */
    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        std::cout << "error: " << message << std::endl;
    }

    /**
     *  Method that is called when the TCP connection ends up in a connected state
     *  @param  connection  The TCP connection
     */
    virtual void onConnected(AMQP::TcpConnection *connection) override
    {
        std::cout << "connected" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection ends up in a ready
     *  @param  connection  The TCP connection
     */
    virtual void onReady(AMQP::TcpConnection *connection) override
    {
        std::cout << "ready" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection is closed
     *  @param  connection  The TCP connection
     */
    virtual void onClosed(AMQP::TcpConnection *connection) override
    {
        std::cout << "closed" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection is detached
     *  @param  connection  The TCP connection
     */
    virtual void onDetached(AMQP::TcpConnection *connection) override
    {
        std::cout << "detached" << std::endl;
    }

public:
    /**
     *  Constructor
     *  @param  ev_loop
     */
    MyHandler(struct ev_loop *loop) : AMQP::LibEvHandler(loop) {}

    /**
     *  Destructor
     */
    virtual ~MyHandler() = default;
};

/**
 *  Class that runs a timer
 */
class MyTimer
{
private:
    /**
     *  The actual watcher structure
     *  @var struct ev_io
     */
    struct ev_timer _timer;

    double *accumulatedTemp;
    int *count;

    /**
     *  Callback method that is called by libev when the timer expires
     *  @param  loop        The loop in which the event was triggered
     *  @param  timer       Internal timer object
     *  @param  revents     The events that triggered this call
     */
    static void callback(struct ev_loop *loop, struct ev_timer *timer, int revents)
    {
        // retrieve the this pointer
        MyTimer *self = static_cast<MyTimer *>(timer->data);
        time_t seconds;
        seconds = time(NULL);

        double average;
        if ((*(self->count)) == 0)
        {
            average = 0;
        }
        else
        {
            average = (*(self->accumulatedTemp)) / (*(self->count));
        }
        cout << "at time= " << seconds << " average = " << average << " accumulated = " << (*(self->accumulatedTemp)) << endl;
    }

public:
    /**
     *  Constructor
     *  @param  loop
     *  @param  channel
     *  @param  queue
     */
    MyTimer(struct ev_loop *loop, double *_accumulatedTemp, int *_count) : accumulatedTemp(_accumulatedTemp), count(_count)
    {
        // initialize the libev structure
        ev_timer_init(&_timer, callback, 5.0, 5.0);

        // this object is the data
        _timer.data = this;

        // and start it
        ev_timer_start(loop, &_timer);
    }

    /**
     *  Destructor
     */
    virtual ~MyTimer()
    {
        // @todo to be implemented
    }
};

/**
 *  Main program
 *  @return int
 */
int main()
{
    // access to the event loop
    auto *loop = EV_DEFAULT;

    // handler for libev
    MyHandler handler(loop);

    // init the SSL library
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif

    // make a connection
    AMQP::Address address("amqp://guest:guest@localhost/");
    //    AMQP::Address address("amqps://guest:guest@localhost/");
    AMQP::TcpConnection connection(&handler, address);

    // we need a channel too
    AMQP::TcpChannel channel(&connection);

    double acc = 0;
    int count = 0;

    // create a temporary queue
    channel.declareQueue("temprature-queue", AMQP::exclusive).onSuccess([&connection, &channel, loop, &acc, &count](const std::string &name, uint32_t messagecount, uint32_t consumercount)
                                                                        {

            // report the name of the temporary queue
            std::cout << "declared queue " << name << std::endl;

            // close the channel
            //channel.close().onSuccess([&connection, &channel]() {
            //
            //    // report that channel was closed
            //    std::cout << "channel closed" << std::endl;
            //
            //    // close the connection
            //    connection.close();
            //});

            // construct a timer that is going to publish stuff
            auto *timer = new MyTimer(loop,&acc,&count ); });
    channel.bindQueue("temprature-exchange", "temprature-queue", "temprature-routing-key");

    auto startCb = [](const std::string &consumertag)
    {
        std::cout << "consume operation started" << std::endl;
    };

    // callback function that is called when the consume operation failed
    auto errorCb = [](const char *message)
    {
        std::cout << "consume operation failed" << std::endl;
    };

    // callback operation when a message was received
    auto messageCb = [&channel, &acc, &count](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
    {
        double temp = stod(message.body());
        acc = acc + temp;
        count++;
        cout << "recieved reading num: " << count << " temp. =" << temp << endl;

        // acknowledge the message
        channel.ack(deliveryTag);
    };

    // start consuming from the queue, and install the callbacks
    channel.consume("temprature-queue")
        .onReceived(messageCb)
        .onSuccess(startCb)
        .onError(errorCb);

    // connection.close();

    // run the loop
    ev_run(loop, 0);

    // done
    return 0;
}
