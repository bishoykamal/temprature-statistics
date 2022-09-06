#include <iostream>
#include <string.h>
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
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

    /**
     *  Pointer towards the AMQP channel
     *  @var AMQP::TcpChannel
     */
    AMQP::TcpChannel *_channel;

    /**
     *  Name of the queue
     *  @var std::string
     */
    std::string _queue;

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
        // publish a message
        double f = (double)rand() / RAND_MAX;
        double fMin = 0;
        double fMax = 100;
        f = fMin + f * (fMax - fMin);
        cout << "at time= " << seconds << " temp. = " << f << endl;
        self->_channel->publish(self->_queue, "temprature-routing-key", to_string(f));
    }

public:
    /**
     *  Constructor
     *  @param  loop
     *  @param  channel
     *  @param  queue
     */
    MyTimer(struct ev_loop *loop, AMQP::TcpChannel *channel, std::string queue) : _channel(channel), _queue(std::move(queue))
    {
        // initialize the libev structure
        ev_timer_init(&_timer, callback, 20.0, 1.0);

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

    channel.declareExchange("temprature-exchange", AMQP::fanout).onSuccess([&connection, &channel, loop]()
                                                                           {
                                                        // report the name of the temporary queue
                                                        std::cout << "declared exchange " << std::endl;

                                                        // close the channel
                                                        // channel.close().onSuccess([&connection, &channel]() {
                                                        //
                                                        //    // report that channel was closed
                                                        //    std::cout << "channel closed" << std::endl;
                                                        //
                                                        //    // close the connection
                                                        //    connection.close();
                                                        //});

                                                        // construct a timer that is going to publish stuff
                                                        auto *timer = new MyTimer(loop, &channel, "temprature-exchange"); });

    // run the loop
    ev_run(loop, 0);

    // done
    return 0;
}
