//
#include <string>
#include <vector>
#include <iostream>

//
#include <boost/bind.hpp>
#include <boost/signals2.hpp>

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
        void start() { dir_.start(); }
        void stop()  { dir_.stop();  }
        void join()  { dir_.join();  }

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
        void start() { dir_.start(); }
        void stop()  { dir_.stop();  }
        void join()  { dir_.join();  }

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
            mon_.connection = mon_.object.connect( boost::bind( &test_moniker::handler_monitor, this, _1 ) );
            pol_.connection = pol_.object.connect( boost::bind( &test_moniker::handler_polling, this, _1 ) );
        }

        virtual ~test_moniker()
        {
            mon_.connection.disconnect();
            pol_.connection.disconnect();
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
        void start() { mon_.object.start(); pol_.object.start(); }
        void stop()  { mon_.object.stop();  pol_.object.stop();  }
        void join()  { mon_.object.join();  pol_.object.join();  }

    protected:
    private:
        //
        void handler_monitor( mti::audit::shield::directory::monitor::messages msg )
        {
            typedef mti::audit::shield::directory::monitor::messages::iterator iter;

            for ( iter m = msg.begin(); m != msg.end(); ++m )
            {
                std::cout << "Moniker (events): " << (*m).name 
                          //<< " ; matches: " << (*m).match.name 
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
                          //<< " ; matches: " << (*m).match.name 
                          << std::endl;
            }
        }

        //
        mti::audit::shield::directory::bag<mti::audit::shield::directory::monitor> mon_;
        mti::audit::shield::directory::bag<mti::audit::shield::directory::polling> pol_;
};

//
int main( int argc, char** argv )
{
    //
    if ( argc < 2 )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <file-list> ..." << std::endl;
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
        //test_monitor.add( dir );
        //test_monitor.start();

        //test_polling.add( dir );
        //test_polling.start();

        test_moniker.add( dir );
        test_moniker.start();
    }
    
    test_monitor.join();
    test_polling.join();
    test_moniker.join();

    return 0;
}
