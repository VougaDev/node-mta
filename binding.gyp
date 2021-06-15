{
  "targets": [
    {
      "target_name": "cpp_mail",
      "sources": [ 
        "src/cpp/dkim.cpp",
        "src/cpp/model.cpp",
        "src/cpp/smtp.cpp",
        "src/cpp/main.cpp",
       ],
      'cflags_cc': [ '-fexceptions -g' ],
      'cflags': [ '-fexceptions -g' ],
      "ldflags": ["-z,defs"],
      'variables': {
        # node v0.6.x doesn't give us its build variables,
        # but on Unix it was only possible to use the system OpenSSL library,
        # so default the variable to "true", v0.8.x node and up will overwrite it.
        'node_shared_openssl%': 'true'
      },
      'conditions': [
        [
          'node_shared_openssl=="false"', {
          # so when "node_shared_openssl" is "false", then OpenSSL has been
          # bundled into the node executable. So we need to include the same
          # header files that were used when building node.
          'include_dirs': [
            '<(node_root_dir)/deps/openssl/openssl/include'
          ],
          "conditions" : [
            ["target_arch=='ia32'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/piii" ]
            }],
            ["target_arch=='x64'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/k8" ]
            }],
            ["target_arch=='arm'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/arm" ]
            }]
          ]
        }]
      ]
    }
  ]
}