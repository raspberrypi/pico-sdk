file(SHA256 "${uf2_file}" sha256sum)
file(WRITE "${uf2_file}.sha256sum" ${sha256sum})
