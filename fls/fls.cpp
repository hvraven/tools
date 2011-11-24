#include <iostream>
#include <sstream>
#include <iomanip>
#include <queue>
#include <map>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>

using namespace boost;

struct Option_Error: public std::exception
{
  Option_Error() {};
};

struct File
{
  filesystem::path path;
  struct stat stat;

  File()
    : path(),
      stat()
  {
  }

  File(filesystem::path path_, struct stat stat_)
    : path(path_),
      stat(stat_)
  {
  }
};

struct Options
{
  bool quiet;
  int max_depth;
  std::string format;
  bool sorted;
  std::string sort;
  std::string exclude;
  bool reverse;
  bool end;

  Options()
    : quiet(false),
      max_depth(-1),
      format("%p %l %u %g %h %M %B"),
      sorted(false),
      sort(),
      exclude(),
      reverse(false),
      end(false)
  {
  }
};

Options options;

std::queue<File> file_queue;

enum Sort_Option_Token
{
  FILENAME,
  BASENAME,
  SIZE,
  USER,
  UID,
  GROUP,
  GID,
  INODE,
  HARDLINKS,
  EXTENSION,
  NOTEXTENSION,
  ATIME,
  MTIME,
  CTIME
};

/* switch functions for finding matching stat in Sort and Sort_End */
std::string switch_string( const File& file, const Sort_Option_Token token )
{
  switch ( token )
    {
    case FILENAME:
      return file.path.string();
    case BASENAME:
      return file.path.filename().string();
    case USER:
      return getpwuid(file.stat.st_uid)->pw_name;
    case GROUP:
      return getgrgid(file.stat.st_gid)->gr_name;
    case EXTENSION:
      return file.path.extension().string();
    case NOTEXTENSION:
      return file.path.stem().string();
    default:
      std::cerr << "Invalid element in Sort<std::string>::get_element\n";
    }
  return "";
}

/* uid_t and gid_t are unsigned int */
unsigned int switch_unsigned_int( const File& file,
                                  const Sort_Option_Token token )
{
  switch ( token )
    {
    case UID:
      return file.stat.st_uid;
    case GID:
      return file.stat.st_gid;
    default:
      std::cerr << "Invalid element in Sort<unsigned int>::get_element\n";
    }
  return 0;
}

/* nlink_t and ino_t are long unsigned int */
long unsigned int switch_long_unsigned_int( const File& file,
                                            const Sort_Option_Token token )
{
  switch ( token )
    {
    case HARDLINKS:
      return file.stat.st_nlink;
    case INODE:
      return file.stat.st_ino;
    default:
      std::cerr << "Invalid element in Sort<long unsigned int>::get_element\n";
    }
  return 0;
}

/* off_t and time_t are long int */
long int switch_long_int( const File& file,
                          const Sort_Option_Token token )
{
  switch ( token )
    {
    case SIZE:
      return file.stat.st_size;
    case ATIME:
      return file.stat.st_atime;
    case MTIME:
      return file.stat.st_mtime;
    case CTIME:
      return file.stat.st_ctime;
    default:
      std::cerr << "Invalid element in Sort<long int>::get_element\n";
    }
  return 0;
}

typedef std::vector< Sort_Option_Token > Sort_Map;

Sort_Map sort_map;

class Sort_Base
{
public:
  virtual void add( const File&, Sort_Map::const_iterator ) = 0;
  virtual void print() = 0;

protected:
  Sort_Base* next_sort_pointer( Sort_Map::const_iterator );
};


template < typename T >
class Sort : public Sort_Base
{
public:
  virtual void add( const File&, Sort_Map::const_iterator );
  virtual void print();

private:
  std::map< T, Sort_Base* > sort_map;

  T get_element( const File&, Sort_Map::const_iterator );
};

template <typename T>
void Sort<T>::add( const File& file, Sort_Map::const_iterator pos )
{
  const T element = get_element( file, pos );
  if ( ! sort_map[ element ] )
    sort_map[ element ] = next_sort_pointer( pos + 1 );

  sort_map[ element ]->add( file, pos + 1 );
}

template < typename T >
void Sort<T>::print()
{
  for ( typename std::map<T,Sort_Base*>::const_iterator it = sort_map.begin() ;
        it != sort_map.end() ; it++ )
    it->second->print();
}

template<>
inline std::string
Sort<std::string>::get_element( const File& file,
                                Sort_Map::const_iterator pos )
{
  return switch_string( file, *pos );
}

template<>
inline unsigned int
Sort<unsigned int>::get_element( const File& file,
                                 Sort_Map::const_iterator pos )
{
  return switch_unsigned_int( file, *pos );
}

template<>
inline long unsigned int
Sort<long unsigned int>::get_element( const File& file,
                                      Sort_Map::const_iterator pos )
{
  return switch_long_unsigned_int( file, *pos );
}

template<>
inline long int
Sort<long int>::get_element( const File& file,
                             Sort_Map::const_iterator pos )
{
  return switch_long_int( file, *pos );
}

std::string print_file( const File& file );

template < typename T >
class Sort_End : public Sort_Base
{
public:
  virtual void add( const File&, Sort_Map::const_iterator );
  virtual void print();

private:
  std::multimap< T, std::string > sort_map;
  T get_element( const File&, Sort_Map::const_iterator );
};

template <typename T>
inline void Sort_End<T>::add( const File& file, Sort_Map::const_iterator pos )
{
  const T element = get_element( file, pos );
  sort_map.insert( std::pair<T, std::string>(element, print_file( file )) );
}

template < typename T >
void Sort_End<T>::print()
{
  for ( typename std::multimap<T,std::string>::const_iterator it
          = sort_map.begin() ; it != sort_map.end() ; it++ )
    std::cout << it->second << std::endl;
}

template<>
inline std::string
Sort_End<std::string>::get_element( const File& file,
                                    Sort_Map::const_iterator pos )
{
  return switch_string( file, *pos );
}

template<>
inline unsigned int
Sort_End<unsigned int>::get_element( const File& file,
                                     Sort_Map::const_iterator pos )
{
  return switch_unsigned_int( file, *pos );
}

template<>
inline long unsigned int
Sort_End<long unsigned int>::get_element( const File& file,
                                          Sort_Map::const_iterator pos )
{
  return switch_long_unsigned_int( file, *pos );
}

template<>
inline long int
Sort_End<long int>::get_element( const File& file,
                                 Sort_Map::const_iterator pos )
{
  return switch_long_int( file, *pos );
}

Sort_Base* Sort_Base::next_sort_pointer( Sort_Map::const_iterator position )
{
  // last sort option
  if ( position == sort_map.end() - 1 )
    {
      switch ( *position )
        {
        case FILENAME:
        case BASENAME:
        case EXTENSION:
        case NOTEXTENSION:
        case USER:
        case GROUP:
          return new Sort_End< std::string >;
        case SIZE:
          return new Sort_End< off_t >;
        case UID:
          return new Sort_End< uid_t >;
        case GID:
          return new Sort_End< gid_t >;
        case INODE:
          return new Sort_End< ino_t >;
        case HARDLINKS:
          return new Sort_End< nlink_t >;
        case ATIME:
        case MTIME:
        case CTIME:
          return new Sort_End< time_t >;
        }
    }
  else
    {
      switch ( *position )
        {
        case FILENAME:
        case BASENAME:
        case EXTENSION:
        case NOTEXTENSION:
        case USER:
        case GROUP:
          return new Sort< std::string >;
        case SIZE:
          return new Sort< off_t >;
        case UID:
          return new Sort< uid_t >;
        case GID:
          return new Sort< gid_t >;
        case INODE:
          return new Sort< ino_t >;
        case HARDLINKS:
          return new Sort< nlink_t >;
        case ATIME:
        case MTIME:
        case CTIME:
          return new Sort< time_t >;
        }
    }
  return 0;
}

void read_sort()
{
  // TODO building sort tree
}

std::string masquerade(std::string input)
{
  std::string output;
  for (unsigned int i = 0; input[i] != '\0'; i++)
    switch ( input[i] )
      {
      case '~':
      case '\\':
      case ' ':
      case '#':
      case '"':
      case '\'':
	output += '\\';
      default:
	output += input[i];
      }
  return output;
}

std::string itoa(int value, int base)
{
  std::ostringstream result;
  result << std::setbase(base) << value;
  return result.str();
}

std::string human_size(int input)
{
  if (input <= 1024)
    {
      std::ostringstream output;
      output << input;
      return output.str();
    }

  const std::string postfix = "KMGTPE";
  double work = ((double)input) / 1024;
  for ( int postfix_count = 0; ; postfix_count++ )
    {
      if (work < 1000)
	{
	  std::ostringstream output;
	  if ( work < 100 )
	    output << std::setprecision(2);
	  else
	    output << std::setprecision(3);
	  output << work
		 << postfix[postfix_count];
	  return output.str();
	}
      work /= 1024;
    }
}

std::string permstring( const mode_t perm )
{
  std::string output = "----------";
  /* file type */
  if (perm & (1 << 15))
    {
      if (perm & (1 << 13))
	output[0] = 'l';
    }
  else
    {
      if (perm & (1 << 13))
	{
	  if (perm & (1 << 14))
	    output[0] = 'b';
	  else
	    output[0] = 'c';
	}
      if (perm & (1 << 14))
	output[0] = 'd';
    }
  if (perm & (1 << 12)) output[0] = 'p';
  /* user perm */
  if (perm & (1 << 8) ) output[1] = 'r';
  if (perm & (1 << 7) ) output[2] = 'w';
  if (perm & (1 << 6) ) output[3] = 'x';
  /* group perm */
  if ( perm & (1 << 5) ) output[4] = 'r';
  if ( perm & (1 << 4) ) output[5] = 'w';
  if ( perm & (1 << 3) ) output[6] = 'x';
  /* other perm */
  if ( perm & (1 << 2) ) output[7] = 'r';
  if ( perm & (1 << 1) ) output[8] = 'w';
  if ( perm & 1 ) output[9] = 'x';
  /* other bits */
  if ( perm & (1 << 9) )
    {
      if ( perm & 1 )
	output[9] = 't';
      else
	output[9] = 'T';
    }
  if ( perm & (1 << 10) )
    {
      if ( perm & (1 << 3) )
	output[6] = 's';
      else
	output[6] = 'S';
    }
  if ( perm & (1 << 11) )
    {
      if ( perm & (1 << 6) )
	output[3] = 's';
      else
	output[3] = 'S';
    }

  return output;
}

std::string format_time( const time_t& time )
{
  std::string work = ctime(&time);
  work.erase(0,4);
  work.erase(12, work.length() -12);
  return work;
}

std::string fill( const std::string& input, unsigned int length )
{
  const int missing = length - input.length();
  std::string work;
  for (int i = 0; i < missing; i++)
    work += ' ';
  return work + input;
}

std::string print_file( const File& file )
{
  std::string work;
  for (size_t pos = 0; options.format[pos] != '\0'; pos++)
    {
      if ( options.format[pos] == '%' )
	{
	  switch ( options.format[++pos] )
	    {
	    case 'n':
	      {
		work += masquerade(file.path.string());
		break;
	      }
	    case 'N':
	      {
		work += file.path.string();
		break;
	      }
	    case 'b':
	      {
		work += masquerade(file.path.filename().string());
		break;
	      }
            case 'B':
              {
		work += file.path.filename().string();
		break;
	      }
	    case 'u':
	      {
		struct passwd* user = getpwuid(file.stat.st_uid);
		work += user->pw_name;
		break;
	      }
	    case 'U':
	      {
		work += itoa(file.stat.st_uid,10);
		break;
	      }
	    case 'g':
	      {
		struct group* group = getgrgid(file.stat.st_gid);
		work += group->gr_name;
		break;
	      }
	    case 'G':
	      {
		work += itoa(file.stat.st_gid,10);
		break;
	      }
	    case 's':
	      {
		work += itoa(file.stat.st_size,10);
		break;
	      }
	    case 'h':
	      {
		work += fill(human_size(file.stat.st_size),4);
		break;
	      }
	    case 'p':
	      {
		work += permstring(file.stat.st_mode);
		break;
	      }
	    case 'P':
	      {
		work += itoa(file.stat.st_mode,8);
		break;
	      }
	    case 'i':
	      {
		work += itoa(file.stat.st_ino,10);
		break;
	      }
	    case 'l':
	      {
		work += itoa(file.stat.st_nlink,10);
		break;
	      }
            case 'e':
              {
                work += file.path.extension().string();
                break;
              }
            case 'E':
              {
                work += file.path.stem().string();
                break;
              }
	    case 'a':
	      {
		work += itoa(file.stat.st_atime,10);
		break;
	      }
	    case 'A':
	      {
		work += format_time(file.stat.st_atime);
		break;
	      }
	    case 'm':
	      {
		work += itoa(file.stat.st_mtime,10);
		break;
	      }
	    case 'M':
	      {
		work += format_time(file.stat.st_mtime);
		break;
	      }
	    case 'c':
	      {
		work += itoa(file.stat.st_ctime,10);
		break;
	      }
	    case 'C':
	      {
		work += format_time(file.stat.st_ctime);
		break;
	      }
	    }
	}
      else
	work += options.format[pos];
    }

  return work;
}

inline void display_file( const File& file )
{
  std::cout << print_file( file ) << std::endl;
}

void unsorted_output()
{
  for(;;)
    {
      if ( ! file_queue.empty() )
	{
	  File work = file_queue.front();
	  file_queue.pop();
	  std::cout << print_file( work ) << std::endl;
	}
      else
	{
	  if ( options.end )
	    break;
	  this_thread::sleep(posix_time::milliseconds(1));
	}
    }
}

void push_to_file_queue( File& file )
{
  file_queue.push( file );
}

void list_content_unsorted( filesystem::path p, int sublevels )
{
  try
    {
      for ( filesystem::directory_iterator dir(p);
	    dir != filesystem::directory_iterator(); dir++ )
	{
	  struct stat tempstat;
	  lstat( dir->path().c_str(), &tempstat );
	  display_file( File(dir->path(), tempstat) );
	  if ( filesystem::is_directory(*dir) )
	    if ( sublevels )
	      list_content_unsorted( *dir, sublevels - 1 );
	}
    }
  catch (const filesystem::filesystem_error& ex)
    {
      if ( ! options.quiet )
	std::cerr << p << ": permission denied" << std::endl;
    }
}

int main(int argc, char* argv[])
{
  try
    {
      program_options::options_description generic("Usage: fls [-x GLOB] [-f FMT] DIR...");
      generic.add_options()
	("help,h", "print this message")
	("format,f",
	 program_options::value<std::string>(&options.format),
	 "output format\n\
%n filename      %N raw filename\n\
%b basename      %B raw basename\n\
%u user          %u uid\n\
%g group         %G gid\n\
%s size          %h human size\n\
%p permstring    %P octal perm\n\
%i inode number  %l number of hardlinks\n\
%e extension     %E name without extension\n\
%a iso atime     %A epoch atime\n\
%m iso mtime     %M epoch mtime\n\
%c iso ctime     %C epoch ctime\n\
%F indicator (*/=|)  %_ column alignment")
	("sort,s",
	 program_options::value<std::string>(&options.sort),
	 "sort the files in order of given following arguments\n\
n filename   b basename    s size\n\
u user       U uid         g group\n\
i inode      l number of hardlinks\n\
e extension  E name without extension\n\
a atime      m mtime       c ctime")
	("reverse,r", "reverse display order")
	("max-depth,m", program_options::value<int>(&options.max_depth),
	 "max depth")
	("exclude,x",
	 program_options::value<std::string>(&options.exclude),
	 "exclude GLOB (** for recursive *)")
	("quiet,q", "don't show trivial error messages");

      program_options::options_description hidden("");
      hidden.add_options()
	("file", program_options::value<std::string>(), "file to stat");

      program_options::options_description cmdline_options;
      cmdline_options.add(generic).add(hidden);

      program_options::positional_options_description posdesc;
      posdesc.add("file", -1);

      program_options::variables_map vm;
      program_options::store(program_options::command_line_parser(argc, argv).
			     options(cmdline_options).positional(posdesc).run(),
			     vm);
      program_options::notify(vm);

      if ( vm.count("help") || argc == 1 )
	{
	  std::cout << generic << std::endl;
	  return 1;
	}

      if (vm.count("quiet"))
	options.quiet = true;

      if (vm.count("sort"))
	options.sorted = true;

      if (vm.count("reverse"))
	options.reverse = true;

      try         /* checking for invalid arguments */
	{
	  read_sort();

	  /* splitting at begin for optimization */
	  if (vm.count("sort"))
	    {
	      std::cout << "sorted check" << std::endl;
	      return 0;
	    }
	  else // not sorted
	    {
	      filesystem::path p = vm["file"].as<std::string>();
	      if ( filesystem::is_regular_file( p ) )
		{
		  struct stat tempstat;
		  lstat(p.c_str(), &tempstat);
		  display_file( File(p,tempstat) );
		  return 0;
		}
	      else if ( filesystem::is_directory(p) )
		{
		  thread output(unsorted_output);
		  list_content_unsorted(p, options.max_depth);
		  options.end = true;
		  output.join();
		  return 0;
		}
	      else
		return 1;
	    }
	}
      catch( Option_Error& e )
	{
	  std::cerr << "Invalid option!"
		    << generic
		    << std::endl;
	  return 1;
	}
    }
  catch( std::exception& e )
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }

}
