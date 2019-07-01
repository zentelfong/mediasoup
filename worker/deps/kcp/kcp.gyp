{
  'targets':
  [
    {
		'target_name': 'kcp',
		'type': 'static_library',
		'sources':
		[
			'src/connection.h',
			'src/connection.cpp',
			'src/ikcp.h',
			'src/ikcp.cpp',
			'src/packetcrypto.h',
			'src/packetcrypto.cpp',
			'src/protocol.h',
		],
		'include_dirs':
		[
		  '../libuv/include',
		  './'
		]
    }
  ]
}
