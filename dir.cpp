//
// dir.cpp
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

// c

// c++
#include <stdexcept>

// boost
#include <boost/regex.hpp>
#include <boost/filesystem.hpp> 

// local
#include "dir.hpp"

//
#ifndef INVALID_HANDLE
#define INVALID_HANDLE  -1
#endif

#ifndef MONITOR_BUFFER
#define MONITOR_BUFFER  ( ( sizeof( struct inotify_event ) + FILENAME_MAX ) * 1024 )
#endif

//
namespace mti { namespace audit { namespace shield {

//
namespace directory {

////////////////////////////////////////////////////////////////////////////////
//
// class monitor
//
////////////////////////////////////////////////////////////////////////////////

monitor::monitor()
    : run_( false ),
      fd_( NONE )
{
}

//
monitor::monitor( const monitor::slot_t& handler )
    : run_( false ),
      fd_( NONE )
{
    con_ = sig_.connect( handler );
}

//
monitor::~monitor()
{
    con_.disconnect();
}

//
void monitor::add_directory( std::string dir, monitor::filter match /*= monitor::filter()*/ )
{
    boost::mutex::scoped_lock lock( mutex_ );

    if ( ! boost::filesystem::is_directory( dir ) ) 
        throw std::invalid_argument( "monitor::add_directory: " + dir + " is not a valid directory entry" ); 

    query_.insert( query( dir, match ) );
}

//
void monitor::del_directory( std::string dir )
{
    boost::mutex::scoped_lock lock( mutex_ );
    query_.erase( query( dir ) );
}

//
void monitor::start()
{
    run_ = true;

    for ( monitor::queryset::iterator q = query_.begin(); q != query_.end(); ++q )
        pool_.create_thread( boost::bind( &monitor::work, 
                                          this, 
                                          *q ) );
}

//
void monitor::stop()
{
    run_ = false;

    if ( fd_ != NONE )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        // stop inotify_read()
        for ( std::vector<HANDLE>::iterator wd = wd_.begin(); wd != wd_.end(); ++wd )
            ::inotify_rm_watch( fd_, *wd );

        wd_.clear();
        ::close( fd_ );
    }

    interrupt();
    join();
}

//
monitor::connection monitor::connect( const monitor::slot_t& handler )
{
    return ( con_ = sig_.connect( handler ) );
}

//
void monitor::interrupt()
{
    pool_.interrupt_all();
}

//
void monitor::join()
{
    pool_.join_all();
}

//
void monitor::work( monitor::query& dir )
{
    try
    {
        //
        if ( connected() )
        {
            //
            messages msg;
    
            //
            init();
    
            while ( run_ )
            {
                HANDLE wd;

                //
                msg.clear();
    
                //
                if ( ( wd = ::inotify_add_watch( fd_, dir.path.c_str(), (uint32_t)(dir.match.event) ) ) >= 0 )
                {
                    {
                        boost::mutex::scoped_lock lock( mutex_ );
                        wd_.push_back( wd );
                    }

                    size_t len, i = 0;
                    char buff[ MONITOR_BUFFER ] = {0};
    
                    //
                    len = ::read( fd_, buff, MONITOR_BUFFER );
    
                    //
                    i = 0;
                    while ( ( i < len ) && ( run_ ) )
                    {
                        struct inotify_event *pevent = ( struct inotify_event*)&buff[ i ];
    
                        //
                        if ( pevent->mask & (dir.match.event) )
                        {
                            try
                            {
                                message m;
    
                                m.name = boost::filesystem::canonical( dir.path + "/" + pevent->name ).c_str();
                                m.match = dir.match;
    
                                if ( matches( m ) )
                                    msg.insert( m ); // msg.push_back( m );
                            }
                            catch ( boost::filesystem::filesystem_error& err )
                            {
                                // do nothing ... except ignore
                            }
    
                            //
                            i += sizeof( struct inotify_event ) + pevent->len;
    
                            //
                            boost::this_thread::yield();
                        }
                    }
                }
                else
                    throw std::runtime_error( "Could not add notification monitor" );
    
                //
                if ( ( msg.size() > 0 ) && ( connected() ) )
                    sig_( msg );

                //
                boost::this_thread::yield();
            }
        }
        else
            throw std::runtime_error( "Signal slot not set" );
    }
    catch ( boost::thread_interrupted const& )
    {
        // interuption is expected, so do nothing
    }
}

//
void monitor::init()
{
    boost::mutex::scoped_lock lock( mutex_ );

    if ( fd_ == NONE )
    {
        if ( ( fd_ = ::inotify_init() ) == INVALID_HANDLE )
            throw std::runtime_error( "Invalid notify file destriptor handle" );
    }
}

//
bool monitor::matches( monitor::message& m )
{
    bool ok = false;

    if ( ::stat( m.name.c_str(), &( m.stat ) ) == 0 )
    {
        boost::regex glob( m.match.regex );

        if ( m.match.regex.length() > 0 )
        {
            boost::regex glob( m.match.regex );

            if( boost::regex_search( m.name, glob ) )
                ok = true;
        }
        else
            ok = true;
    }

    return ok;
}

//
bool monitor::expired( time_t tm, int sec )
{
    return ( tm > 0 ) ? ( ::difftime( ::time( NULL ), tm ) >= sec ) : false;
}

//
bool monitor::connected()
{
    return ( ! sig_.empty() );
}

////////////////////////////////////////////////////////////////////////////////
//
// class polling
//
////////////////////////////////////////////////////////////////////////////////

polling::polling()
    : run_( false )
{
}

//
polling::polling( const polling::slot_t& handler )
    : run_( false )
{
    con_ = sig_.connect( handler );
}

//
polling::~polling()
{
    con_.disconnect();
}

//
void polling::add_directory( std::string dir, polling::filter match /*= polling::filter()*/, size_t ms /*= 0*/ )
{
    boost::mutex::scoped_lock lock( mutex_ );

    if ( ! boost::filesystem::is_directory( dir ) ) 
        throw std::invalid_argument( "polling::add_directory: " + dir + " is not a valid directory entry" ); 

    query_.insert( query( dir, match, ms ) );
}

//
void polling::del_directory( std::string dir )
{
    boost::mutex::scoped_lock lock( mutex_ );
    query_.erase( query( dir ) );
}

//
void polling::start()
{
    run_ = true;

    for ( polling::queryset::iterator q = query_.begin(); q != query_.end(); ++q )
        pool_.create_thread( boost::bind( &polling::work, 
                                          this, 
                                          *q ) );
}

//
void polling::stop()
{
    run_ = false;
    interrupt();
    join();
}

//
void polling::interrupt()
{
    pool_.interrupt_all();
}

//
void polling::join()
{
    pool_.join_all();
}


//
polling::connection polling::connect( const polling::slot_t& handler )
{
    return ( con_ = sig_.connect( handler ) );
}

//
void polling::work( polling::query& qry )
{
    try
    {
        //
        messages msg;
    
        while ( run_ )
        {
            //
            msg.clear();
    
            //
            wait( qry.wait );
            list( qry, msg );
    
            //
            if ( ( msg.size() ) && ( connected() ) )
                sig_( msg );

            //
            boost::thread::yield();
        }
    }
    catch ( boost::thread_interrupted const& )
    {
        // interuption is expected, so do nothing
    }
}

//
void polling::list( query dir, messages& msg )
{
    if ( dir.match.recur )
    {
        boost::filesystem::recursive_directory_iterator path( dir.path ), end;

        while ( path != end )
        {
            if ( boost::filesystem::exists( *path ) )
            {
                if ( boost::filesystem::is_regular_file( *path ) )
                {
                    message m;

                    m.name = boost::filesystem::canonical( ( *path ).path().c_str() ).c_str();
                    m.match = dir.match;

                    if ( matches( m ) )
                         msg.insert( m );
                }
            }

            try
            {
                ++path;
            }
            catch ( const boost::filesystem::filesystem_error )
            {
                continue;
            }

            //
            boost::this_thread::yield();
        }
    }
    else
    {
        boost::filesystem::directory_iterator path( dir.path ), end;

        while ( path != end )
        {
            if ( boost::filesystem::exists( *path ) )
            {
                if ( boost::filesystem::is_regular_file( *path ) )
                {
                    message m;

                    m.name = ( *path ).path().c_str();
                    m.match = dir.match;

                    if ( matches( m ) )
                        msg.insert( m );
                }
            }

            try
            {
                ++path;
            }
            catch ( const boost::filesystem::filesystem_error )
            {
                continue;
            }

            //
            boost::this_thread::yield();
        }
    }
}

//
bool polling::wait( size_t ms )
{
    bool ok = false;
    boost::mutex::scoped_lock lock( mutex_ );
    boost::system_time timeout = boost::get_system_time()
                               + boost::posix_time::milliseconds( ( ms == 0 ) ? 1 : ms );
    
    ok = ( ! cond_.timed_wait( lock, timeout ) ); // returns false on timeout ... which in our case is "true"
    boost::this_thread::yield();

    return ok;
}

//
bool polling::matches( polling::message& m )
{
    bool ok = false;

    if ( ::stat( m.name.c_str(), &( m.stat ) ) == 0 )
    {
        if ( m.match.regex.length() > 0 )
        {
            boost::regex glob( m.match.regex );

            if ( boost::regex_search( m.name, glob ) )
                ok = true;
        }
        else
            ok = true;
    }

    return ok;
}

//
bool polling::expired( time_t tm, int sec )
{
    return ( tm > 0 ) ? ( ::difftime( ::time( NULL ), tm ) >= sec ) : false;
}

//
bool polling::connected()
{
    return ( ! sig_.empty() );
}

}   // namespace mti::audit::shield::directory

}}} // namespace mti::audit::shield

