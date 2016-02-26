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
class test_moniker
{
    public:
        test_moniker()
            : mon_( boost::bind( &test_moniker::handler_monitor, this, _1 ) ),
              pol_( boost::bind( &test_moniker::handler_polling, this, _1 ) )
        {}

        //
        void add( directories dir )
        {
            for ( directories::iterator d = dir.begin(); d != dir.end(); ++d )
            {
                mon_.add_directory( (*d), 
                                    mti::audit::shield::directory::monitor::filter(
                                    mti::audit::shield::directory::monitor::event_close_write ) );

                pol_.add_directory( (*d),
                                    mti::audit::shield::directory::polling::filter(),
                                    5000 /* = 5 sec */ );
            }
        }

        //
        void start()     { mon_.start();     pol_.start();     }
        void stop()      { mon_.stop();      pol_.stop();      }
        void join()      { mon_.join();      pol_.join();      }
        void interrupt() { mon_.interrupt(); pol_.interrupt(); }

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
        mti::audit::shield::directory::monitor mon_;
        mti::audit::shield::directory::polling pol_;
};

//
void interruption( test_moniker* ptr )
{
    std::cout << "entering interruption thread" << std::endl;

    // in 2 minutes, interrupt the object and stop
    boost::this_thread::sleep( boost::posix_time::seconds( 15 ) );

    std::cout << "interruptiing objects" << std::endl;
    ptr->stop();
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
    class test_moniker test_moniker;

    for ( int i = 1; i < argc; ++i )
        dir.push_back( argv[ i ] );

    if ( dir.size() > 0 )
    {
        test_moniker.add( dir );
        test_moniker.start();
    }

    boost::thread pardon( &interruption, &test_moniker );
   
    std::cout << "joining indivdual objects" << std::endl;
    test_moniker.join();

    std::cout << "joining interruption thread" << std::endl;
    pardon.join();

    return 0;
}
