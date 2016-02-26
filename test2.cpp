//
#include <string>
#include <vector>
#include <iostream>

//
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//
#include "dir.hpp"

typedef std::vector<std::string> directories;

//
class test_monitor
{
    public:
        test_monitor() : con_( dir_.connect( boost::bind( &test_monitor::handler, this, _1 ) ) ) {}
        virtual ~test_monitor() { con_.disconnect(); }

        //
        void add( directories dir )
        {
            for ( directories::iterator d = dir.begin(); d != dir.end(); ++d )
                dir_.add_directory( (*d), 
                                    mti::audit::shield::directory::monitor::filter(
                                    mti::audit::shield::directory::monitor::event_close_write ) );
        }

        //
        void start()     { dir_.start();     }
        void stop()      { dir_.stop();      }
        void join()      { dir_.join();      }
        void interrupt() { dir_.interrupt(); }

    protected:
    private:
        //
        void handler( mti::audit::shield::directory::monitor::messages msg )
        {
            typedef mti::audit::shield::directory::monitor::messages::iterator iter;

            for ( iter m = msg.begin(); m != msg.end(); ++m )
            {
                std::cout << "Monitor: " << (*m).name 
                          //<< " ; matches: " << (*m).match.name 
                          << " ; event: " << (*m).match.event 
                          << std::endl;
            }
        }

        //
        mti::audit::shield::directory::monitor             dir_;
        mti::audit::shield::directory::monitor::connection con_;
};

//
class test_polling
{
    public:
        test_polling() : con_( dir_.connect( boost::bind( &test_polling::handler, this, _1 ) ) ) {}
        virtual ~test_polling() { con_.disconnect(); }

        //
        void add( directories dir )
        {
            for ( directories::iterator d = dir.begin(); d != dir.end(); ++d )
                dir_.add_directory( (*d),
                                    mti::audit::shield::directory::polling::filter(),
                                    5000 /* = 5 sec */ );
                                    //300000 /* = 300 sec = 5 minutes */ );
        }

        //
        void start()     { dir_.start();     }
        void stop()      { dir_.stop();      }
        void join()      { dir_.join();      }
        void interrupt() { dir_.interrupt(); }

    protected:
    private:
        //
        void handler( mti::audit::shield::directory::polling::messages msg )
        {
            typedef mti::audit::shield::directory::polling::messages::iterator iter;

            for ( iter m = msg.begin(); m != msg.end(); ++m )
            {
                std::cout << "Polling: " << (*m).name 
                          //<< " ; matches: " << (*m).match.name 
                          << std::endl;
            }
        }

        //
        mti::audit::shield::directory::polling             dir_;
        mti::audit::shield::directory::polling::connection con_;
};

//
class test_moniker
{
    public:
        test_moniker()
        {
            mon_.signal = mon_.object.connect( boost::bind( &test_moniker::handler_monitor, this, _1 ) );
            pol_.signal = pol_.object.connect( boost::bind( &test_moniker::handler_polling, this, _1 ) );
        }

        virtual ~test_moniker()
        {
            mon_.signal.disconnect();
            pol_.signal.disconnect();
        }

        //
        void add( directories dir )
        {
            for ( directories::iterator d = dir.begin(); d != dir.end(); ++d )
            {
                mon_.object.add_directory( (*d), 
                                           mti::audit::shield::directory::monitor::filter(
                                           mti::audit::shield::directory::monitor::event_close_write ) );

                pol_.object.add_directory( (*d),
                                           mti::audit::shield::directory::polling::filter(),
                                           5000 /* = 5 sec */ );
            }
        }

        //
        void start()     { mon_.object.start();     pol_.object.start();     }
        void stop()      { mon_.object.stop();      pol_.object.stop();      }
        void join()      { mon_.object.join();      pol_.object.join();      }
        void interrupt() { mon_.object.interrupt(); pol_.object.interrupt(); }

    protected:
    private:
        //
        void handler_monitor( mti::audit::shield::directory::monitor::messages msg )
        {
            typedef mti::audit::shield::directory::monitor::messages::iterator iter;

            for ( iter m = msg.begin(); m != msg.end(); ++m )
            {
                std::cout << "Moniker (events): " << (*m).name 
                          << " ; event: " << (*m).match.event 
                          << std::endl;
            }
        }

        //
        void handler_polling( mti::audit::shield::directory::polling::messages msg )
        {
            typedef mti::audit::shield::directory::polling::messages::iterator iter;

            for ( iter m = msg.begin(); m != msg.end(); ++m )
            {
                std::cout << "Moniker (polled): " << (*m).name 
                          << std::endl;
            }
        }

        //
        mti::audit::shield::directory::bag<mti::audit::shield::directory::monitor> mon_;
        mti::audit::shield::directory::bag<mti::audit::shield::directory::polling> pol_;
};

//
void interruption( test_monitor* mon, test_polling* pol, test_moniker* bag )
{
    std::cout << "entering interruption thread" << std::endl;

    // in 2 minutes, interrupt the object and stop
    boost::this_thread::sleep( boost::posix_time::seconds( 15 ) );

    std::cout << "interruptiing objects" << std::endl;
    mon->stop();
    pol->stop();
    bag->stop();
}

//
int main( int argc, char** argv )
{
    //
    if ( argc < 2 )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <[directory|file]-list> ..." << std::endl;
        exit( 1 );
    }

    directories dir;
    class test_monitor test_monitor;
    class test_polling test_polling;
    class test_moniker test_moniker;

    for ( int i = 1; i < argc; ++i )
        dir.push_back( argv[ i ] );

    if ( dir.size() > 0 )
    {
        //test_monitor.add( dir );  // indivdual test
        //test_monitor.start();

        //test_polling.add( dir );  // indivdual test
        //test_polling.start();

        test_moniker.add( dir );    // collective test
        test_moniker.start();
    }

    boost::thread pardon( &interruption, &test_monitor, &test_polling, &test_moniker );
   
    std::cout << "joining indivdual objects" << std::endl;
    test_monitor.join();
    test_polling.join();
    test_moniker.join();

    std::cout << "joining interruption thread" << std::endl;
    pardon.join();

    return 0;
}
