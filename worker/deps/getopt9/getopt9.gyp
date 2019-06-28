{
  'targets':
  [
    {
		'target_name': 'getopt9',
		'type': 'static_library',
		'sources':
		[
			'include/getopt.h',
			'src/tailor.h',
			'src/getopt.c',
		],
		'include_dirs':
		[
		  'include'
		]
    }
  ]
}
