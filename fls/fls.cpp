#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace boost;

struct Options
{
  bool quiet;
  int max_depth;
  std::string format;
  std::string sort;

  Options()
    : quiet(false),
      max_depth(-1),
      format(),
      sort()
  {
  }
};

Options options;

void display_path( filesystem::path path )
{
  std::cout << path << std::endl;
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
	("format,f", program_options::value<std::string>(),
	 "output format\n\
%n filename      %N raw filename\n\
%b basename      %B raw basename\n\
%u user          %u uid\n\
%g group         %G gid\n\
%s size          %h human size\n\
%p modestring    %P octal mode\n\
%i inode number  %l number of hardlinks\n\
%e extension     %E name without extension\n\
%a iso atime     %A epoch atime\n\
%m iso mtime     %M epoch mtime\n\
%c iso ctime     %C epoch ctime\n\
%F indicator (*/=|)  %_ column alignment")
	("sort,s", program_options::value<std::string>(),
	 "sort the files in order of given following arguments\n\
n filename   b basename    s size\n\
u user       U uid         g group\n\
i inode      l number of hardlinks\n\
e extension  E name without extension\n\
a atime      m mtime       c ctime")
	("reverse,r", "reverse display order")
	("max-depth,m", program_options::value<int>(&options.max_depth),
	 "max depth")
	("exclude,x", program_options::value<std::string>(),
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

      if (vm.count("help"))
	{
	  std::cout << generic << std::endl;
	  return 1;
	}

      if (vm.count("quiet"))
	options.quiet = true;

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
