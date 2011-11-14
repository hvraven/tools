#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <sys/types.h>
#include <sys/stat.h>

using namespace boost;

struct Option_Error: public std::exception
{
  Option_Error() {};
};

struct File
{
  filesystem::path path;
  struct stat stat;
};

struct Options
{
  bool quiet;
  int max_depth;
  std::string format;
  bool sorted;
  std::string sort;
  std::string exclude;

  Options()
    : quiet(false),
      max_depth(-1),
      format("%p %l %u %g %h %M %N"),
      sorted(false),
      sort(),
      exclude()
  {
  }
};

Options options;

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

std::string print_path( filesystem::path path )
{
  std::string work = options.format;
  for (size_t pos = 0; work[pos] != '\0'; pos++)
    {
      if ( work[pos] == '%' )
	{
	  work.erase(pos, 2);
	  switch ( work[++pos] )
	    {
	    case 'n':
	      {
		work.insert(pos - 1, masquerade(path.string()));
		break;
	      }
	    case 'N':
	      {
		work.insert(pos - 1, path.string());
		break;
	      }
	    case 'b':
	      {
		std::string temp = path.string();
		size_t last = 0;
		for (size_t pos2 = 0; temp[pos2] != '\0'; pos2++)
		  if (temp[pos2] == '/')
		    last = pos2;
		temp.erase(0, last + 1);
		work.insert(pos - 1, masquerade(temp));
		break;
	      }
	    case 'B':
	      {
		std::string temp = path.string();
		size_t last = 0;
		for (size_t pos2 = 0; temp[pos2] != '\0'; pos2++)
		  if (temp[pos2] == '/')
		    last = pos2;
		temp.erase(0, last + 1);
		work.insert(pos - 1, temp);
		break;
	      }
	    }
	}
    }

  return work;
}

inline void display_path( filesystem::path path )
{
  std::cout << print_path( path ) << std::endl;
}

void list_content_unsorted( filesystem::path p, int sublevels )
{
  try
    {
      for ( filesystem::directory_iterator dir(p);
	    dir != filesystem::directory_iterator(); dir++ )
	{
	  display_path(dir->path());
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

      if ( vm.count("help") || argc == 0 )
	{
	  std::cout << generic << std::endl;
	  return 1;
	}

      if (vm.count("quiet"))
	options.quiet = true;

      if (vm.count("sort"))
	options.sorted = true;

      /* reading options to structures */
      try
	{
	  read_sort();
	}
      catch( Option_Error e )
	{
	  std::cerr << "Invalid arguments" << std::endl;
	  return 1;
	}

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
	      display_path( p );
	      return 0;
	    }
	  else if ( filesystem::is_directory(p) )
	    {
	      list_content_unsorted(p, options.max_depth);
	      return 0;
	    }
	  else
	    return 1;
	}
    }
  catch( std::exception& e )
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }

}
