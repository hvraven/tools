#include <iostream>
#include <boost/filesystem.hpp>

void list_content( boost::filesystem::path p )
{
  for ( boost::filesystem::directory_iterator dir(p);
	dir != boost::filesystem::directory_iterator(); dir++ )
    {
      std::cout << *dir << std::endl;
      if ( boost::filesystem::is_directory(*dir) )
	list_content( *dir );
    }
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Recursive ls\nUsage: fls path\n";
    return 1;
  }

  boost::filesystem::path p = argv[1];

  if ( boost::filesystem::is_regular_file( p ) )
    {
      std::cout << p << " "
		<< boost::filesystem::file_size(p) << '\n';
      return 0;
    }
  else if ( boost::filesystem::is_directory(p) )
    {
      list_content(p);
      return 0;
    }
    else
      return 1;
}
