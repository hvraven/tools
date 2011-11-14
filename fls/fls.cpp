#include <iostream>
#include <boost/filesystem.hpp>

using namespace boost

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
  if (argc < 2)
  {
    std::cout << "Usage: fls path\n";
    return 1;
  }

  filesystem::path p = argv[1];

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
