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

    for ( int i = 1; i < argc; ++i )
        dir.push_back( argv[ i ] );

    if ( dir.size() > 0 )
    {
        test_monitor.add( dir );
        test_monitor.start();

        test_polling.add( dir );
        test_polling.start();
    }
    
    test_monitor.join();
    test_polling.join();

    return 0;
}
