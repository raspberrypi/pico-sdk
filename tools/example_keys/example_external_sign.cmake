# Delete existing sig if present
file(REMOVE ${SIG})

if(1)
    # OpenSSL variant, for example with keys that require a password
    # You must use the Make generator if you need to pass input to
    # the shell (e.g. entering a password), as Ninja doesn't support that

    # Copy across default public key
    file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/public.pem ${PUBKEY})

    # Use openssl to sign
    execute_process(COMMAND
        openssl pkeyutl -in ${HASH} -inkey ${CMAKE_CURRENT_LIST_DIR}/private.pem -out ${SIG} -pkeyopt digest:sha256
    )
else()
    # PKCS11 variant, for example with an HSM
    # You must use the Make generator if you need to pass input to
    # the shell (e.g. entering a pin), as Ninja doesn't support that

    # Get public key from HSM
    execute_process(COMMAND
        pkcs11-tool --read-object --type pubkey --id 1 --output-file ${PUBKEY}.der
    )
    # Convert to PEM format
    execute_process(COMMAND
        openssl pkey -pubin -inform DER -in ${PUBKEY}.der -out ${PUBKEY}
    )
    file(REMOVE ${PUBKEY}.der)

    # Use pkcs11-tool to sign
    execute_process(COMMAND
        pkcs11-tool --sign --id 1 --mechanism ECDSA --input-file ${HASH} --output-file ${SIG} --signature-format openssl
    )
endif()
