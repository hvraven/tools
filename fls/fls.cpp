#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace boost;

void list_content( filesystem::path p )
{
  for ( filesystem::directory_iterator dir(p);
	dir != filesystem::directory_iterator(); dir++ )
    {
      std::cout << *dir << std::endl;
      if ( filesystem::is_directory(*dir) )
	list_content( *dir );
    }
}

int main(int argc, char* argv[])
{
  try
    {
      program_options::options_description generic("Generic Options:");
      generic.add_options()
	("help,h", "print this message");

      program_options::options_description hidden("Hidden Options");
      hidden.add_options()
	("file", program_options::value<std::string>(), "file to stat");

      program_options::options_description cmdline_options;
      cmdline_options.add(generic).add(hidden);

      program_options::options_description visible("Usage: fls [-x GLOB] [-f FMT] DIR");
      visible.add(generic);

      program_options::positional_options_description posdesc;
      posdesc.add("file", -1);

      program_options::variables_map vm;
      program_options::store(program_options::command_line_parser(argc, argv).
			     options(cmdline_options).positional(posdesc).run(),
			     vm);
      program_options::notify(vm);

      if (vm.count("help")) {
	std::cout << visible << std::endl;
	return 1;
      }

      std::cout << "File: " << vm["file"].as<std::string>() << std::endl;

      filesystem::path p = vm["file"].as<std::string>();

      if ( filesystem::is_regular_file( p ) )
	{
	  std::cout << p << " "
		    << filesystem::file_size(p) << '\n';
	  return 0;
	}
      else if ( filesystem::is_directory(p) )
	{
	  list_content(p);
	  return 0;
	}
      else
	return 1;
    }
  catch( std::exception& e )
    {
      std::cerr << e.what() << std::endl;
      return 1;
    }
}
