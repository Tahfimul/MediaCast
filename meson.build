project('MediaCastClient', 'cpp')
dependencies = [
	dependency('gtk4')
]
asio_lib = library('asio', sources : ['/home/cvql/Downloads/asio-1.28.0/include/asio.hpp'], include_directories : include_directories('/home/cvql/Downloads/asio-1.28.0/include'))

incdir = include_directories('/home/cvql/Downloads/asio-1.28.0/include')
sources = ['SimpleClient.cpp', 'net_connection.h', 'net_server.h','net_client.h', 'net_threadsafeQueue.hpp', 'net_info.h', 'net_base.h','tl_net.h']
executable('client', sources, dependencies:dependencies, include_directories : incdir,
cpp_args : '-std=c++20')

