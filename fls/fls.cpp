#include <iostream>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

void list_content( path p )
{
  for ( directory_iterator dir(p); dir != directory_iterator(); dir++ )
    {
      std::cout << *dir << std::endl;
      if ( is_directory(*dir) )
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

  path p = argv[1];

  if ( is_regular_file( p ) )
    {
      std::cout << p << " "
		<< boost::filesystem::file_size(p) << '\n';
      return 0;
    }
  else if ( is_directory(p) )
    {
      list_content(p);
      return 0;
    }
    else
      return 1;
}
