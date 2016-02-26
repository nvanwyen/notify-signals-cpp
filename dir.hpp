//
// directory.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2004-2012 Metasystems Technologies Inc. (MTI)
// All rights reserved
//
// Distributed under the MTI Software License, Version 0.1.
//
// as defined by accompanying file MTI-LICENSE-0.1.info or
// at http://www.mtihq.com/license/MTI-LICENSE-0.1.info
//

#ifndef __DIRECTORY_HPP
#define __DIRECTORY_HPP

// c
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>

// c++
#include <string>
#include <vector>

// boost
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

// local

// flag for gcc version 4.7.3 or higher
#if  __GNUC__           >= 4 && \
   ( __GNUC_MINOR__     >= 7 && \
    __GNUC_PATCHLEVEL__ >= 3 )
#define _USE_GNUCPP_473
#else
#undef  _USE_GNUCPP_473
#endif

#ifndef NONE
#define NONE 0
#endif

//
namespace mti { namespace audit { namespace shield {

//
namespace directory {

//
// There are 2 primary classes in this namespace ...
//
//  o> The "monitor" class - which signals based on directory events
//  o> The "polling" class - which signals based on a directory timer
//
//
class monitor;
class polling;

// combiner ...
template <typename container>
class aggregate
{
    public:
        typedef container result_type;

        template <typename input>
        container operator()( input first, input last ) const
        {
            //
            container values;

            //
            while ( first != last )
            {
                values.push_back( *first );
                ++first;
            }

            //
            return values;
        }

    protected:
    private:
};

//
class monitor
{
    public:
        //
        typedef boost::signals2::connection connection;

        //
        enum events
        {
            event_none           = NONE,
            event_access         = IN_ACCESS,
            event_attrib         = IN_ATTRIB,
            event_close_write    = IN_CLOSE_WRITE,
            event_close_no_write = IN_CLOSE_NOWRITE,
            event_create         = IN_CREATE,
            event_deleted        = IN_DELETE,
            event_delete_self    = IN_DELETE_SELF,
            event_modified       = IN_MODIFY,
            event_move_self      = IN_MOVE_SELF,
            event_moved_from     = IN_MOVED_FROM,
            event_moved_to       = IN_MOVED_TO,
            event_opened         = IN_OPEN,
            event_all            = IN_ALL_EVENTS
        };

        //
        struct filter
        {
            filter() : name( "" ), regex( "" ), event( event_all ) {}
            filter( events e ) : name( "" ), regex( "" ), event( e ) {}
            filter( std::string n, std::string x ) : name( n ), regex( x ), event( event_all ) {}
            filter( std::string n, std::string x, events e ) : name( n ), regex( x ), event( e ) {}

            std::string name;   // named identifier (registry)
            std::string regex;  // glob expression
            enum events event;  // monitor events

            filter& operator=( filter const& f )
            {
                name  = f.name;
                regex = f.regex;
                event = f.event;

                return *this;
            }
        };

        //
        struct query
        {
            query() {}
            query( std::string p, filter m = filter() ) : path( p ), match( m ) {}

            std::string path;
            filter      match;

            query& operator=( query const& q )
            {
                path  = q.path;
                match = q.match;

                return *this;
            }
        
            bool operator<( const query& q ) const
            {
                return path < q.path;
            }
        };

        //
        typedef std::set<query> queryset;

        //
        struct message
        {
            message() : name( "" ) { memset( &stat, 0, sizeof( struct stat ) ); }
            message( std::string n ) : name( n ) { memset( &stat, 0, sizeof( struct stat ) ); }

            std::string name;
            struct stat stat;

            enum events  event;
            filter       match;

            message& operator=( message const& m )
            {
                name  = m.name;
                stat  = m.stat;
                event = m.event;
                match = m.match;

                return *this;
            }

            bool operator<( const message& m ) const
            {
                return name < m.name;
            }
        };

        //
        typedef std::set<message> messages;

        //
        typedef boost::signals2::signal<void (messages)> signal_t;

        //
        monitor() : run_( false ), fd_( NONE ) {}
        virtual ~monitor() {}

        //
        void add_directory( std::string dir, filter match = filter() );
        void del_directory( std::string dir );

        //
        void start();
        void stop();

        //
        void interrupt();
        void join();

        //
        boost::signals2::connection connect( const signal_t::slot_type& handler );

    protected:
    private:
        //
        void init ();
        bool matches( message& m );
        bool expired( time_t tm, int sec );
        void work( query& qry );
        bool connected();

        //
        volatile bool       run_;
        boost::mutex        mutex_;
        int                 fd_;
        std::vector<int>    wd_;
        queryset            query_;
        boost::thread_group pool_;

        //
        signal_t            sig_;
};

//
class polling
{
    public:
        //
        typedef boost::signals2::connection connection;

        //
        struct filter
        {
            filter() : regex( "" ), name( "" ), recur( false ), size( NONE ), time( NONE ) {}
            filter( std::string n, std::string x ) : name( n ), regex( x ), recur( false ), size( NONE ), time( NONE ) {}
            filter( std::string n, std::string x, bool r ) : name( n ), regex( x ), recur( r ), size( NONE ), time( NONE ) {}
            filter( std::string n, std::string x, bool r, int s, int t ) : name( n ), regex( x ), recur( false ), size( s ), time( t ) {}

            std::string name;   // named identifier (registry)
            std::string regex;  // glob expression
            bool        recur;  // recusrive
            int         size;   // size greater than
            int         time;   // seconds greater than

            filter& operator=( filter const& f )
            {
                name  = f.name;
                regex = f.regex;
                recur = f.recur;
                size  = f.size;
                time  = f.time;

                return *this;
            }
        };

        //
        struct query
        {
            query() {}
            query( std::string p, filter m = filter(), size_t ms = 0 ) : path( p ), match( m ), wait( ms ) {}

            std::string path;
            filter      match;
            size_t      wait;   // interval wait milliseconds

            query& operator=( query const& q )
            {
                path  = q.path;
                match = q.match;
                wait  = q.wait;

                return *this;
            }

            bool operator<( const query& q ) const
            {
                return path < q.path;
            }
        };

        //
        typedef std::set<query> queryset;

        //
        struct message
        {
            message() : name( "" ) { memset( &stat, 0, sizeof( struct stat ) ); }
            message( std::string n ) : name( n ) { memset( &stat, 0, sizeof( struct stat ) ); }

            std::string name;
            struct stat stat;

            filter      match;

            message& operator=( message const& m )
            {
                name  = m.name;
                stat  = m.stat;
                match = m.match;

                return *this;
            }

            bool operator<( const message& m ) const
            {
                return name < m.name;
            }
        };

        //
        typedef std::set<message> messages;

        //
        typedef boost::signals2::signal<void (messages)> signal_t;


        //
        polling() : run_ ( false ) {}
        virtual ~polling() {}

        //
        void add_directory( std::string dir, filter match = filter(), size_t ms = 0 );
        void del_directory( std::string dir );

        //
        void start();
        void stop();

        //
        void interrupt();
        void join();

        //
        boost::signals2::connection connect( const signal_t::slot_type& handler );

    protected:
    private:
        //
        void work( query& dir );
        void list( query dir, messages& msg );
        bool wait( size_t ms );
        bool expired( time_t tm, int sec );
        bool matches( polling::message& m );
        bool connected();

        //
        volatile bool             run_;
        boost::mutex              mutex_;
        boost::condition_variable cond_;
        queryset                  query_;
        boost::thread_group       pool_;

        //
        signal_t                  sig_;
};

//
typedef boost::shared_ptr<monitor> monitor_ptr;
typedef boost::shared_ptr<polling> polling_ptr;

// moniker class for encapsulting an object and a connection togther
template <typename T>
struct bag
{
    T object;
    typename T::connection connection;
};

}   // namespace mti::audit::shield::directory

}}} // namespace mti::audit::shield

#endif // __DIRECTORY_HPP
